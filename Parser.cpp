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
#include "Parser.h"

#include "Parser_gen.h"

#include <QFile>
#include <QBuffer>
#include <QRegExp>
#include "utils.h"
#include "pda.h"
#include "Recorder.h"

namespace {
	QString escape(QString s) {
		return s.replace(QChar('\\'), "\\\\").replace(QChar('"'), "\\\"");
	}
	
}

int Parser::nextCharacter() {
// 	if(pos<buf.size())std::cout<<buf[pos];
// 	currentchar=pos<buf.size()?(unsigned char)buf[pos++]:-1;
	int res(pos<buf.length()?buf.at(pos++).unicode():-1);
	if(res==10)line2++;
/*	QTextStream s(stdout);
	s<<QChar(currentchar);*/
	return res;
}

void Parser::error(const QString & s) {
	errmsg=s;
}

void Parser::error(int line, const QString & m) {
	error(QString("line %1:%2").arg(line).arg(m));
}

void Parser::issue(const QString & s) {
	issues.append(s);
}

bool Parser::errorFlag()const {
	return errmsg.length()||issues.size()>20;
}

void Parser::appendString(const QString & token, TokenList & q)const {
	q.appendSTRING(token, line1);
}

void Parser::appendIdentifier(const QString & token, TokenList & q)const {
	q.appendIDENTIFIER(token, line1);
}

void Parser::appendInteger(const QString & token, TokenList & q)const {
	q.appendINTEGER(token.toInt(), line1);
}

void Parser::appendSigned(const QString & token, TokenList & q)const {
	q.appendSIGNED(token, line1);
}

void Parser::appendFloat(const QString & token, TokenList & q)const {
	q.appendFLOAT(token, line1);
}

void Parser::appendRegExp(const QString & token, TokenList & q)const {
	q.appendREGEXP(token.mid(1, token.length()-2), line1);
}


void Parser::include(const QString & filename) {
	includes.append(filename);
	if(rec) {
		rec->addLine(QString("parser.include(\"%1\");").arg(escape(filename)));
	}
}

void Parser::setOption(const QString & option, const QString & value, int line) {
//	qDebug("void Parser::setOption(const QString & option:%s, const QString & value:%s, int line:%d)", option.toUtf8().data(), value.toUtf8().data(), line);
	if(rec) {
		rec->addLine(QString("parser.setOption(\"%1\", \"%2\", 0);").arg(option).arg(value));
	}
	if(option=="class") {
		options[OptionClass]=value;
		return;
	} else if(option=="StringType") {
		options[OptionStringType]=value;
		return;
	} else if(option=="CharacterType") {
		options[OptionCharType]=value;
		return;
	} else if(option=="TokenInfoType") {
		options[OptionInfoType]=value;
		return;
	} else if(value=="") {
		if(option=="lr1") {
			options[OptionLR1]="on";
			return;
		}
	} else if(value.indexOf("::")<0) {
		if(option=="lex") {
			options[OptionLex]=value;
			return;
		} else if(option=="parse") {
			options[OptionParse]=value;
			return;
		} else if(option=="LexerState") {
			options[OptionLexerState]=value;
			return;
		} else if(option=="TokenList") {
			options[OptionTokenList]=value;
			return;
		} else if(option=="nextCharacter") {
			options[OptionNext]=value;
			return;
		} else if(option=="error") {
			options[OptionError]=value;
			return;
		} else if(option=="errorFlag") {
			options[OptionErrorFlag]=value;
			return;
		} else if(option=="dump") {
			options[OptionDump]=value;
			return;
		} else if(option=="init") {
			options[OptionInit]=value;
			return;
		} else if(option=="issue") {
			options[OptionIssue]=value;
			return;
		} else if(option=="tokenInfo") {
			options[OptionInfoFunc]=value;
			return;
		}
	}
	
	error(line, QString("Option '%1' does not exist or value (%2) not compatible").arg(option).arg(value));
}

void Parser::define(const QString & id, const QString & regexp, int line_id, int line_re) {
	if(reparser.isDefined(id)) {
		error(line_id, QString("'%1' is already used as the name for a regular expression.").arg(id));
		return;
	}
	if(!reparser.parse(regexp)) {
		error(line_re, QString("Interpretation of the regular expression '%1' failed:\n%2").arg(regexp).arg(reparser.lastError()));
		return;
	}
	reparser.defineAs(id);
}

void Parser::addPattern(const QSet<QString> & startConditions, const QString & regexp, const QString & func, int line) {
	if(!func.size()) {
		if(anonPatterns.contains(regexp)) {
			error(line, QString("Anonymous pattern /%1/ defined twice").arg(regexp));
			return;
		}
		anonPatterns[regexp]=anonPatterns.size();
// 		symtypes[cfg.addSymbol(regexp, true)]=QString("%1::%2").arg(opt_class).arg("QString");//arg(opt_string_type);
		symtypes[cfg.addSymbol(regexp, true)]=QString("%1::%2").arg(options[OptionClass]).arg(options[OptionStringType]);
	}
	patterns.append(Pattern(startConditions, regexp, func, line));
	if(rec) {
		rec->addLine(QString("parser.addPattern(QSet<QString>()%1, \"%2\", \"%3\");").arg(startConditions.size()?QString("<<\"%1\"").arg(QStringList(startConditions.toList()).join("\"<<\"")):QString("")).arg(escape(regexp)).arg(func));
	}
}

void Parser::addAnonPattern(const QSet<QString> & startConditions, const QString & regexp, const QString & inst, int line) {
	if(anonPatterns.contains(regexp)) {
		error(line, QString("Anonymous pattern /%1/ defined twice").arg(regexp));
		return;
	}
	anonPatterns[regexp]=anonPatterns.size();
	const CFG::Symbol sym(cfg.addSymbol(regexp, true));
	symtypes[sym]=QString("%1::%2").arg(options[OptionClass]).arg(options[OptionStringType]);
	const QString i(inst.at(0)==QChar('"')?QString("\"%1\"").arg(qpg::escape_c(qpg::unescape(inst.mid(1, inst.size()-2)))):qpg::escape_c(qpg::unescape(inst)));
	instances[sym]=i;
	patterns.append(Pattern(startConditions, regexp, "", line));
	if(rec) {
		rec->addLine(QString("parser.addAnonPattern(QSet<QString>()%1, \"%2\", \"%3\");").
			arg(startConditions.size()?QString("<<\"%1\"").arg(QStringList(startConditions.toList()).join("\"<<\"")):QString(""))
			.arg(escape(regexp)).arg(qpg::escape_c(inst)));
	}
}
	
void Parser::defineTerminal(const QString & name, int line) {
	defineTerminal(name, "", "", line);
}
	
void Parser::defineTerminal(const QString & name, const QString & type, int line) {
	defineTerminal(name, type, "", line);
}

void Parser::defineTerminal(const QString & name, const QString & type, const QString & inst, int line) {
	if(cfg.findSymbol(name).isValid()) {
		error(line, QString("Terminal symbol '%1' is already defined.").arg(name));
		return;
	}
	CFG::Symbol s(cfg.addSymbol(name, true));
	Q_ASSERT(!symtypes.contains(s));
	symtypes[s]=type;
	if(inst.size()) {
		const QString i(inst.at(0)==QChar('"')?QString("\"%1\"").arg(qpg::escape_c(qpg::unescape(inst.mid(1, inst.size()-2)))):qpg::escape_c(qpg::unescape(inst)));
		instances[s]=i;
	}
	if(rec) {
		rec->addLine(QString("parser.defineTerminal(\"%1\", \"%2\", \"%3\");").arg(name).arg(type).arg(qpg::escape_c(inst)));
	}
}

void Parser::declareProduction(const QString & name, const QString & type, int line) {
	CFG::Symbol s;
	if((s=cfg.findSymbol(name)).isValid()) {
		if(cfg.hasProductionsFor(s)) {
			error(line, QString("Non-Terminal symbol '%1' declared twice.").arg(name));
			return;
		}
	} else {
		s=cfg.addSymbol(name, false);
	}
	symtypes[s]=type;
	currentProd=s;//cfg.findSymbol(name);
	if(rec) {
		rec->addLine(QString("parser.declareProduction(\"%1\", \"%2\");").arg(name).arg(type));
	}
}

void Parser::addRightHandSide(const QList<CFG::Shift> & shifts, const CFG::Action & act, int line) {
	CFG::ProductionInfo prodInfo(currentProd, shifts, act);
/*	if(!prod.check()) {
		error(QString("Invalid production: %1").arg(prod.flaws()));
		return;
	}*/
	const int n(shifts.size());
	for(int i=0; i<=n; i++) {
		const int mref=i<n?shifts[i].action().maxRef():act.maxRef();
		if(mref>(i<n?i+1:i)) {
			error(line, QString("Value reference out of range(%1)").arg(mref));
			return;
		}
	}
	cfg.addProduction(prodInfo);
	if(rec) {
		QStringList l;
		foreach(const CFG::Shift & s, shifts) {
			l<<QString("s%1").arg(s.rid);
		}
		rec->addLine(QString("parser.addRightHandSide(QList<CFG::Shift>()%1, %2);").arg(l.size()?QString("<<")+l.join("<<"):QString("")).arg(act.rid>=0?QString("a%1").arg(act.rid):QString("CFG::Action()")));
	}
}

CFG::Shift Parser::createShift(const QString & sym, const CFG::Action & act) {
	CFG::Symbol s(cfg.findSymbol(sym));
	if(!s.isValid()) {
		s=cfg.addSymbol(sym, false);
		symtypes[s]="";
	}
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("CFG::Shift s%1(parser.createShift(\"%2\", %3));").arg(rid).arg(escape(sym)).arg(act.rid>=0?QString("a%1").arg(act.rid):QString("CFG::Action()")));
	}
	return CFG::Shift(s, act, rid);
}

QString Parser::lookupAnonPattern(const QString & regexp, int line) {
	QMap<QString, int>::const_iterator it(anonPatterns.find(regexp));
	if(it==anonPatterns.end()) {
		anonPatterns[regexp]=anonPatterns.size();
		patterns.prepend(Pattern(QSet<QString>(), regexp, "", line));
		cfg.addSymbol(regexp, true);
	}
	if(rec) {
		rec->addLine(QString("parser.lookupAnonPattern(\"%1\");").arg(escape(regexp)));
	}
	return regexp;
}

CFG::Action Parser::createAction(const QList<CFG::Arg> & args) {
	const QString t(symtypes.value(currentProd));
	if(!t.length()) {
		error(QString("Left hand symbol of production (%1) is of type void.").arg(cfg.toString(currentProd)));
		return CFG::Action();
	}
	return createAction("", args);
}

CFG::Action Parser::createAction(const QString & func, const QList<CFG::Arg> & args) {
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		QStringList l;
		foreach(const CFG::Arg & a, args) {
			l<<QString("r%1").arg(a.rid);
		}
		rec->addLine(QString("CFG::Action a%1(parser.createAction(\"%2\", QList<CFG::Arg>()<<%3));").arg(rid).arg(func).arg(l.join("<<")));
	}
	return CFG::Action(func, args, rid);
}

CFG::Action Parser::createAction(int pivot, const CFG::Action & act) {
	CFG::Action res(act);
	res.setPivot(pivot);
	if(rec) {
		if(res.rid<0) {
			res.rid=rec->uniqueId();
			rec->addLine(QString("CFG::Action a%1;").arg(res.rid));
		}
		rec->addLine(QString("a%1.setPivot(%2);").arg(res.rid).arg(pivot));
	}
	return res;
}

CFG::Arg Parser::createArg(const QString & literal) {
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("const CFG::Arg r%1(parser.createArg(\"%2\"));").arg(rid).arg(escape(literal)));
	}
	return CFG::Arg(literal, rid);
}

CFG::Arg Parser::createArg(int ref) {
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("const CFG::Arg r%1(parser.createArg(%2));").arg(rid).arg(ref));
	}
	return CFG::Arg(ref, rid);
}

CFG::Arg Parser::createArgMeta(int ref) {
	int rid(-1);
	if(rec) {
		rid=rec->uniqueId();
		rec->addLine(QString("const CFG::Arg r%1(parser.createArgMeta(%2));").arg(rid).arg(ref));
	}
	return CFG::Arg(ref, true, rid);
}

QString Parser::constType(const QString & type) {
	return QString("%1 const").arg(type);
}

QString Parser::pointerType(const QString & type, bool c) {
	return QString(c?"%1 * const":"%1 *").arg(type);
}

QString Parser::basicType(const QString & name, const QList<QString> & args) {
	QString a;
	if(args.size()) {
		a+="<";
		for(int i=0; i<args.size(); i++) {
			if(i)a+=", ";
			a+=args[i];
		}
		a+=">";
	}
	return QString("%1%2").arg(name).arg(a);
}

namespace {
	QString args2string(const QList<QString> & l) {
		QString res;
		const int n(l.size());
		for(int i(0); i<n; i++) {
			if(i)res+=", ";
			res+=l[i];
		}
		return res;
	}
	
	QString targs2string(const QList<QString> & l) {
		if(!l.size())return "";
		return QString("<%1>").arg(args2string(l));
	}
}

QString Parser::functionType_rec(const QList<QString> & s1)const {
	return QString("( * (%1) )").arg(args2string(s1));
}

QString Parser::functionType_rec(const QString & s1, const QList<QString> & s2)const {
	return QString("( * %1 (%2) )").arg(s1).arg(args2string(s2));
}

QString Parser::functionType(const QString & s1, const QList<QString> & s2)const {
	return QString("%1 (*) (%2)").arg(s1).arg(args2string(s2));
}

QString Parser::functionType(const QString & s1, const QString & s2, const QList<QString> & s3)const {
	return QString("%1 %2 (%3)").arg(s1).arg(s2).arg(args2string(s3));
}

QString Parser::templateType(const QString & s1, const QString & s2, const QList<QString> & s3)const {
	return QString("%1::%2%3").arg(s1).arg(s2).arg(targs2string(s3));
}

QString Parser::templateType(const QString & s1, const QList<QString> & s2)const {
	return QString("%1::%2%3").arg(options[OptionClass]).arg(s1).arg(targs2string(s2));
}

QString Parser::unsignedType(const QString & p)const {
	return QString("unsigned %1").arg(p);
}

QString Parser::nsname(const QString & n1, const QString & n2)const {
	return n1+"::"+n2;
}

QString Parser::nameLiteral(const QString & n)const {
	return options[OptionClass]+"::"+n;
}

void Parser::resetOptions() {
	options[OptionClass]="Parser";
	options[OptionLex]="lex";
	options[OptionParse]="parse";
	options[OptionLexerState]="LexerState";
	options[OptionTokenList]="TokenList";
	options[OptionNext]="nextCharacter";
	options[OptionError]="error";
	options[OptionErrorFlag]="errorFlag";
	options[OptionStringType]="String";
	options[OptionCharType]="Char";
	options[OptionDump]="dump";
	options[OptionInit]="";
	options[OptionIssue]="issue";
	options[OptionInfoType]="";
	options[OptionInfoFunc]="tokenInfo";
	options[OptionLR1]="";
}


bool Parser::parseFile(const QString & s) {
	resetOptions();
	
	QFile file(s);
	if(!file.open(QFile::ReadOnly))return false;
	buf=QString::fromUtf8(file.readAll());
	pos=0;
	errmsg="";
	line1=1;
	line2=1;
	reparser.clear();
	patterns.clear();
	anonPatterns.clear();
	symtypes.clear();
	includes.clear();
	cfg=CFG();
	if(!parse() || issues.size()) {
		if(errmsg.length())issues.append(errmsg);
		errmsg=issues.join("\n");
		issues.clear();
		return false;
	}
	return parsePatterns();
}

void Parser::setOutputCodeFile(const QString & filename) {
	ofile.setFileName(filename);
}

void Parser::setOutputHeaderFile(const QString & filename) {
	hfile.setFileName(filename);
}

bool Parser::compileTokens(const QMap<QString, int> & startConditions) {
	const QString & opt_class=options[OptionClass];
	const QString & opt_tokenList=options[OptionTokenList];
	const QString & opt_info_type=options[OptionInfoType];
	const int n(cfg.terminalCount());
	hstream<<
		"class "<<opt_class<<"::"<<opt_tokenList<<" {\n"
		"\tpublic:\n"
		"\t\tstruct Node;\n";
	if(startConditions.size()>1) {
		hstream<<"\t\tenum SC {\n\t\t\t";
		for(QMap<QString, int>::const_iterator it(startConditions.begin()); it!=startConditions.end(); ++it) {
			if(it!=startConditions.begin())hstream<<",\n\t\t\t";
			hstream<<"SC_"<<it.key()<<"="<<it.value();
		}
		hstream<<"\n\t\t};\n";
	}
	hstream<<
		"\tprotected:\n"
		//"\t\t"<<opt_class<<" *_parser;\n"
		"\t\tNode *firstNode;\n"
		"\t\tNode *lastNode;\n";
	if(startConditions.size()>1)hstream<<"\t\tSC cursc;\n";
	hstream<<
		"\tpublic:\n"
		//"\t\t"<<opt_tokenList<<"("<<opt_class<<" *_p"<<");\n"//_parser
		"\t\t"<<opt_tokenList<<"();\n"
		"\t\t~"<<opt_tokenList<<"();\n"
		"\t\tvoid eof();\n";
	for(int i=0; i<n; i++) {
		CFG::Symbol s(i, true);
		QString sym(cfg.toString(s));
		if(anonPatterns.contains(sym))continue;
		Q_ASSERT(symtypes.contains(s));
		QString t(symtypes[s]);
		if(t=="void")t="";
		hstream<<"\t\tvoid append"<<sym<<"(";
		if(t.length()) {
			hstream<<"const "<<t;
			if(!t.endsWith("*"))hstream<<" & ";
			hstream<<"val";
		}
		if(opt_info_type.length())hstream<<", const "<<opt_class<<"::"<<opt_info_type<<" & meta";
		hstream<<");\n";
	}
	if(startConditions.size()>1) {
		hstream<<
			"\t\tvoid setStartCondition(SC sc) {cursc=sc;}\n"
			"\t\tSC startCondition()const {return cursc;}\n";
	}
	hstream<<
		"};\n"
		"\n";
	hstream.flush();
	ostream<<
		"struct "<<opt_class<<"::"<<opt_tokenList<<"::Node {\n"
		"\tNode *ref;\n"
		"\tint mark;\n";
	if(opt_info_type.length())ostream<<"\t"<<opt_class<<"::"<<opt_info_type<<" info;\n";
	ostream<<
		"\tNode():ref(0), mark(0) {}\n"
		"\tNode(int m"<<(opt_info_type.length()?QString(", const %1::%2 & i").arg(opt_class).arg(opt_info_type):QString(""))<<"):ref(0), mark(m)"<<(opt_info_type.length()?", info(i)":"")<<" {}\n"
		"\t~Node() {if(ref)delete ref;}\n"
		"\tvirtual void foo() {}\n"
		"};\n"
		"\n"
		"namespace {\n"
		"\ttemplate<typename T>\n"
		"\tstruct NodeImpl : public "<<opt_class<<"::"<<opt_tokenList<<"::Node {\n"
		"\t\tT value;\n"
		"\t\tNodeImpl(int m"<<(opt_info_type.length()?QString(", const %1::%2 & i").arg(opt_class).arg(opt_info_type):QString(""))<<", const T & v=T()):Node(m"<<(opt_info_type.length()?", i":"")<<"), value(v) {}\n"
		"\t};\n"
		"\t\n"
		"\tclass "<<opt_tokenList<<"Impl : public "<<opt_class<<"::"<<opt_tokenList<<" {\n"
		"\t\tpublic:\n"
		//"\t\t\t"<<opt_tokenList<<"Impl("<<opt_class<<" *_p=0):"<<opt_tokenList<<"(_p) {}\n"//_parser
		"\t\t\t"<<opt_tokenList<<"Impl() {}\n"
		"\t\t\tvoid append(Node *node);\n"
		"\t\t\tvoid prepend(Node *node);\n"
		"\t\t\tNode *first()const;\n"
		//"\t\t\tNode *last()const;\n"
		"\t\t\tvoid removeFirst();\n"
		"\t\t\tbool isEmpty()const;\n"
		"\t};\n"
		"\t\n"
		"\tvoid "<<opt_tokenList<<"Impl::append(Node *node) {\n"
		"\t\tif(lastNode)lastNode->ref=node;\n"
		"\t\tlastNode=node;\n"
		"\t\tif(!firstNode)firstNode=node;\n"
		"\t}\n"
		"\t\n"
		"\tvoid "<<opt_tokenList<<"Impl::prepend(Node *node) {\n"
		"\t\tnode->ref=firstNode;\n"
		"\t\tfirstNode=node;\n"
		"\t\tif(!lastNode)lastNode=node;\n"
		"\t}\n"
		"\t\n"
		"\t"<<opt_class<<"::"<<opt_tokenList<<"::Node *"<<opt_tokenList<<"Impl::first()const {\n"
		"\t\tQ_ASSERT(firstNode);\n"
		"\t\tNode *res(firstNode);\n"
		"\t\treturn res;\n"
		"\t}\n"
		"\t\n"
		/*"\t"<<opt_class<<"::"<<opt_tokenList<<"::Node *"<<opt_tokenList<<"Impl::last()const {\n"
		"\t\tQ_ASSERT(lastNode);\n"
		"\t\tNode *res(lastNode);\n"
		"\t\treturn res;\n"
		"\t}\n"
		"\t\n"
		*/"\tvoid "<<opt_tokenList<<"Impl::removeFirst() {\n"
		"\t\tQ_ASSERT(firstNode);\n"
		"\t\tNode *node(firstNode);\n"
		"\t\tfirstNode=node->ref;\n"
		"\t\tif(!firstNode)lastNode=0;\n"
		"\t\tnode->ref=0;\n"
		"\t}\n"
		"\t\n"
		"\tbool "<<opt_tokenList<<"Impl::isEmpty()const {\n"
		"\t\treturn !firstNode;\n"
		"\t}\n"
		"}\n"
		"\n"
		//<<opt_class<<"::"<<opt_tokenList<<"::"<<opt_tokenList<<"("<<opt_class<<" *_p):_parser(_p), firstNode(0), lastNode(0)"<<(startConditions.size()>1?", cursc(SC_INITIAL)":"")<<" {}\n"
		<<opt_class<<"::"<<opt_tokenList<<"::"<<opt_tokenList<<"():firstNode(0), lastNode(0)"<<(startConditions.size()>1?", cursc(SC_INITIAL)":"")<<" {}\n"
		"\n"
		<<opt_class<<"::"<<opt_tokenList<<"::~"<<opt_tokenList<<"() {\n"
		"\tif(firstNode)delete firstNode;\n"
		"}\n"
		"\n"
		//"void "<<opt_class<<"::"<<opt_tokenList<<"::eof() {\n"
		//"\t"<<opt_tokenList<<"::Node *newNode(new "<<opt_tokenList<<"::Node("<<cfg.terminalCount()<<(opt_info_type.length()?QString(", _parser->%1()").arg(opt_info_func):QString(""))<<"));\n"
		"void "<<opt_class<<"::"<<opt_tokenList<<"::eof() {\n"
		"\t"<<opt_tokenList<<"::Node *newNode(new "<<opt_tokenList<<"::Node("<<cfg.terminalCount()<<(opt_info_type.length()?QString(", %1::%2()").arg(opt_class).arg(opt_info_type):QString(""))<<"));\n"
		"\tif(lastNode)lastNode->ref=newNode;\n"
		"\tlastNode=newNode;\n"
		"\tif(!firstNode)firstNode=newNode;\n"
		"}\n"
		"\n";
	for(int i=0; i<n; i++) {
		CFG::Symbol s(i, true);
		QString sym(cfg.toString(s));
		if(anonPatterns.contains(sym))continue;
		Q_ASSERT(symtypes.contains(s));
		QString t(symtypes[s]);
		if(t=="void")t="";
		ostream<<"void "<<opt_class<<"::"<<opt_tokenList<<"::append"<<sym<<"(";
		if(t.length()) {
			ostream<<"const "<<t;
			if(!t.endsWith("*"))ostream<<" & ";
			ostream<<"val";
		}
		if(opt_info_type.length())ostream<<", const "<<opt_class<<"::"<<opt_info_type<<" & meta";//instead of _parser
		ostream<<") {\n";
		if(t.length()) {
			//ostream<<"\t"<<opt_tokenList<<"::Node *newNode(new NodeImpl<"<<t<<" >("<<i<<(opt_info_type.length()?QString(", _parser->%1()").arg(opt_info_func):QString(""))<<", val));\n";
			ostream<<"\t"<<opt_tokenList<<"::Node *newNode(new NodeImpl<"<<t<<" >("<<i<<(opt_info_type.length()?QString(", meta"):QString(""))<<", val));\n";
		} else {
			//ostream<<"\t"<<opt_tokenList<<"::Node *newNode(new Node("<<i<<(opt_info_type.length()?QString(", _parser->%1()").arg(opt_info_func):QString(""))<<"));\n";
			ostream<<"\t"<<opt_tokenList<<"::Node *newNode(new Node("<<i<<(opt_info_type.length()?QString(", meta"):QString(""))<<"));\n";
		}
		ostream<<
			"\tif(lastNode)lastNode->ref=newNode;\n"
			"\tlastNode=newNode;\n"
			"\tif(!firstNode)firstNode=newNode;\n"
			"}\n"
			"\n";
	}
	ostream.flush();
	return true;
}

namespace {
	void printClassesArray(const char *name, const FA & fa, QTextStream & ostream) {
		ostream<<
			"\tconst int "<<name<<"[]={\n"
			"\t\t";
		const int nranges(fa.rangeCount());
		int col(0);
		for(int r=0; r<nranges; r++) {
			const FA::Range range(fa.range(r));
			for(int c=range.from.id(); c<range.to.id(); c++) {
				if(r || c>range.from.id()) {
					ostream<<", ";
					if(!col)ostream<<"\n\t\t";
				}
				ostream<<QString("%1").arg(r, 4);
				col=(col+1)%10;
			}
			if(r<nranges-1) {
				const FA::Range & next=fa.range(r+1);
				for(int c=range.to.id(); c<next.from.id(); c++) {
					ostream<<", ";
					if(!col)ostream<<"\n\t\t";
					ostream<<"  -1";
					col=(col+1)%10;
				}
			}
		}
		ostream<<
			"\n"
			"\t};\n";
	}

	void printTransitionsArray(const char *name, const FA & fa, QTextStream & ostream) {
		const int n(fa.count());
		const int nranges(fa.rangeCount());
		ostream<<"\tconst int "<<name<<"[]={";
		for(int s=0; s<n; s++) {
			const QVector<FA::State> v(fa.transitions(FA::State(s)));
			Q_ASSERT(v.size()==nranges);
			for(int r=0; r<nranges; r++) {
				if(s || r)ostream<<", ";
				if(!r)ostream<<"\n\t\t";
				ostream<<QString("%1").arg(v[r].id(), 4);
			}
		}
		ostream<<"\n\t};\n";
	}
}

bool Parser::parsePatterns() {
	int i(0);
	for(QLinkedList<Pattern>::iterator it(patterns.begin()); it!=patterns.end(); ++it) {
		Pattern & pattern=*it;
		if(!reparser.parse(pattern.re, i)) {
			error(pattern.line, QString("Failed to parse regular expression '%1': %2").arg(pattern.re).arg(reparser.lastError()));
			return false;
		}
		pattern.fa=reparser.result();
		if(rec) {
			rec->addLine(QString("(*(parser.patterns.begin()+%1)).fa=reparser.result();").arg(i));
		}
		i++;
	}
	return true;
}

bool Parser::compilePatterns(const QMap<QString, int> & startConditions) {
	const QString & opt_class=options[OptionClass];
	const QString & opt_string_type=options[OptionStringType];
	const QString & opt_lexer_state=options[OptionLexerState];
	const QString & opt_lex=options[OptionLex];
	const QString & opt_tokenList=options[OptionTokenList];
	const QString & opt_next_char=options[OptionNext];
	const QString & opt_issue=options[OptionIssue];
	const QString & opt_dump=options[OptionDump];
	const QString & opt_info_type=options[OptionInfoType];
	const QString & opt_info_func=options[OptionInfoFunc];
	const QString & opt_char_type=options[OptionCharType];
	const QString & opt_error=options[OptionError];
	int i(0);
	FA fa;
	QList<FA::State> starts;
	foreach(const Pattern & pattern, patterns) {
/*		if(!reparser.parse(pattern.re, i)) {
			error(QString("Failed to parse regular expression '%1': %2").arg(pattern.re).arg(reparser.lastError()));
			return false;
		}
		starts.append(fa.insert(reparser.result()).first);
		i++;*/
		starts.append(fa.insert(pattern.fa).first);
		i++;
	}
	if(startConditions.size()>1) {
		if(fa.rangeCount()<startConditions.size()) {
			const FA::State d1(fa.addState());
			const FA::State d2(fa.addState());
			while(fa.rangeCount()<startConditions.size()) {
				fa.addTransition(d1, FA::Range(fa.range(fa.rangeCount()-1).to), d2);
			}
		}
		i=0;
		foreach(const Pattern & pattern, patterns) {
			QSet<QString> sc(pattern.sc);
			if(!sc.size())sc.insert("INITIAL");
			const FA::State from(fa.addState());
			const FA::State to(starts[i]);
			foreach(const QString & c, sc) {
				Q_ASSERT(startConditions.contains(c));
				fa.addTransition(from, fa.range(startConditions[c]), to);
			}
			starts[i]=from;
			i++;
		}
	}
	FA::State start(fa.addState());
	fa.setStart(start);
	foreach(const FA::State & s, starts) {
		fa.addETransition(start, s);
	}
	fa=fa.deterministic();
	int n(fa.count());
	const int np(patterns.size());
	QVector<FA::Mark> marks(n, FA::Mark(np));
	for(int i=0; i<n; i++) {
		QSet<FA::Mark> s(fa.marks(FA::State(i)));
		foreach(FA::Mark m, s) {
			marks[i]=qMin(marks[i], m);
		}
	}
	fa.removeAllMarks();
	for(int i=0; i<n; i++) {
		if(marks[i]<FA::Mark(np))fa.addMark(FA::State(i), marks[i]);
	}
	fa=fa.minimal();
// 	fa.print();
	n=fa.count();
	
	const int nranges(fa.rangeCount());
	const int firstCode(fa.range(0).from.id());
	const int lastCode(fa.range(nranges-1).to.id()-1);
	
	ostream<<
		"\n"
		"namespace {\n";
	printClassesArray("lc", fa, ostream);
	ostream<<
		"\n";
	printTransitionsArray("lt", fa, ostream);
	ostream<<
		"\n"
		"\tconst int lm[]={\n\t\t";
	for(int s=0; s<n; s++) {
		const QSet<FA::Mark> m(fa.marks(FA::State(s)));
		if(s)ostream<<", ";
		if(m.size()) {
			ostream<<(m.begin()->id());
		} else {
			ostream<<"-1";
		}
	}
	
	const QString string_type(opt_class+"::"+opt_string_type);
	
	ostream<<
		"\n"
		"\t};\n"
		"\n"
		"}\n"
		"\n"
		"struct "<<opt_class<<"::"<<opt_lexer_state<<" {\n"
		"\tint currentChar;\n"
		"\t"<<opt_tokenList<<"Impl list;\n"
		//"\t"<<opt_lexer_state<<"("<<opt_class<<" *_p):currentChar(-1), list(_p)"//_parser
		"\t"<<opt_lexer_state<<"():currentChar(-1)"
		" {}\n"
		"};\n"
		"\n"
		"bool "<<opt_class<<"::"<<opt_lex<<"("<<opt_lexer_state<<" & state) {\n"
		"\tint curc(state.currentChar);\n"
		"\tif(curc<0) {\n"
		"\t\tcurc="<<opt_next_char<<"();\n"
		"\t\tif(curc<0) {\n"
// 		"\t\t\tstate.list.append(new "<<opt_tokenList<<"::Node("<<cfg.terminalCount()<<"));\n"
		"\t\t\tstate.list.eof();\n"
		"\t\t\treturn true;\n"
		"\t\t}\n"
		"\t}\n"
		"\tint fastate(";
	if(startConditions.size()>1) {
		ostream<<"lt[state.list.startCondition()+"<<(nranges*fa.start().id())<<"]";
	} else {
		ostream<<fa.start().id();
	}
	ostream<<
		");\n"
		"\t"<<string_type<<" token;\n"
		"\twhile(true) {\n"
		"\t\tint n(-1);\n"
		"\t\tif(curc>=0) {\n"
		"\t\t\tn=fastate;\n"
		"\t\t\tint c(curc);\n"
		"\t\t\tdo {\n"
		"\t\t\t\tint ec(c&127);\n"
		"\t\t\t\tc=c>>7;\n"
		"\t\t\t\tif(c)ec|=128;\n"
		"\t\t\t\tif(ec<"<<firstCode<<" || ec>"<<lastCode<<") {\n"
		"\t\t\t\t\t"<<opt_issue<<"(QString(\"Read unknown symbol with code %1.\").arg(curc));\n"
		"\t\t\t\t\tstate.currentChar=-1;\n"
		"\t\t\t\t\treturn false;\n"
		"\t\t\t\t}\n"
		"\t\t\t\tconst int r(lc[ec-"<<firstCode<<"]);\n"
		"\t\t\t\tif(r<0) {\n"
		"\t\t\t\t\t"<<opt_issue<<"(QString(\"Read unknown symbol with code %1.\").arg(curc));\n"
		"\t\t\t\t\tstate.currentChar=-1;\n"
		"\t\t\t\t\treturn false;\n"
		"\t\t\t\t}\n"
		"\t\t\t\tn=lt[n*"<<nranges<<"+r];\n"
		"\t\t\t} while(c && n>=0);\n"
		"\t\t}\n"
		"\t\tif(n<0) {\n"
		"\t\t\tstate.currentChar=curc;\n"
		"\t\t\tconst int m(lm[fastate]);\n"
		"\t\t\tif(m<0) {\n"
		"\t\t\t\t"<<opt_issue<<"(QString(\"Read unknown token:'%1'.\").arg(token));\n"
		"\t\t\t\tstate.currentChar=-1;\n"
		"\t\t\t\treturn false;\n"
		"\t\t\t}\n"
		"\t\t\tswitch(m) {\n";
	i=0;
	foreach(const Pattern & p, patterns) {
		ostream<<
			"\t\t\t\tcase "<<i<<":\n"
			"\t\t\t\t\t//"<<p.re<<"\n";
		if(p.func.length()) {
			if(p.func==opt_dump) {
				ostream<<
					"\t\t\t\t\treturn true;\n";
			} else {
				ostream<<
					"\t\t\t\t\tthis->"<<p.func<<"(token, state.list);\n"
					"\t\t\t\t\treturn !(this->errorFlag());\n";
			}
		} else {
			CFG::Symbol s(cfg.findSymbol(p.re));
			Q_ASSERT(s.isValid());
			Q_ASSERT(s.isTerminal());
			ostream<<
				//"\t\t\t\t\tstate.list.append(new "<<opt_tokenList<<"::Node("<<s.hash()<<"));\n"
				"\t\t\t\t\tstate.list.append(new NodeImpl<"<<string_type<<" >("<<s.hash()<<(opt_info_type.length()?QString(", this->%1()").arg(opt_info_func):QString(""))<<", token));\n"
				"\t\t\t\t\treturn true;\n";
		}
		i++;
	}
	ostream<<
		"\t\t\t}\n"
		"\t\t} else {\n"
		"\t\t\ttoken.append("<<opt_class<<"::"<<opt_char_type<<"(curc));\n"
		"\t\t\tfastate=n;\n"
		"\t\t\tcurc="<<opt_next_char<<"();\n"
		"\t\t}\n"
		"\t}\n"
		"\t"<<opt_error<<"(QString(\"Unexpected end of file.\"));\n"
		"\treturn false;\n"
		"}\n"
		"\n";
		
	ostream.flush();
		
	return true;
}

namespace {
	class Compiler {
		private:
			CFG cfg;
			QMap<CFG::Symbol, QString> symtypes;
			QMap<CFG::Symbol, QString> instances;
			
			PDA pda;
			int numSymbols;
			QVector<int> table;
			QVector<int> sr;
			
			QStringList errmsg;
			void error(const QString & m) {
				errmsg.append(m);
			}
			bool createPda(bool lr1);
			void createTable();
			bool createSR();
			void createErrorRecovery();
		public:
			Compiler(const CFG & c, const QMap<CFG::Symbol, QString> & s, const QMap<CFG::Symbol, QString> & i):cfg(c), symtypes(s), instances(i) {}
			QSet<int> descendants(int state, int sym)const;
			bool compile(bool lr1);
			void print(QTextStream & ostream, const QString *options);
			QString errors()const;
	};
	
	QString Compiler::errors()const {
		return errmsg.join("\n");
	}
	
	bool Compiler::createPda(bool lr1) {
		{
			const QSet<CFG::Symbol> s(cfg.startSymbols());
			if(!s.size()) {
				errmsg.append("Grammer is empty");
				return false;
			}
			if(s.size()>1) {
				QStringList l;
				foreach(CFG::Symbol sym, s)l.append(cfg.toString(sym));
				errmsg.append(QString("The grammer contains more than one candidate for the start symbol:%1").arg(l.join(", ")));
				return false;
			}
		}
		pda=cfg.toPDA(lr1);
		if(!pda.isValid()) {
			errmsg.append("Could not create push down automaton accepting supplied grammer.");
			return false;
		}
		return true;
	}
	
	void Compiler::createTable() {
		const FA & fa=pda.fa();
		const int numSymbols(this->numSymbols=fa.range(fa.rangeCount()-1).to.id());
		table.resize(numSymbols*fa.count());
		for(int s(0); s<fa.count(); s++) {
			const FA::State state(s);
			for(int c(0); c<numSymbols; c++) {
				const FA::State dst(fa.transition(state, FA::Symbol(c)));
				table[s*numSymbols+c]=dst.id();
			}
		}
	}
	
	bool Compiler::createSR() {
		const FA & fa=pda.fa();
		sr.resize(numSymbols*fa.count());
		
		bool res(true);
		for(int s(0); s<fa.count(); s++) {
			for(int r=0; r<=cfg.terminalCount(); r++) {
				QSet<PDA::Action> a(pda.lookAheadActions(FA::State(s), r));
				const QSet<PDA::Shift> shifts(pda.nonActions(FA::State(s), r));
				if(a.size()>1) {
					res=false;
					QString text;
					QTextStream out(&text);
					const FA::Range range(pda.fa().range(r));
					const int nt(cfg.terminalCount());
					QStringList syms;
					for(int i(range.from.id()); i<range.to.id(); i++) {
						if(i<nt) {
							syms.append(cfg.toString(CFG::Symbol(i, true)));
						} else if(i==nt) {
							syms.append("<End Of Input>");
						} else {
							syms.append(cfg.toString(CFG::Symbol(i-nt-1, false)));
						}
					}
					out<<"Decision-conflict on symbol(s):\n\t"<<syms.join(", ")<<"\nbetween:\n";
					foreach(PDA::Action action, a) {
						if(!action.isValid()) {
							foreach(PDA::Shift sh, shifts) {
								const CFG::Production prod(pda.production(sh));
								const int mark(pda.mark(sh));
								const CFG::ProductionInfo & pinfo=cfg.productionInfo(prod);
								out<<"\t"<<cfg.toString(CFG::Symbol(pinfo.leftSide(), false))<<" : (";
								const int n(pinfo.size());
								for(int i(0); i<n; i++) {
									if(i)out<<" ";
									if(i==mark)out<<"*";
									out<<cfg.toString(pinfo.shift(i).symbol());
								}
								out<<")\n";
							}
							continue;
						}
						const CFG::Production prod(pda.production(action));
						const int mark(pda.mark(action));
						const CFG::ProductionInfo & pinfo=cfg.productionInfo(prod);
						out<<"\t"<<cfg.toString(CFG::Symbol(pinfo.leftSide(), false))<<" : (";
						const int n(pinfo.size());
						for(int i(0); i<n; i++) {
							if(i)out<<" ";
							if(i==mark)out<<"*";
							out<<cfg.toString(pinfo.shift(i).symbol());
						}
						out<<")\n";
					}
					out.flush();
					error(text);
				} else {
					const int index(s*numSymbols+r);
					if(!a.size()) {
						sr[index]=0;
					} else {
						Q_ASSERT(a.size()==1);
						const PDA::Action act(*(a.begin()));
						if(act.isValid()) {
							const int id(act.id()+1);
							if(act.isShiftAction()) {
								sr[index]=-id;
							} else {
								sr[index]=id;
							}
						} else {
							sr[index]=0;
						}
					}
				}
			}
		}
		
		if(!res)return false;
		
		if(pda.shiftActionCount()) {
			for(int a=0; a<pda.shiftActionCount(); a++) {
				const PDA::Action pdaAction(pda.shiftAction(a));
				const CFG::Production prod(pda.production(pdaAction));
				Q_ASSERT(prod.isValid());
				const int pmark(pda.mark(pdaAction));
				Q_ASSERT(pmark>=0);
				const CFG::ProductionInfo & pinfo=cfg.productionInfo(prod);
				Q_ASSERT(pmark<pinfo.size());
				const CFG::Action & cfgAction=pinfo.shift(pmark).action();
				for(int i=0; i<cfgAction.count(); i++) {
					const CFG::Arg & arg=cfgAction.arg(i);
					if(arg.isReference()) {
						if(arg.isMetaReference()) {
							error(QString("???META???"));
							return false;
						}
						const int ref(arg.reference());
						const QString t(symtypes.value(pinfo.shift(ref-1).symbol()));
						if(t=="" || t=="void") {
							error(QString("???SHIFT TYPE???"));
							return false;
						}
					}
				}
			}
		}
		
		for(int a=0; a<pda.reduceActionCount(); a++) {
			const PDA::Action pdaAction(pda.reduceAction_i(a));
			const CFG::Production prod(pda.production(pdaAction));
			if(prod.id()==cfg.count()) {
				continue;
			}
			Q_ASSERT(prod.isValid());
			const CFG::ProductionInfo & pinfo=cfg.productionInfo(prod);
			const CFG::Action & cfgAction=pinfo.action();
			const QString rt(symtypes.value(CFG::Symbol(pinfo.leftSide(), false)));
			const int pivot(cfgAction.pivot());
			{
				if(pivot) {
					if(pivot>pinfo.size()) {
						error(QString("pivot out of range in production:%1").arg(cfg.toString(pinfo)));
						return false;
					}
					if(symtypes.value(pinfo.shift(pivot-1).symbol())!=rt) {
						error(QString("Type of pivot does not match the type of the production: %1").arg(cfg.toString(pinfo)));
						return false;
					}
				}
				bool call(false);
				if(cfgAction.function().size() || (!pivot && rt!="" && rt!="void" && cfgAction.count())) {
					call=true;
					for(int i=0; i<cfgAction.count(); i++) {
						const CFG::Arg & arg=cfgAction.arg(i);
						if(arg.isReference() && !arg.isMetaReference()) {
							const int ref(arg.reference());
							const QString t(symtypes.value(pinfo.shift(ref-1).symbol()));
							if(t=="" || t=="void") {
								error(QString("The type of the right hand side symbol at position %1 in the production %2 is void.").arg(ref).arg(cfg.toString(pinfo)));
								return false;
							}
						}
					}
				}
			}
		}
		
		return true;
	}
	
	void Compiler::createErrorRecovery() {
		const int numStates(pda.fa().count());
		const int numNonTerminals(cfg.nonterminalCount());
		const int numTerminals(cfg.terminalCount());
		
		QVector<QSet<int> > targetsByNonTerminal(numNonTerminals);
		for(int i(0); i<numNonTerminals; ++i) {
			QSet<int> & targets=targetsByNonTerminal[i];
			const int sym(i+numTerminals+1);
			Q_ASSERT(sym<numSymbols);
			for(int s(0); s<numStates; ++s) {
				const int t(table[s*numSymbols+sym]);
				if(t>=0)targets.insert(t);
			}
		}
		
		QVector<QHash<int, QSet<int> > > F(numStates);
		QVector<double> ranks(numStates);
		
		for(int i(0); i<numStates; ++i) {
			for(int sym(0); sym<numTerminals; ++sym) {
				if(table[i*numSymbols+sym]>=0)ranks[i]+=1;
				QSet<int> states;
				QLinkedList<int> toProcess;
				toProcess<<i;
				QVector<bool> visited(numStates, false);
				while(toProcess.size()) {
					const int state(toProcess.first());
					toProcess.removeFirst();
					if(visited[state])continue;
					visited[state]=true;
					const int index(state*numSymbols+sym);
					const int a(sr[index]);
					if(a>0) {
						const int nonterminal(cfg.productionInfo(pda.production(pda.reduceAction_i(a-1))).leftSide());
						Q_ASSERT(nonterminal>=0 && nonterminal<numNonTerminals);
						const QSet<int> & t=targetsByNonTerminal[nonterminal];
						foreach(int target, t)toProcess.append(target);
					} else if(!a && table[index]>=0) {
						states.insert(table[index]);
					} else {
						toProcess.clear();
						states.clear();
					}
				}
				if(states.size()) {
					F[i][sym]=states;
				}
			}
		}
		
		for(int i(0); i<numStates; ++i) {
			for(int sym(0); sym<numTerminals; ++sym) {
				const int index(i*numSymbols+sym);
				if(sr[index])continue;
				if(table[index]>=0)continue;
				double rank(0);
				int winner(-1);
				for(int cand(0); cand<numTerminals; ++cand) {
					if(!F[i].contains(cand))continue;
					const QSet<int> & states=F[i][cand];
					Q_ASSERT(states.size());
					bool suc(true);
					QSet<int> targets;
					foreach(int st, states) {
/*						const int index(st*numSymbols+sym);
						if(sr[index]==0 && table[index]<0) {
							suc=false;
							break;
						}*/
						if(!F[st].contains(sym)) {
							suc=false;
							break;
						}
						targets.unite(F[st][sym]);
					}
					if(!suc)break;
					double r(0);
					foreach(int t, targets)r+=ranks[t];
					if(r>=rank) {
						rank=r;
						winner=cand;
					}
				}
				if(winner>=0) {
					table[index]=-(winner+2);
				}
			}
		}
	}
	
	bool Compiler::compile(bool lr1) {
		if(!createPda(lr1))return false;
		createTable();
		if(!createSR())return false;
		createErrorRecovery();
		return true;
	}
	
	void Compiler::print(QTextStream & ostream, const QString *options) {
		const QString opt_class(options[Parser::OptionClass]);
		const QString opt_parse(options[Parser::OptionParse]);
		const QString opt_tokenList(options[Parser::OptionTokenList]);
		const QString opt_error(options[Parser::OptionError]);
		const QString opt_errorFlag(options[Parser::OptionErrorFlag]);
		const QString opt_lexer_state(options[Parser::OptionLexerState]);
		const QString opt_lex(options[Parser::OptionLex]);
		const QString opt_init(options[Parser::OptionInit]);
		const QString opt_issue(options[Parser::OptionIssue]);
		const QString opt_info_type(options[Parser::OptionInfoType]);
		const QString opt_info_func(options[Parser::OptionInfoFunc]);
		
		const int numStates(pda.fa().count());
		ostream<<
			"namespace {\n"
			"\tconst int pt[]={\n\t\t";
		for(int s(0); s<numStates; s++) {
			if(s)ostream<<",\n\t\t";
			for(int c(0); c<numSymbols; c++) {
				if(c)ostream<<", ";
				ostream<<QString("%1").arg(table[s*numSymbols+c], 4);
			}
		}
		ostream<<"\n\t};\n";
		ostream<<"\n";
		ostream<<"\tconst int sr[]={\n\t\t";
		for(int s(0); s<numStates; s++) {
			if(s)ostream<<",\n\t\t";
			for(int t(0); t<numSymbols; t++) {
				if(t)ostream<<", ";
				ostream<<QString("%1").arg(sr[s*numSymbols+t], 4);
			}
		}
		ostream<<"\n\t};\n";
		ostream<<"\tconst QStringList tokenNames(QStringList()";
		for(int i=0; i<cfg.terminalCount(); i++) {
			ostream<<"<<\n\t\t\""<<cfg.toString(CFG::Symbol(i, true)).replace(QChar('\\'), "\\\\").replace(QChar('"'), "\\\"")<<"\"";
		}
		ostream<<"<<\n\t\t\"<EOF>\");\n";
		ostream<<"}\n\n";
		ostream<<
			"bool "<<opt_class<<"::"<<opt_parse<<"() {\n"
			//"\t"<<opt_tokenList<<"Impl stack(this);\n"//_parser
			"\t"<<opt_tokenList<<"Impl stack;\n"
			"\tstack.prepend(new "<<opt_tokenList<<"::Node("<<pda.fa().start().id()<<(opt_info_type.length()?QString(", %1()").arg(opt_info_type):QString(""))<<"));\n"
			//"\t"<<opt_lexer_state<<" lexerState(this);\n"//_parser
			"\t"<<opt_lexer_state<<" lexerState;\n"
			"\t"<<opt_tokenList<<"Impl & tokens=lexerState.list;\n";
		if(opt_init.length()) {
			ostream<<"\tthis->"<<opt_init<<"(tokens);\n";
		}
		ostream<<
			"\tbool done(false);\n"
			//"\twhile("<<opt_lex<<"(lexerState)) {\n"
			"\twhile(!done) {\n"
			"\t\twhile(tokens.isEmpty()) {\n"
			"\t\t\tthis->"<<opt_lex<<"(lexerState);\n"
			"\t\t\tif(this->"<<opt_errorFlag<<"())return false;\n"
			"\t\t}\n"
			"\t\twhile(!tokens.isEmpty()) {\n"
			"\t\t\t"<<opt_tokenList<<"::Node *node(tokens.first());\n"
			"\t\t\tconst int lasymbol(node->mark);\n"
			"\t\t\tif(lasymbol=="<<cfg.terminalCount()<<")done=true;\n"
			"\t\t\tconst int curstate(stack.first()->mark);\n"
			"\t\t\tconst int act(lasymbol<"<<numSymbols<<"?sr[curstate*"<<numSymbols<<"+lasymbol]:0);\n"
			"\t\t\tif(act<=0) {\n"
			"\t\t\t\tconst int nstate(pt[curstate*"<<numSymbols<<"+lasymbol]);\n"
			"\t\t\t\tif(nstate<0) {\n"
			"\t\t\t\t\t"<<opt_issue<<"(QString(\"Unexpected token:%1\").arg(::tokenNames.value(lasymbol)));\n";
			//"\t\t\t\t\treturn false;\n"
		if(instances.size()) {
			ostream<<
				"\t\t\t\t\tswitch(-nstate) {\n";
			for(QMap<CFG::Symbol, QString>::const_iterator it(instances.begin()); it!=instances.end(); ++it) {
				ostream<<
					"\t\t\t\t\t\tcase "<<(it.key().hash()+2)<<":\n"
					"\t\t\t\t\t\t\ttokens.prepend(new NodeImpl<"<<symtypes[it.key()]<<" >("<<it.key().hash()<<(opt_info_type.length()?QString(", %1::%2()").arg(opt_class).arg(opt_info_type):QString(""))<<", "<<it.value()<<"));\n"
					"\t\t\t\t\t\t\tbreak;\n";
			}
			ostream<<
				"\t\t\t\t\t\tdefault:\n"
				"\t\t\t\t\t\t\ttokens.removeFirst();\n"
				"\t\t\t\t\t\t\tdelete node;\n"
				"\t\t\t\t\t}\n";
		} else {
			ostream<<
				"\t\t\t\t\ttokens.removeFirst();\n"
				"\t\t\t\t\tdelete node;\n";
		}
		ostream<<
			"\t\t\t\t} else {\n"
			"\t\t\t\t\ttokens.removeFirst();\n"
			"\t\t\t\t\tstack.prepend(node);\n"
			"\t\t\t\t\tnode->mark=nstate;\n";
		if(pda.shiftActionCount()) {
			ostream<<"\t\t\t\t\tif(act<0)switch(-act) {\n";
			for(int a=0; a<pda.shiftActionCount(); a++) {
				const PDA::Action pdaAction(pda.shiftAction(a));
				const CFG::Production prod(pda.production(pdaAction));
				Q_ASSERT(prod.isValid());
				const int pmark(pda.mark(pdaAction));
				Q_ASSERT(pmark>=0);
				const CFG::ProductionInfo & pinfo=cfg.productionInfo(prod);
				Q_ASSERT(pmark<pinfo.size());
				const CFG::Action & cfgAction=pinfo.shift(pmark).action();
				ostream<<
					"\t\t\t\t\t\tcase "<<(a+1)<<":\n"
					"\t\t\t\t\t\t{\n";
				const int minref(cfgAction.minRef());
				if(minref>0)for(int i(pmark); i>=minref-1; --i) {
					const QString t(symtypes.value(pinfo.shift(i).symbol()));
					if(t!="" && t!="void") ostream<<"\t\t\t\t\t\t\tNodeImpl<"<<t<<" > *n"<<(i+1)<<"((NodeImpl<"<<t<<" >*)";
					else ostream<<"\t\t\t\t\t\t\t"<<opt_tokenList<<"::Node *n"<<(i+1)<<"(";
					if(i==pmark)ostream<<"stack.first()";
					else ostream<<"n"<<(i+2)<<"->ref";
					ostream<<");\n";
				}
				ostream<<
					"\t\t\t\t\t\t\tthis->"<<cfgAction.function()<<"(";
				for(int i=0; i<cfgAction.count(); i++) {
					if(i)ostream<<", ";
					const CFG::Arg & arg=cfgAction.arg(i);
					if(arg.isReference()) {
						const int ref(arg.reference());
						if(arg.isMetaReference()) {
							ostream<<"n"<<ref<<"->info";
						} else {
							const QString t(symtypes.value(pinfo.shift(ref-1).symbol()));
							Q_ASSERT(t!=""&&t!="void");
							ostream<<"n"<<ref<<"->value"<<"/*"<<t<<"*/";
						}
					} else {
						ostream<<arg.literal();
					}
				}
				ostream<<
					");\n"
					"\t\t\t\t\t\t\tbreak;\n"
					"\t\t\t\t\t\t}\n";
			}
			ostream<<"\t\t\t\t\t}\n";
		}
		ostream<<
			"\t\t\t\t}\n"
			"\t\t\t} else {\n"
			"\t\t\t\tswitch(act) {\n";
		for(int a=0; a<pda.reduceActionCount(); a++) {
			ostream<<
				"\t\t\t\t\tcase "<<(a+1)<<":\n"
				"\t\t\t\t\t{\n";
			const PDA::Action pdaAction(pda.reduceAction_i(a));
			const CFG::Production prod(pda.production(pdaAction));
			if(prod.id()==cfg.count()) {
				ostream<<
					"\t\t\t\t\t\treturn true;\n"
					"\t\t\t\t\t}\n";
				continue;
			}
			Q_ASSERT(prod.isValid());
			const CFG::ProductionInfo & pinfo=cfg.productionInfo(prod);
			ostream<<"\t\t\t\t\t\t//"<<cfg.toString(pinfo)<<"\n";
			const CFG::Action & cfgAction=pinfo.action();
			const QString rt(symtypes.value(CFG::Symbol(pinfo.leftSide(), false)));
			for(int i(pinfo.size()-1); i>=0; --i) {
				const QString t(symtypes.value(pinfo.shift(i).symbol()));
				if(t!="" && t!="void") ostream<<"\t\t\t\t\t\tNodeImpl<"<<t<<" > *n"<<(i+1)<<"((NodeImpl<"<<t<<" >*)";
				else ostream<<"\t\t\t\t\t\t"<<opt_tokenList<<"::Node *n"<<(i+1)<<"(";
				ostream<<
					"stack.first());\n"
					"\t\t\t\t\t\tstack.removeFirst();\n";
			}
			const int pivot(cfgAction.pivot());
			{
				const int nmark(pinfo.leftSide()+cfg.terminalCount()+1);
				ostream<<"\t\t\t\t\t\t";
				if(!pivot) {
					const QString metaInfoExp(opt_info_type.length()?(pinfo.size()?QString(", n1->info"):QString(", %1::%2()").arg(opt_class).arg(opt_info_type)):QString(""));
					ostream<<"tokens.prepend(new ";
					if(rt!="" && rt!="void") {
						ostream<<"NodeImpl<"<<rt<<" >("<<nmark<<metaInfoExp;
						if(cfgAction.function().size()||cfgAction.count())ostream<<", ";
					} else {
						ostream<<opt_tokenList<<"::Node("<<(pinfo.leftSide()+cfg.terminalCount()+1)<<metaInfoExp<<"));\n";
					}
				}
				bool call(false);
				if(cfgAction.function().size() || (!pivot && rt!="" && rt!="void" && cfgAction.count())) {
					call=true;
					if(cfgAction.function().size())ostream<<"this->"<<cfgAction.function()<<"(";
					else ostream<<rt<<"(";
					for(int i=0; i<cfgAction.count(); i++) {
						if(i)ostream<<", ";
						const CFG::Arg & arg=cfgAction.arg(i);
						if(arg.isReference()) {
							const int ref(arg.reference());
							if(arg.isMetaReference()) {
								ostream<<"n"<<ref<<"->info";
							} else {
								const QString t(symtypes.value(pinfo.shift(ref-1).symbol()));
								Q_ASSERT(t!="" && t!="void");
								ostream<<"n"<<ref<<"->value"<<"/*"<<t<<"*/";
							}
						} else {
							ostream<<arg.literal();
						}
					}
					ostream<<")";
				}
				if(!pivot) {
					if(rt!="" && rt!="void") {
						ostream<<"));\n";
					} else {
						if(call)ostream<<";\n";
					}
				} else {
					if(cfgAction.function().size())ostream<<";\n\t\t\t\t\t\t";
					ostream<<
						"n"<<pivot<<"->mark="<<nmark<<";\n"
						"\t\t\t\t\t\ttokens.prepend(n"<<pivot<<");\n";
				}
			}
			for(int i(pinfo.size()-1); i>=0; --i) {
				if((i+1)!=pivot)ostream<<"\t\t\t\t\t\tdelete n"<<(i+1)<<";\n";
			}
			ostream<<
				"\t\t\t\t\t\tbreak;\n"
				"\t\t\t\t\t}\n";
		}
		ostream<<
			"\t\t\t\t}\n"
			"\t\t\t}\n"
			"\t\t\tif("<<opt_errorFlag<<"())return false;\n"
			"\t\t}\n"
			"\t}\n"
			"\treturn false;\n"
			"}\n"
			"\n";
	}
}

bool Parser::compileGrammer() {
/*	QMap<Parser::Option, QString> options;
	options[OptionClass]=opt_class;
	options[OptionParse]=opt_parse;
	options[OptionTokenList]=opt_tokenList;
	options[OptionError]=opt_error;
	options[OptionErrorFlag]=opt_errorFlag;
	options[OptionLexerState]=opt_lexer_state;
	options[OptionLex]=opt_lex;
	options[OptionInit]=opt_init;
	options[OptionIssue]=opt_issue;
	options[OptionInfoType]=opt_info_type;
	options[OptionInfoFunc]=opt_info_func;*/
	
	Compiler compiler(cfg, symtypes, instances);
	if(!compiler.compile(options[OptionLR1].length())) {
		error(compiler.errors());
		return false;
	}
	compiler.print(ostream, options);
	return true;
}

bool Parser::compile() {
	if(!hfile.open(QFile::WriteOnly|QFile::Text)) {
		error(QString("Cannot open header file '%1' for writing.").arg(hfile.fileName()));
		return false;
	}
	if(!ofile.open(QFile::WriteOnly|QFile::Text)) {
		error(QString("Cannot open cpp file for '%1' writing.").arg(ofile.fileName()));
		hfile.close();
		return false;
	}
	hstream.setDevice(&hfile);
	const QString head(hfile.fileName().replace(QRegExp("\\W"), "_").toUpper());
	hstream<<
		"#ifndef _"<<head<<"\n"
		"#define _"<<head<<"\n"
		"\n";
	
	ostream.setDevice(&ofile);
	foreach(const QString i, includes) {
		ostream<<"#include "<<i<<"\n";
	}
	ostream<<
		"\n"
		"#include \""<<hfile.fileName()<<"\"\n"
		"#include <QStringList>\n"
		"\n";
	
	QMap<QString, int> startConditions;
	startConditions["INITIAL"]=0;
	foreach(const Pattern & pattern, patterns) {
		foreach(const QString & s, pattern.sc) {
			Q_ASSERT(s.length());
			if(startConditions.contains(s))continue;
			const int n(startConditions.size());
			startConditions[s]=n;
		}
	}
	
	if(!compileTokens(startConditions))return false;
	if(!compilePatterns(startConditions))return false;
	if(!compileGrammer())return false;
	
	hstream<<
		"\n"
		"#endif\n";
	
	ostream<<"//EOF\n";
	
	return true;
}

void Parser::setRecorder(Recorder *r) {
	reparser.setRecorder(r);
	rec=r;
}

Parser::Parser():rec(0) {
	resetOptions();
}

//EOF
