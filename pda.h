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
#ifndef PDA_H
#define PDA_H

#include "fa.h"
#include "cfg.h"

class PDA {
	public:
		class Action {
			friend class PDA;
			private:
				bool shift;
				int _id;
				Action(bool s, int id):shift(s), _id(id) {}
			public:
				Action():shift(true), _id(-1) {}
				bool isValid()const {return _id>=0;}
				bool isShiftAction()const {return shift;}
				bool operator == (const Action & a)const {return shift==a.shift && _id==a._id;}
				bool operator != (const Action & a)const {return shift!=a.shift || _id!=a._id;}
				qint32 hash()const {return _id;}
				int id()const {return _id;}
		};
		class Shift {
			friend class PDA;
			private:
				int _id;
				Shift(int id):_id(id) {}
			public:
				Shift():_id(-1) {}
				bool isValid()const {return _id>=0;}
				bool operator == (const Shift & a)const {return _id==a._id;}
				bool operator != (const Shift & a)const {return _id!=a._id;}
				qint32 hash()const {return _id;}
				int id()const {return _id;}
		};
	private:
		FA _fa;
		int nsym;
		QHash<int, QSet<Action> > actions;
		QHash<int, QSet<Shift> > nonactions;
		
		QVector<QPair<CFG::Production, int> > shiftActions;
		QHash<QPair<CFG::Production, int>, int> shiftActionsIndex;
		QVector<CFG::Production> reduceActions;
		QHash<CFG::Production, int> reduceActionsIndex;

		QVector<QPair<CFG::Production, int> > shifts;
		QHash<QPair<CFG::Production, int>, int> shiftsIndex;
	public:
		void setFA(const FA & fa);
		Action shiftAction(CFG::Production p, int mark);
		Action reduceAction(CFG::Production p);
		Shift shift(CFG::Production p, int mark);
		void addLookAheadAction(FA::State s, int sym, Action a);
		QSet<Action> lookAheadActions(FA::State s, int sym)const;
		void addNonAction(FA::State s, int sym, Shift shift);
		QSet<Shift> nonActions(FA::State s, int sym)const;
		bool isValid()const;
		CFG::Production production(const Action & a)const;
		int mark(const Action & a)const;
		CFG::Production production(Shift s)const;
		int mark(Shift s)const;
		const FA & fa()const;
		
		int shiftActionCount()const;
		Action shiftAction(int i)const;
		int reduceActionCount()const;
		Action reduceAction_i(int i)const;
};

qint32 qHash(const PDA::Action & a);
qint32 qHash(const PDA::Shift & s);

#endif
