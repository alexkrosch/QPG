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
#ifndef FA_H
#define FA_H

#include <QHash>
#include <QSet>
#include <QList>
#include <QPair>

class FA {
	public:
		class Element {
			private:
				int _id;
			public:
				Element():_id(-1) {}
				Element(int i):_id(i) {
					Q_ASSERT(i>=0);
				}
				bool isValid()const {return _id>=0;}
				int id()const {return _id;}
				bool operator == (const Element & o)const {return _id==o._id;}
				bool operator != (const Element & o)const {return _id!=o._id;}
				bool operator < (const Element & o)const {return _id<o._id;}
				bool operator <= (const Element & o)const {return _id<=o._id;}
		};
		class State : public Element {
			public:
				State() {}
				State(int s):Element(s) {}
		};
		class Symbol : public Element {
			public:
				Symbol() {}
				Symbol(int s):Element(s) {}
				int value()const {return id();}
		};
		class Mark : public Element {
			public:
				Mark():Element(0) {}
				Mark(int m):Element(m) {}
		};
		struct Range {
			Symbol from;//inclusive
			Symbol to;//exclusive
			Range() {}
			Range(Symbol s):from(s), to(Symbol(s.id()+1)) {}
			Range(Symbol f, Symbol t):from(f), to(t) {
				Q_ASSERT(f<t);
			}
			bool isValid()const {return from.isValid()&&to.isValid();}
		};
		//typedef QPair<State, State> StatePair;
		struct StatePair {
			State first;
			State second;
			int rid;
			StatePair():rid(-1) {
			}
			StatePair(const State & f, const State & s, int r=-1):first(f), second(s), rid(r) {
// 				Q_ASSERT(rid!=345);
			}
			bool operator == (const StatePair & p)const {return first==p.first && second==p.second;}
			bool operator != (const StatePair & p)const {return first!=p.first || second!=p.second;}
		};
	private:
		//int nextState;
		QList<QSet<Mark> > states;
		QList<QPair<Range, QHash<State, State> > > trans;
		State startState;
		QSet<StatePair> etrans;
		int search(Symbol s)const;
		void compress();
	public:
		FA();
		State addState();
		int count()const;
		void addMark(State s, Mark m);
		QSet<Mark> marks(State s)const;
		void removeAllMarks();
		void setStart(State s);
		State start()const;
		bool isDeterministic()const;
		bool addTransition(State from, const Range & on, State to);
		State transition(State from, Symbol on)const;
		QVector<State> transitions(State from)const;
		bool addETransition(State from, State to);
		StatePair insert(const FA & fa, bool keepMarks=true);
		
		int rangeCount()const;
		Range range(int r)const;
		
		FA deterministic()const;
		FA minimal()const;
		
		bool match(const QVector<Symbol> & input)const;
		
		void print()const;
};

qint32 qHash(FA::Element e);
qint32 qHash(FA::Range r);
qint32 qHash(FA::StatePair p);

#endif
