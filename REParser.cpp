/****************************************************************************
** 
** Copyright 2010 Alexander Krosch
** 
** This file is part of QPG - the Q Parser Generator.
** 
** QPG is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** QPG is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with QPG.  If not, see <http://www.gnu.org/licenses/>.
** 
****************************************************************************/
#include "REParser.h"

#include <QString>
#include <QVector>

#include "REParser_gen.h"
#include "utils.h"
#include "Recorder.h"

namespace {
	
	bool testForClass(QChar ch, REParser::CharClass cl) {
		switch(cl) {
			case REParser::Digit:
				return ch.isDigit();
			case REParser::NonDigit:
				return !ch.isDigit();
			case REParser::Whitespace:
				return ch.isSpace();
			case REParser::NonWhitespace:
				return !ch.isSpace();
			case REParser::WordChar:
				return ch.isLetterOrNumber()||ch.isMark()||ch==QChar('_');
			case REParser::NonWordChar:
				return !ch.isLetterOrNumber() && !ch.isMark() && ch!=QChar('_');
		}
		return false;
	}
	
	FA facache[6];
	
	FA faForCharClass(REParser::CharClass cl) {
		if(facache[cl].count())return facache[cl];
		bool m(false);
		FA fa;
		FA::State start(fa.addState());
		FA::State end(fa.addState());
		int blockStart(-1);
		for(int i=1; i<0xFFFF; i++) {
			const QChar ch(i);
			const bool d(i<0xFFFE?testForClass(ch, cl):false);
			if(d!=m) {
				if(d) {
					blockStart=i;
				} else {
					FA::State s1(fa.addState());
					FA::State s2(fa.addState());
					fa.addTransition(s1, FA::Range(FA::Symbol(blockStart), FA::Symbol(i)), s2);
					fa.addETransition(start, s1);
					fa.addETransition(s2, end);
				}
				m=d;
			}
		}
		fa.setStart(start);
		fa.addMark(end, FA::Mark(0));
		return facache[cl]=fa.deterministic();
	}
	
}

int REParser::nextCharacter() {
	currentchar=pos<buf.length()?buf.at(pos++).unicode():-1;
	return currentchar;
}

void REParser::error(const QString & m) {
	errmsg=m;
}

void REParser::issue(const QString & s) {
	issuemsg=s;
}



void REParser::beginCharacterClass(const QString & token, TokenList & q) {
	q.appendCCBEGIN(token=="[^");
	q.setStartCondition(TokenList::SC_cc);
}

void REParser::endCharacterClass(const QString &, TokenList & q) {
	q.appendCCEND();
	q.setStartCondition(TokenList::SC_INITIAL);
}

void REParser::pushCharacter(const QString & token, TokenList & q) {
	QString s(qpg::unescape(token));
	Q_ASSERT(s.length()==1);
	q.appendCHARACTER(s.at(0).unicode());
}

void REParser::pushIdentifier(const QString & token, TokenList & q) {
	q.appendIDENTIFIER(token.mid(1, token.length()-2));
}

void REParser::pushString(const QString & token, TokenList & q) {
	q.appendSTRING(qpg::unescape(token.mid(1, token.length()-2)));
}

void REParser::appendQuantifier(const QString & token, TokenList & q) {
	const QString s(token.mid(1, token.size()-2));
	int i(s.indexOf(QChar(',')));
	if(i<0) {
		i=s.toInt();
		q.appendQUANTIFIER(Quantifier(i, i));
	} else {
		const QString l(s.left(i));
		const QString r(s.mid(i+1));
		int min(0);
		int max(-1);
		if(l.size())min=l.toInt();
		if(r.size())max=r.toInt();
		q.appendQUANTIFIER(Quantifier(min, max));
	}
}


bool REParser::errorFlag()const {
	return errmsg.size();
}


FA::StatePair REParser::reunion(const FA::StatePair & p1, const FA::StatePair & p2) {
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("FA::StatePair p%1(reparser.reunion(p%2, p%3));").arg(rid).arg(p1.rid).arg(p2.rid));
	}
	return reunion_impl(p1, p2, rid);
}

FA::StatePair REParser::reunion_impl(const FA::StatePair & p1, const FA::StatePair & p2, int rid) {
	const FA::State f(fa.addState());
	const FA::State t(fa.addState());
	fa.addETransition(f, p1.first);
	fa.addETransition(f, p2.first);
	fa.addETransition(p1.second, t);
	fa.addETransition(p2.second, t);
	return FA::StatePair(f, t, rid);
}

FA::StatePair REParser::recomplement(const FA::StatePair & p1, const FA::StatePair & p2) {
	FA fa(this->fa);
	
	{
		const FA::State f(fa.addState());
		const FA::State t(fa.addState());
		fa.addETransition(f, p1.first);
		fa.addETransition(f, p2.first);
		fa.addETransition(p1.second, t);
		fa.addETransition(p2.second, t);
		
		fa.setStart(f);
		fa.addMark(p1.second, FA::Mark(1));
		fa.addMark(p2.second, FA::Mark(2));
	}
	
	fa=fa.deterministic();
	{
		const FA::State r(fa.addState());
		
		for(int i=0; i<fa.count(); i++) {
			QSet<FA::Mark> m(fa.marks(FA::State(i)));
			if(m.size()!=1)continue;
			if((*(m.begin()))!=FA::Mark(1))continue;
			fa.addETransition(FA::State(i), r);
		}
		fa.removeAllMarks();
		fa.addMark(r, FA::Mark(0));
	}
	
	fa=fa.deterministic();
	
	FA::StatePair res(this->fa.insert(fa, false));
	
	if(rec) {
		res.rid=rec->uniqueId();
		rec->addLine(QString("FA::StatePair p%1(reparser.recomplement(p%2, p%3));").arg(res.rid).arg(p1.rid).arg(p2.rid));
	}
	return res;
}

FA::StatePair REParser::reconcat_impl(const FA::StatePair & p1, const FA::StatePair & p2, int rid) {
	Q_ASSERT(p1.second.isValid() && p1.second.id()<fa.count());
	fa.addETransition(p1.second, p2.first);
	return FA::StatePair(p1.first, p2.second, rid);
}

FA::StatePair REParser::reconcat(const FA::StatePair & p1, const FA::StatePair & p2) {
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("FA::StatePair p%1(reparser.reconcat(p%2, p%3));").arg(rid).arg(p1.rid).arg(p2.rid));
	}
	return reconcat_impl(p1, p2, rid);
}

FA::StatePair REParser::reclosure_impl(const FA::StatePair & p, int rid) {
	const FA::State f(fa.addState());
	const FA::State t(fa.addState());
	fa.addETransition(f, p.first);
	fa.addETransition(f, t);
	fa.addETransition(p.second, t);
	fa.addETransition(t, p.first);
	return FA::StatePair(f, t, rid);
}

FA::StatePair REParser::reclosure(const FA::StatePair & p) {
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("FA::StatePair p%1(reparser.reclosure(p%2));").arg(rid).arg(p.rid));
	}
	return reclosure_impl(p, rid);
}

FA::StatePair REParser::repclosure(const FA::StatePair & p) {
	const FA::State f(fa.addState());
	const FA::State t(fa.addState());
	fa.addETransition(f, p.first);
	fa.addETransition(p.second, t);
	fa.addETransition(t, p.first);
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("FA::StatePair p%1(reparser.repclosure(p%2));").arg(rid).arg(p.rid));
	}
	return FA::StatePair(f, t, rid);
}

FA::StatePair REParser::reoptional_impl(const FA::StatePair & p, int rid) {
	const FA::State f(fa.addState());
	const FA::State t(fa.addState());
	fa.addETransition(f, p.first);
	fa.addETransition(f, t);
	fa.addETransition(p.second, t);
	return FA::StatePair(f, t, rid);
}

FA::StatePair REParser::reoptional(const FA::StatePair & p) {
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("FA::StatePair p%1(reparser.reoptional(p%2));").arg(rid).arg(p.rid));
	}
	return reoptional_impl(p, rid);
}

FA::StatePair REParser::quantified(const FA::StatePair & p, const Quantifier & q) {
	FA dfa;
	{
		FA fa(this->fa);
		fa.removeAllMarks();
		fa.setStart(p.first);
		fa.addMark(p.second, 0);
		dfa=fa.deterministic();
	}
	QList<FA::StatePair> pairs;
	int i(0);
	for(; i<q.min; ++i) {
		pairs.append(fa.insert(dfa, false));
	}
	if(q.max>0) {
		for(; i<q.max; ++i) {
			pairs.append(reoptional_impl(fa.insert(dfa, false), -1));
		}
	} else {
		pairs.append(reclosure_impl(fa.insert(dfa, false), -1));
	}
	for(i=1; i<pairs.size(); i++) {
		pairs[0]=reconcat_impl(pairs[0], pairs[i], -1);
	}
	Q_ASSERT(pairs.size());
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("FA::StatePair p%1(reparser.quantified(p%2, REParser::Quantifier(%3)));").arg(rid).arg(p.rid).arg(q.max>=0?QString("%1,%2").arg(q.min).arg(q.max):QString().setNum(q.min)));
	}
	pairs[0].rid=rid;
	return pairs[0];
}

FA::StatePair REParser::rechar(ushort c) {
	return rerange(c, c);
}


FA::StatePair REParser::rerange(ushort c1, ushort c2) {
	if(c2<c1) {
		error(QString("Character range is out of order: %1 - %2").arg((int)c1).arg((int)c2));
		return FA::StatePair();
	}

	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("FA::StatePair p%1(reparser.rerange(%2, %3));").arg(rid).arg(c1).arg(c2));
	}
	return rerange_impl(c1, c2, rid);
}
	
FA::StatePair REParser::rerange_impl(ushort c1, ushort c2, int rid) {
	int r1(c1);
	int r2(c2);
	
	int l1(r1&127);
	int l2(r2&127);
	
	r1=r1>>7;
	r2=r2>>7;
	
	if(!r1 && !r2) {
		const FA::State f(fa.addState());
		const FA::State t(fa.addState());
		fa.addTransition(f, FA::Range(FA::Symbol(l1), FA::Symbol(l2+1)), t);
		return FA::StatePair(f, t, rid);
	} else if(r1==r2) {
		const FA::State f(fa.addState());
		const FA::State t(fa.addState());
		fa.addTransition(f, FA::Range(FA::Symbol(l1+128), FA::Symbol(l2+128+1)), t);
		FA::StatePair p(rerange_impl((ushort)r1, (ushort)r2, rid));
		fa.addETransition(t, p.first);
		return FA::StatePair(f, p.second, rid);
	} else {
		Q_ASSERT(r1<r2);
		FA::StatePair p1(rerange_impl(c1, (r1<<7)+127, rid));
		FA::StatePair p2(rerange_impl((r2<<7), c2, rid));
		FA::StatePair res(reunion_impl(p1, p2, rid));
		if((r2-r1)>1) {
			p1=rerange_impl(r1+1, r2-1, rid);
			const FA::State f(fa.addState());
			const FA::State t(fa.addState());
			fa.addTransition(f, FA::Range(FA::Symbol(128), FA::Symbol(256)), t);
			fa.addETransition(t, p1.first);
			p2=FA::StatePair(f, p1.second);
			res=reunion_impl(res, p2, rid);
		}
		return res;
	}
	
}

FA::StatePair REParser::reclass(bool invert, const FA::StatePair & p) {
	if(!invert)return p;
	return recomplement(reinsert("."), p);
}

FA::StatePair REParser::reclass(CharClass c) {
	return fa.insert(faForCharClass(c), false);
}

FA::StatePair REParser::reinsert(const QString & id) {
	if(!defs.contains(id)) {
		error(QString("No expression '%1' defined.").arg(id));
		return FA::StatePair();
	}
	if(id==".")dot=true;
	FA::State start(fa.addState());
	FA::State end(fa.addState());
	const int offset(end.id()+1);
	const FA & src=defs[id];
	Q_ASSERT(src.isDeterministic());
	const int n(src.count());
	const int nr(src.rangeCount());
	for(int i=0; i<n; i++) {
		FA::State s(fa.addState());
		if(src.start()==FA::State(i)) {
			fa.addETransition(start, s);
		}
		if(src.marks(FA::State(i)).size()) {
			fa.addETransition(s, end);
		}
	}
	for(int i=0; i<n; i++) {
		const FA::State st_src(i);
		const FA::State st_fa(i+offset);
		for(int r=0; r<nr; r++) {
			FA::Range range(src.range(r));
			const FA::State dst(src.transition(st_src, range.from));
			if(!dst.isValid())continue;
			fa.addTransition(st_fa, range, FA::State(dst.id()+offset));
		}
/*		const QSet<FA::State> eset(src.eTransitions(st_src));
		foreach(FA::State dst, eset) {
			fa.addETransition(st_fa, FA::State(dst.id()+offset));
		}*/
	}
	
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("FA::StatePair p%1(reparser.reinsert(\"%2\"));").arg(rid).arg(id));
	}
	return FA::StatePair(start, end, rid);
}

FA::StatePair REParser::restring(const QString & s) {
	const FA::State start(fa.addState());
	FA::State cur(start);
	const int n(s.length());
	Q_ASSERT(n);
	for(int i=0; i<n; i++) {
		int c(s.at(i).unicode());
		do {
			int ec(c&127);
			c=c>>7;
			if(c)ec|=128;
			FA::State to(fa.addState());
			fa.addTransition(cur, FA::Range(FA::Symbol(ec), FA::Symbol(ec+1)), to);
			cur=to;
		} while(c);
	}
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("FA::StatePair p%1(reparser.restring(\"%2\"));").arg(rid).arg(qpg::escape_c(s)));
	}
	return FA::StatePair(start, cur, rid);
}

void REParser::convert(const FA::StatePair & p) {
	fa.setStart(p.first);
	fa.addMark(p.second, FA::Mark(endmark));
	//fa.print();
	fa=fa.deterministic();
	//fa.print();
	fa=fa.minimal();
	//fa.print();
	if(rec) {
		rec->addLine(QString("reparser.convert(p%1);").arg(p.rid));
	}
}

bool REParser::parse(const QString & str, int em) {
	if(rec) {
		rec->addLine("reparser.fa=FA();");
		rec->addLine(QString("reparser.endmark=%1;").arg(em));
	}
	errmsg="";
	issuemsg="";
	fa=FA();
	buf=str;
	currentchar=-1;
	pos=0;
	endmark=em;
	if(!parse()) {
		fa=FA();
		return false;
	}
	if(issuemsg.length()) {
		fa=FA();
		return false;
	}
	return true;
}

void REParser::defineAs(const QString & id) {
	Q_ASSERT(id=="." || !defs.contains(id));
	if(id==".") {
		Q_ASSERT(!dot);
		dot=true;
		FA fa_dot;
		const FA::State startSate(fa_dot.addState());
		const FA::State finalState(fa_dot.addState());
		fa_dot.setStart(startSate);
		Q_ASSERT(defs.contains("."));
		FA::StatePair p(fa_dot.insert(defs["."]));
		fa_dot.addETransition(startSate, p.first);
		fa_dot.addETransition(p.second, finalState);
		fa_dot.addMark(p.second, 0);
		for(int i=0; i<6; i++) {
			FA fa(fa_dot);
			p=fa.insert(faForCharClass((CharClass)i));
			fa.addETransition(startSate, p.first);
			fa.addETransition(p.second, finalState);
			fa.addMark(p.second, 1);
			fa=fa.deterministic();
			QVector<FA::State> finals;
			for(int s(0); s<fa.count(); ++s) {
				const FA::State curState(s);
				if(fa.marks(curState).size()==2)finals.append(curState);
			}
			fa.removeAllMarks();
			foreach(FA::State s, finals) {
				fa.addMark(s, 0);
			}
			fa=fa.minimal();
			predefs[i]=fa;
		}
	}
	defs[id]=fa;
	if(rec) {
		rec->addLine(QString("reparser.defineAs(\"%1\");").arg(id));
	}
}

bool REParser::isDefined(const QString & id)const {
	if(id==".")return dot;
	return defs.contains(id);
}

const QString & REParser::lastError()const {
	return errmsg;
}

REParser::REParser():rec(0) {
	clear();
	for(int i=0; i<6; i++) {
		predefs[i]=faForCharClass((CharClass)i);
	}
}

void REParser::setRecorder(Recorder *r) {
	rec=r;
}

void REParser::clear() {
	defs.clear();
	const FA::StatePair p(rerange(0, 65533));
	fa.setStart(p.first);
	fa.addMark(p.second, FA::Mark(0));
	fa=fa.deterministic();
	fa=fa.minimal();
	defs["."]=fa;
	dot=false;
	fa=FA();
}

//EOF
