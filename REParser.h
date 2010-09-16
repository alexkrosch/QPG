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
#ifndef REPARSER_H
#define REPARSER_H

#include "fa.h"
#include <QMap>

class QString;
class Recorder;
class Player;

class REParser {
	friend class Player;
	public:
		struct TokenList;
		struct LexerState;
		typedef QString String;
		typedef QChar Char;
		typedef FA::StatePair StatePair;
		enum CharClass {Digit=0, NonDigit=1, Whitespace=2, NonWhitespace=3, WordChar=4, NonWordChar=5};
		struct Quantifier {
			int min;
			int max;
			Quantifier():min(1), max(-1) {}
			Quantifier(int n):min(n), max(-1) {
				Q_ASSERT(n>=1);
			}
			Quantifier(int n, int m):min(n), max(m) {
				Q_ASSERT(n>=0);
				Q_ASSERT(m>=n);
				Q_ASSERT(m>0);
			}
		};
private:
		Recorder *rec;
		
		FA fa;
		QMap<QString, FA> defs;
		bool dot;
		FA predefs[6];
		int endmark;
		
		QString buf;
		int pos;
		int currentchar;
		QString errmsg;
		QString issuemsg;
		int nextCharacter();
//		void error(const std::string & str);
		void error(const QString & m);
		void issue(const QString & m);
		bool errorFlag()const;
		bool parse();
		
		bool lex(LexerState & state);
		
		void beginCharacterClass(const QString & token, TokenList & q);
		void endCharacterClass(const QString & token, TokenList & q);
		void pushCharacter(const QString & token, TokenList & q);
		void pushIdentifier(const QString & token, TokenList & q);
		void pushString(const QString & token, TokenList & q);
		void appendQuantifier(const QString & token, TokenList & q);
		
		FA::StatePair reunion(const FA::StatePair & p1, const FA::StatePair & p2);
		FA::StatePair reunion_impl(const FA::StatePair & p1, const FA::StatePair & p2, int rid);
		FA::StatePair recomplement(const FA::StatePair & p1, const FA::StatePair & p2);
		FA::StatePair reconcat_impl(const FA::StatePair & p1, const FA::StatePair & p2, int rid);
		FA::StatePair reconcat(const FA::StatePair & p1, const FA::StatePair & p2);
		FA::StatePair reclosure_impl(const FA::StatePair & p, int rid);
		FA::StatePair reclosure(const FA::StatePair & p);
		FA::StatePair repclosure(const FA::StatePair & p);
		FA::StatePair reoptional_impl(const FA::StatePair & p, int rid);
		FA::StatePair reoptional(const FA::StatePair & p);
		FA::StatePair quantified(const FA::StatePair & p, const Quantifier & q);
		FA::StatePair rechar(ushort c);
		FA::StatePair rerange(ushort c1, ushort c2);
		FA::StatePair rerange_impl(ushort c1, ushort c2, int rid);
		FA::StatePair reclass(bool invert, const FA::StatePair & p);
		FA::StatePair reclass(CharClass c);
		void convert(const FA::StatePair & p);
		
		FA::StatePair reinsert(const QString & id);
		FA::StatePair restring(const QString & s);
		
	public:
		REParser();
		void setRecorder(Recorder *r);
		void setActiveStartConditions(const QSet<QString> & c);
		bool parse(const QString & s, int endmark=0);
		const FA & result()const {return fa;}
		void defineAs(const QString & id);
		bool isDefined(const QString & id)const;
		const QString & lastError()const;
		void clear();
};

#endif
