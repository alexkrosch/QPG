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
#ifndef PARSER_H
#define PARSER_H

#include <QByteArray>

#include "fa.h"
#include "cfg.h"
#include "REParser.h"

#include <QStringList>
#include <QSet>
#include <QMap>
#include <QLinkedList>
#include <QFile>
#include <QTextStream>

class Recorder;
class Player;

class Parser {
	friend class Player;
	public:
		enum Option {
			OptionClass=0,
			OptionParse=1,
			OptionTokenList=2,
			OptionError=3,
			OptionErrorFlag=4,
			OptionLexerState=5,
			OptionLex=6,
			OptionInit=7,
			OptionIssue=8,
			OptionInfoType=9,
			OptionInfoFunc=10,
			OptionNext=11,
			OptionStringType=12,
			OptionCharType=13,
			OptionDump=14,
			OptionLR1=15,
			OptionMax=15
		};
		
		//QString opt_next_char;
		//QString opt_string_type;
		//QString opt_char_type;
		//QString opt_dump;
		
		struct TokenList;
		struct LexerState;
		typedef QString String;
		typedef QChar Char;
		typedef CFG::Shift Shift;
		typedef CFG::Action Action;
		typedef CFG::Arg Arg;
		template<typename T>
		struct Cont {
			typedef QSet<T> Set;
			typedef QList<T> List;
		};
	private:
		int line1;
		int line2;
	public:
		typedef int TokenInfo;
		int tokenInfo() {
			int res(line1);
			line1=line2;
			return res;
		}
	private:
		
		Recorder *rec;
		
		QString buf;
		int pos;
		QString errmsg;
		QStringList issues;
		
		
		REParser reparser;
		QStringList includes;
		struct Pattern {
			QSet<QString> sc;
			QString re;
			QString func;
			int line;
			FA fa;
			Pattern() {}
			Pattern(const QSet<QString> & s, const QString & r, const QString & f, int l):sc(s), re(r), func(f), line(l) {}
		};
		QLinkedList<Pattern> patterns;
		QMap<QString, int> anonPatterns;
		
		CFG cfg;
		QMap<CFG::Symbol, QString> symtypes;
		QMap<CFG::Symbol, QString> instances;
		CFG::Symbol currentProd;
		
		QFile hfile;
		QTextStream hstream;
		QFile ofile;
		QTextStream ostream;
		
		QString hfilename;
		QString ofilename;
		QString options[OptionMax+1];
		
		int nextCharacter();
		bool lex(LexerState & state);
		void error(const QString & m);
		void error(int line, const QString & m);
		void issue(const QString & m);
		bool parse();
		
		void dump(const QString &, TokenList &)const {}
		bool errorFlag()const;

		void appendString(const QString & token, TokenList & q)const;
		void appendIdentifier(const QString & token, TokenList & q)const;
		void appendInteger(const QString & token, TokenList & q)const;
		void appendSigned(const QString & token, TokenList & q)const;
		void appendFloat(const QString & token, TokenList & q)const;
		void appendRegExp(const QString & token, TokenList & q)const;

		template<typename T>
		QSet<T> createSet(const T & t)const {
			QSet<T> res;
			res.insert(t);
			return res;
		}
		template<typename T>
		void insert(QSet<T> & s, const T & t)const {
			s.insert(t);
		}
		template<typename T>
		QList<T> createList(const T & t)const {
			QList<T> res;
			res<<t;
			return res;
		}
		template<typename T>
		void append(QList<T> & l, const T & t)const {
			l<<t;
		}

		void include(const QString & filename);
		void setOption(const QString & option, const QString & value, int line);
		void define(const QString & id, const QString & regexp, int line_id=-1, int line_re=-1);
		void addPattern(const QSet<QString> & startConditions, const QString & regexp, const QString & func, int line=-1);
		void addAnonPattern(const QSet<QString> & startConditions, const QString & regexp, const QString & inst, int line=-1);
		void defineTerminal(const QString & name, int line=-1);
		void defineTerminal(const QString & name, const QString & type, int line=-1);
		void defineTerminal(const QString & name, const QString & type, const QString & inst, int line=-1);
		void declareProduction(const QString & leftHandSide, const QString & type, int line=0);
		void addRightHandSide(const QList<CFG::Shift> & shifts, const CFG::Action & act, int line=0);
		CFG::Shift createShift(const QString & sym, const CFG::Action & act);
		QString lookupAnonPattern(const QString & regexp, int line=0);
		CFG::Action createAction(const QList<CFG::Arg> & args);
		CFG::Action createAction(const QString & func, const QList<CFG::Arg> & args);
		CFG::Action createAction(int pivot, const CFG::Action & act);
		CFG::Arg createArg(const QString & literal);
		CFG::Arg createArg(int ref);
		CFG::Arg createArgMeta(int ref);
		QString constType(const QString & type);
		QString pointerType(const QString & type, bool c);
		QString basicType(const QString & name, const QList<QString> & args);
		QString functionType_rec(const QList<QString> & s1)const;
		QString functionType_rec(const QString & s1, const QList<QString> & s2)const;
		QString functionType(const QString & s1, const QList<QString> & s3)const;
		QString functionType(const QString & s1, const QString & s3, const QList<QString> & s4)const;
		QString templateType(const QString & s1, const QString & s2, const QList<QString> & s3)const;
		QString templateType(const QString & s1, const QList<QString> & s2)const;
		QString unsignedType(const QString & p)const;
		QString nsname(const QString & n1, const QString & n2)const;
		QString nameLiteral(const QString & n)const;

	public:
		bool parseFile(const QString & s);
		void setOutputCodeFile(const QString & filename);
		void setOutputHeaderFile(const QString & filename);
	private:
		bool parsePatterns();
		void resetOptions();
		bool compileTokens(const QMap<QString, int> & startConditions);
		bool compilePatterns(const QMap<QString, int> & startConditions);
		bool compileGrammer();
	public:
		bool compile();
		void setRecorder(Recorder *r);
		
		Parser();
		
		const QString & lastError()const {return errmsg;}
};

#endif
