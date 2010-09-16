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
#include "fa.h"

#include <QVector>
#include <QMap>
#include <QLinkedList>

#include <QBuffer>
#include <QTextStream>

FA::FA() {}

FA::State FA::addState() {
	State res(states.size());
	states.append(QSet<Mark>());
	if(!startState.isValid())startState=res;
	return res;
}

int FA::count()const {
	return states.size();
}

void FA::addMark(State s, Mark m) {
	Q_ASSERT(s.isValid());
	Q_ASSERT(s.id()<states.size());
	states[s.id()].insert(m);
}

QSet<FA::Mark> FA::marks(State s)const {
	Q_ASSERT(s.isValid());
	Q_ASSERT(s.id()<states.size());
	return states[s.id()];
}

void FA::removeAllMarks() {
	for(QList<QSet<Mark> >::iterator it(states.begin()); it!=states.end(); ++it) {
		it->clear();
	}
}

void FA::setStart(State s) {
	Q_ASSERT(s.isValid());
	Q_ASSERT(s.id()<states.size());
	startState=s;
}

FA::State FA::start()const {
	return startState;
}

bool FA::isDeterministic()const {
	return !etrans.size();
}

int FA::search(Symbol s)const {
	const int n(trans.size());
	if(!trans.size())return -1;
	if(s<trans[0].first.from)return -1;
	if(trans[n-1].first.to<=s)return 2*n-1;
	int left(0);
	int right((n-1)*2);
	while(left<right) {
		const int mid((left+right)/2);
		Q_ASSERT(mid>=0 && ((mid+1)/2<trans.size()));
		const Range r1(trans[mid/2].first);
		const Range r2(trans[(mid+1)/2].first);
		const Symbol s1(mid%2?r1.to:r1.from);
		const Symbol s2(mid%2?r2.from:r2.to);
		if(s<s1) {
			right=mid-1;
		} else if(s2<=s) {
			left=mid+1;
		} else {
			return mid;
		}
		Q_ASSERT(left<=right);
	}
	//Q_ASSERT(false);
	Q_ASSERT(left==right);
	return left;
}

bool FA::addTransition(State from, const Range & on, State to) {
// 	qDebug("bool FA::addTransition(State from:%d, const Range & on:%d-%d, State to:%d)", from.id(), on.from.id(), on.to.id(), to.id());
	print();
	Q_ASSERT(from.isValid());
	Q_ASSERT(from.id()<states.size());
	Q_ASSERT(to.isValid());
	Q_ASSERT(to.id()<states.size());
	Q_ASSERT(on.isValid());
	Range range(on);
	while(range.isValid()) {
		const Symbol left(range.from);
		const Symbol right(range.to);
		const int index(search(left));
		int slot(-1);
		range=Range();
		if(index%2 || index==-1) {
			//insert inbetween
			slot=(index+1)/2;
			Q_ASSERT(slot<=trans.size());
			Symbol nright;
			if(slot<trans.size()) {
				nright=trans[slot].first.from;
				if(right<nright) {
					nright=right;
				} else if(nright<right) {
					range=Range(nright, right);
				}
			} else {
				nright=right;
			}
			trans.insert(slot, QPair<Range, QHash<State, State> >(Range(left, nright), QHash<State, State>()));
		} else {
			Q_ASSERT(index>=0&&(index/2)<trans.size());
			const int tindex(index/2);
			slot=tindex;
			if(trans[tindex].second.contains(from))return false;
			const Symbol f(trans[tindex].first.from);
			const Symbol t(trans[tindex].first.to);
			QPair<Range, QHash<State, State> > p(trans[slot]);
			if(f<left) {
				trans.insert(slot, p);
				trans[slot].first.to=left;
				slot++;
				trans[slot].first.from=left;
				p=trans[slot];
			}
// 			qDebug("*** slot:%d", slot);
// 			print();
			if(right<t) {
				trans.insert(slot, p);
				trans[slot].first.to=right;
				trans[slot+1].first.from=right;
			}
			if(t<right) {
				range=Range(t, right);
			}
		}
		trans[slot].second[from]=to;
	}
	for(int i=0; i<trans.size()-1; i++) {
		if(trans[i].first.to.id()>trans[i+1].first.from.id()) {
			print();
			Q_ASSERT(false);
			return false;
		}
	}
	compress();
	for(int i=0; i<trans.size()-1; i++) {
		if(trans[i].first.to.id()>trans[i+1].first.from.id()) {
			print();
			Q_ASSERT(false);
			return false;
		}
	}
	return true;
}

FA::State FA::transition(State from, Symbol on)const {
	int p(search(on));
	if(p%2)return State();
	p/=2;
	Q_ASSERT(p>=0&&p<trans.size());
	return trans[p].second.value(from);
}

QVector<FA::State> FA::transitions(State from)const {
	const int n(trans.size());
	QVector<State> res(n);
	for(int i=0; i<n; i++) {
		res[i]=trans[i].second.value(from);
	}
	return res;
}

bool FA::addETransition(State from, State to) {
	Q_ASSERT(from.isValid() && from.id()<states.size());
	Q_ASSERT(to.isValid() && to.id()<states.size());
	StatePair p(from, to);
	if(etrans.contains(p))return false;
	etrans.insert(p);
	return true;
}

FA::StatePair FA::insert(const FA & src, bool keepMarks) {
	State start(addState());
	State end(addState());
	const int offset(end.id()+1);
	Q_ASSERT(src.isDeterministic());
	const int n(src.count());
	const int nr(src.rangeCount());
	for(int i=0; i<n; i++) {
		State s(addState());
		if(src.start()==State(i)) {
			addETransition(start, s);
		}
		if(src.marks(State(i)).size()) {
			addETransition(s, end);
			if(keepMarks)states[end.id()].unite(src.marks(State(i)));
		}
	}
	for(int i=0; i<n; i++) {
		const State st_src(i);
		const State st_fa(i+offset);
		for(int r=0; r<nr; r++) {
			Range range(src.range(r));
			const State dst(src.transition(st_src, range.from));
			if(!dst.isValid())continue;
			addTransition(st_fa, range, FA::State(dst.id()+offset));
		}
	}
	return StatePair(start, end);
}

int FA::rangeCount()const {
	return trans.size();
}

FA::Range FA::range(int r)const {
	return trans[r].first;
}

namespace {
	typedef QSet<FA::State> Closure;
	
/*	class Closure : public QSet<FA::State> {
		public:
			Closure() {}
			bool operator < (const Closure & o)const;
	};

	bool Closure::operator < (const Closure & o)const {
		const int n(size());
		const int no(o.size());
		if(n<no)return true;
		if(no<n)return false;
		QSet<FA::State>::const_iterator it1(begin());
		QSet<FA::State>::const_iterator it2(o.begin());
		while(it1!=end()) {
			const int id1(it1->id());
			const int id2(it2->id());
			if(id1<id2)return true;
			if(id2<id1)return false;
			++it1;
			++it2;
		}
		return false;
	}*/
	
	template<typename T>
	bool operator < (const QSet<T> & s1, const QSet<T> & s2) {
		const int n1(s1.size());
		const int n2(s2.size());
		if(n1<n2)return true;
		if(n2<n1)return false;
		typename QSet<T>::const_iterator it1(s1.begin());
		typename QSet<T>::const_iterator it2(s2.begin());
		while(it1!=s1.end()) {
			if((*it1)<(*it2))return true;
			if((*it2)<(*it1))return false;
			++it1;
			++it2;
		}
		return false;
	}
	
	bool operator < (const QVector<int> & v1, const QVector<int> & v2) {
		const int n1(v1.size());
		const int n2(v2.size());
		if(n1<n2)return true;
		if(n2<n1)return false;
		for(int i(0); i<n1; i++) {
			const int val1(v1[i]);
			const int val2(v2[i]);
			if(val1<val2)return true;
			if(val2<val1)return false;
		}
		return false;
	}
}


FA FA::deterministic()const {
	const int nstates(states.size());
	QVector<Closure> closures(nstates);
// 	typedef QPair<State, State> StatePair;
	foreach(const StatePair & p, etrans) {
		const State & from=p.first;
		const State & to=p.second;
		Q_ASSERT(from.isValid() && from.id()<closures.size());
		closures[from.id()].insert(to);
	}
	for(int i=0; i<nstates; i++) {
		closures[i].insert(State(i));
	}
	bool done(false);
	while(!done) {
		done=true;
		for(int i(0); i<nstates; i++) {
			Closure & closure=closures[i];
			foreach(State st, closure) {
				if(st.id()==i)continue;
				Q_ASSERT(st.id()>=0&&st.id()<closures.size());
				const int n(closure.size());
				closure.unite(closures[st.id()]);
				if(closure.size()>n)done=false;
			}
		}
	}
	
	//QBuffer buf;
	//buf.open(QBuffer::WriteOnly);
	//QTextStream out(&buf);
	for(int i=0; i<nstates; i++) {
		//out<<i<<":\t";
		const Closure & closure=closures[i];
		foreach(State s, closure) {
// 			out<<s.id()<<" ";
		}
		//out<<"\n";
	}
	
	QMap<Closure, State> newStates;
	QList<QSet<Mark> > dMarks;
	dMarks<<QSet<Mark>();
	Q_ASSERT(startState.id()>=0&&startState.id()<closures.size());
	newStates[closures[startState.id()]]=State(0);
	QLinkedList<Closure> todo;
	todo<<closures[startState.id()];
	
	const int nranges(trans.size());
	QVector<QPair<Range, QHash<State, State> > > dtrans(trans.size());
	for(int r(0); r<nranges; r++) {
		dtrans[r].first=trans[r].first;
	}
	
	while(todo.size()) {
		Closure closure(todo.front());
		//out<<"SRC:\t";
/*		foreach(State s, closure) {
			out<<s.id()<<" ";
		}*/
		//out<<"\n";
		todo.pop_front();
		Q_ASSERT(newStates.contains(closure));
		const State sourceState(newStates[closure]);
		
		foreach(State s, closure) {
			Q_ASSERT(s.id()>=0&&s.id()<states.size());
			dMarks[sourceState.id()].unite(states[s.id()]);
		}
		
		for(int r(0); r<nranges; r++) {
			const QHash<State, State> & h=trans[r].second;
			Closure dest;
			foreach(State s, closure) {
				QHash<State, State>::const_iterator it(h.find(s));
				if(it!=h.end())dest.unite(closures[it.value().id()]);
			}
			//out<<"DST("<<r<<"):\t";
/*			foreach(State s, dest) {
				out<<s.id()<<" ";
			}*/
			//out<<"\n\n";
			if(!dest.size())continue;
			State destState;
			QMap<Closure, State>::const_iterator it(newStates.find(dest));
			if(it==newStates.end()) {
				destState=State(newStates.size());
				newStates[dest]=destState;
				todo.append(dest);
				dMarks.append(QSet<Mark>());
				Q_ASSERT(dMarks.size()==newStates.size());
			} else {
				destState=it.value();
			}
			dtrans[r].second[sourceState]=destState;
		}
	}
	
	//out.flush();
	//qDebug("\n%s", buf.buffer().data());
	
	FA res;
	res.states=dMarks;
	res.trans=dtrans.toList();
	res.startState=State(0);
	return res;
}

FA FA::minimal()const {
	Q_ASSERT(isDeterministic());
	if(!isDeterministic()) {
		return deterministic().minimal();
	}
	const int nstates(states.size());
	const int nranges(trans.size());
	QVector<bool> reachable(nstates);
	reachable[startState.id()]=true;
	bool done(false);
	while(!done) {
		done=true;
		for(int s=0; s<nstates; s++) {
			const State st(State(s));
			for(int r=0; r<nranges; r++) {
				const QHash<State, State> & h=trans[r].second;
				QHash<State, State>::const_iterator it(h.find(State(s)));
				if(it==h.end())continue;
				const State d(*it);
				if(reachable[d.id()])continue;
				reachable[d.id()]=true;
				done=false;
			}
		}
	}
	QVector<bool> terminating(nstates);
	for(int s=0; s<nstates; s++)terminating[s]=states[s].size();
	done=false;
	while(!done) {
		done=true;
		for(int r=0; r<nranges; r++) {
			const QHash<State, State> & h=trans[r].second;
			for(QHash<State, State>::const_iterator it(h.begin()); it!=h.end(); ++it) {
				if(terminating[it.key().id()] || !terminating[it.value().id()])continue;
				done=false;
				terminating[it.key().id()]=true;
			}
		}
	}
	
	QMap<QSet<Mark>, int> marks2class;
	QList<QSet<State> > classes;
	QVector<int> state2class(nstates, -1);
	
	for(int s=0; s<nstates; s++) {
		if(reachable[s]&&terminating[s]) {
			QMap<QSet<FA::State>, int>::const_iterator it(marks2class.find(states[s]));
			if(!marks2class.contains(states[s])) {
				marks2class[states[s]]=classes.size();
				state2class[s]=classes.size();
				classes.append(QSet<FA::State>());
				classes.back().insert(State(s));
			} else {
				classes[it.value()].insert(State(s));
				state2class[s]=it.value();
			}
		}
	}
/*	{
		QBuffer buf;
		buf.open(QBuffer::WriteOnly);
		QTextStream out(&buf);
		out<<"CLASSES:\n";
		for(int i=0; i<classes.size(); i++) {
			out<<i<<":\t";
			foreach(State s, classes[i]) {
				out<<s.id()<<" ";
			}
			out<<"\n";
		}
		out.flush();
		qDebug("%s", buf.buffer().data());
	}*/
	
	QList<QSet<State> > nclasses;
	done=false;
	while(!done) {
		nclasses.clear();
		for(QList<QSet<State> >::const_iterator it(classes.begin()); it!=classes.end(); ++it) {
			const QSet<State> & cls=*it;
			QMap<QVector<int>, QSet<State> > m;
			foreach(State s, cls) {
				QVector<int> v(nranges);
				for(int r=0; r<nranges; r++) {
					v[r]=trans[r].second.value(s).id();
					if(v[r]>=0)v[r]=state2class[v[r]];
				}
				m[v].insert(s);
			}
			for(QMap<QVector<int>, QSet<State> >::const_iterator mit(m.begin()); mit!=m.end(); ++mit) {
				nclasses.append(mit.value());
			}
		}
		done=(classes.size()==nclasses.size());
		classes=nclasses;
/*		{
			QBuffer buf;
			buf.open(QBuffer::WriteOnly);
			QTextStream out(&buf);
			out<<"CLASSES:\n";
			for(int i=0; i<classes.size(); i++) {
				out<<i<<":\t";
				foreach(State s, classes[i]) {
					out<<s.id()<<" ";
				}
				out<<"\n";
			}
			out.flush();
			qDebug("%s", buf.buffer().data());
		}*/
		state2class=QVector<int>(states.size(), -1);
		for(int i=0; i<classes.size(); i++) {
			const QSet<State> & cls=classes[i];
			foreach(State s, cls) {
				state2class[s.id()]=i;
			}
		}
	}
	
	FA res;
	typedef QSet<State> EqClass;
	foreach(const EqClass & cls, classes) {
		if(cls.contains(startState))res.startState=State(res.states.size());
		const State s(*(cls.begin()));
		res.states.append(states[s.id()]);
	}
	for(int r=0; r<nranges; r++) {
		QHash<State, State> h;
		for(int s=0; s<classes.size(); s++) {
			const State from(s);
			const State e(*(classes[s].begin()));
			const State org_to(trans[r].second.value(e));
			if(org_to.isValid() && state2class[org_to.id()]>=0) {
				const State to(state2class[org_to.id()]);
				h[from]=to;
			}
		}
		res.trans.append(QPair<Range, QHash<State, State> >(trans[r].first, h));
	}
	
	res.compress();
	return res;
}

void FA::compress() {
	int i(0);
	while(i<trans.size()-1) {
		QPair<Range, QHash<State, State> > & p1=trans[i];
		const QPair<Range, QHash<State, State> > & p2=trans[i+1];
		if(p1.first.to==p2.first.from && p1.second==p2.second) {
			const Range r(p1.first.from, p2.first.to);
			p1=QPair<Range, QHash<State, State> >(r, p2.second);
			trans.removeAt(i+1);
		} else {
			i++;
		}
	}
}

bool FA::match(const QVector<Symbol> & input)const {
	if(!count())return false;
	Q_ASSERT(isDeterministic());
	if(!isDeterministic())return false;
	State curState(startState);
	const int n(input.size());
	for(int i=0; i<n; i++) {
		const int c(search(input[i]));
		if(c%2)return false;
		Q_ASSERT(c>=0&&(c/2)<trans.size());
		const QHash<State, State> & h=trans[c/2].second;
		QHash<State, State>::const_iterator it(h.find(curState));
		if(it==h.end())return false;
		curState=it.value();
	}
	return states[curState.id()].size();
}

void FA::print()const {
 	return;
	QBuffer buf;
	buf.open(QBuffer::WriteOnly);
	QTextStream out(&buf);
	out<<"\n";
	const int nr(trans.size());
	for(int i=0; i<nr; i++) {
		const Range & r=trans[i].first;
		out<<r.from.id()<<"\t"<<r.to.id()<<"\t:"<<i<<"\n";
		//qDebug("%d\t%d\t:%d", r.from.id(), r.to.id(), i);
	}
// 	qDebug("");
	out<<"\n";
	const int n(states.size());
	for(int s=0; s<n; s++) {
		const State st(s);
		if(st==startState)out<<"*";
		out<<s<<":\t";
		for(int r=0; r<nr; r++) {
			if(r)out<<"\t";
			out<<trans[r].second.value(s, State()).id();
		}
		if(states[s].size()) {
			out<<"\t[";
			foreach(Mark m, states[s])out<<m.id()<<" ";
			out<<"]";
		}
		out<<"\n";
	}
	out<<"\n";
	QHash<State, QSet<State> > eh;
	for(QSet<StatePair>::const_iterator it(etrans.begin()); it!=etrans.end(); ++it) {
		eh[it->first].insert(it->second);
	}
	for(int s=0; s<n; s++) {
		const State st(s);
		if(!eh.contains(st))continue;
		out<<s<<":\t";
		const QSet<State> & set=eh[st];
		foreach(State d, set)out<<d.id()<<"\t";
		out<<"\n";
	}
	out.flush();
	qDebug("%s", buf.buffer().data());
}

qint32 qHash(FA::Element e) {
	return qHash(e.id());
}

qint32 qHash(FA::Range r) {
	return qHash(QPair<FA::Symbol, FA::Symbol>(r.from, r.to));
}

qint32 qHash(FA::StatePair p) {
	return qHash(QPair<qint32, qint32>(qHash(p.first), qHash(p.second)));
}

//EOF
