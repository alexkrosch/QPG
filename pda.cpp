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
#include "pda.h"

void PDA::setFA(const FA & f) {
	Q_ASSERT(f.isDeterministic());
	//Q_ASSERT(n>f.range(f.rangeCount()-1).to.id());
	nsym=f.range(f.rangeCount()-1).to.id();
	if(!f.isDeterministic())return;
	_fa=f;
	actions.clear();
	shiftActions.clear();
	shiftActionsIndex.clear();
	reduceActions.clear();
	reduceActionsIndex.clear();
}

PDA::Action PDA::shiftAction(CFG::Production prod, int mark) {
	const QPair<CFG::Production, int> p(prod, mark);
	const QHash<QPair<CFG::Production, int>, int>::const_iterator it(shiftActionsIndex.find(p));
	int res(-1);
	if(it==shiftActionsIndex.end()) {
		shiftActionsIndex[p]=res=shiftActions.size();
		shiftActions.append(p);
	} else {
		res=it.value();
	}
	return Action(true, res);
}

PDA::Action PDA::reduceAction(CFG::Production p) {
	const QHash<CFG::Production, int>::const_iterator it(reduceActionsIndex.find(p));
	int res(-1);
	if(it==reduceActionsIndex.end()) {
		reduceActionsIndex[p]=res=reduceActions.size();
		reduceActions.append(p);
	} else {
		res=it.value();
	}
	return Action(false, res);
}

PDA::Shift PDA::shift(CFG::Production prod, int mark) {
	const QPair<CFG::Production, int> p(prod, mark);
	const QHash<QPair<CFG::Production, int>, int>::const_iterator it(shiftsIndex.find(p));
	int res(-1);
	if(it==shiftsIndex.end()) {
		shiftsIndex[p]=res=shifts.size();
		shifts.append(p);
	} else {
		res=it.value();
	}
	return Shift(res);
}

void PDA::addLookAheadAction(FA::State s, int sym, Action a) {
	Q_ASSERT(s.isValid() && s.id()<_fa.count());
//	Q_ASSERT(a.isValid());
	Q_ASSERT(sym>=0 && sym<nsym);
	const int index(s.id()*nsym+sym);
	actions[index].insert(a);
}

QSet<PDA::Action> PDA::lookAheadActions(FA::State s, int sym)const {
	Q_ASSERT(s.isValid() && s.id()<_fa.count());
	Q_ASSERT(sym>=0 && sym<nsym);
	const int index(s.id()*nsym+sym);
	return actions.value(index);
}

void PDA::addNonAction(FA::State s, int sym, Shift shift) {
	Q_ASSERT(s.isValid() && s.id()<_fa.count());
	Q_ASSERT(sym>=0 && sym<nsym);
	const int index(s.id()*nsym+sym);
	nonactions[index].insert(shift);
}

QSet<PDA::Shift> PDA::nonActions(FA::State s, int sym)const {
	Q_ASSERT(s.isValid() && s.id()<_fa.count());
	Q_ASSERT(sym>=0 && sym<nsym);
	const int index(s.id()*nsym+sym);
	return nonactions.value(index);
}

bool PDA::isValid()const {
	return _fa.count() && _fa.rangeCount();
}

CFG::Production PDA::production(const Action & a)const {
	if(a.shift)return shiftActions[a._id].first;
	else return reduceActions[a._id];
}

int PDA::mark(const Action & a)const {
	if(a.shift)return shiftActions[a._id].second;
	else return -1;
}

CFG::Production PDA::production(Shift s)const {
	return shifts[s._id].first;
}

int PDA::mark(Shift s)const {
	return shifts[s._id].second;
}

const FA & PDA::fa()const {
	return _fa;
}

int PDA::shiftActionCount()const {
	return shiftActions.size();
}

PDA::Action PDA::shiftAction(int i)const {
	Q_ASSERT(i>=0&&i<shiftActions.size());
	if(i<0||i>=shiftActions.size())return Action();
	return Action(true, i);
}

int PDA::reduceActionCount()const {
	return reduceActions.size();
}

PDA::Action PDA::reduceAction_i(int i)const {
	Q_ASSERT(i>=0&&i<reduceActions.size());
	if(i<0||i>=reduceActions.size())return Action();
	return Action(false, i);
}

qint32 qHash(const PDA::Action & a) {
	return a.hash();
}

qint32 qHash(const PDA::Shift & s) {
	return s.hash();
}

//EOF
