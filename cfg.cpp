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
#include "cfg.h"

#include <QLinkedList>
#include <QStringList>
#include "fa.h"
#include "pda.h"

template<typename T>
qint32 qHash(const QList<T> & l) {
	qint32 res(0);
	foreach(const T & t, l) {
		res=qHash(QPair<qint32, qint32>(res, qHash(t)));
	}
	return res;
}

template<typename T>
qint32 qHash(const QSet<T> & s) {
	qint32 res(0);
	foreach(const T & t, s) {
		res=qHash(QPair<qint32, qint32>(res, qHash(t)));
	}
	return res;
}

int CFG::Action::maxRef()const {
	int res(-1);
	foreach(const Arg & a, args) {
		res=qMax(res, a.reference());
	}
	return res;
}

int CFG::Action::minRef()const {
	int res(0);
	foreach(const Arg & a, args) {
		if(!a.isReference())continue;
		if(res<=0) {
			res=a.reference();
		} else {
			res=qMin(res, a.reference());
		}
	}
	return res;
}


CFG::Symbol CFG::addSymbol(const QString & name, bool terminal) {
	int i(0);
	if(terminal) {
		Q_ASSERT(!tindex.contains(name));
		tindex[name]=i=terminals.size();
		terminals.append(name);
	} else {
		Q_ASSERT(!nindex.contains(name));
		nindex[name]=i=nonterminals.size();
		nonterminals.append(name);
	}
	return Symbol(i, terminal);
}

CFG::Symbol CFG::findSymbol(const QString & name)const {
	QMap<QString, int>::const_iterator it(tindex.find(name));
	if(it!=tindex.end()) {
		return Symbol(it.value(), true);
	}
	it=nindex.find(name);
	if(it!=nindex.end()) {
		return Symbol(it.value(), false);
	}
	return Symbol();
}

QString CFG::toString(const Symbol & s)const {
	if(s.isTerminal()) {
		return terminals.value(s._id);
	} else {
		return nonterminals.value(s._id);
	}
}

namespace {
	QString toString(const CFG::Action & a) {
		if(!a.isValid())return "";
		QStringList args;
		for(int i=0; i<a.count(); i++) {
			const CFG::Arg & arg=a.arg(i);
			args.append(arg.isReference()?QString().setNum(arg.reference()):arg.literal());
		}
		return QString(":%1%2(%3)").arg(a.pivot()>0?QString("%1 ").arg(a.pivot()):QString("")).arg(a.function()).arg(args.join(", "));
	}
}

QString CFG::toString(const ProductionInfo & pinfo)const {
	QStringList shifts;
	for(int i=0; i<pinfo.size(); i++) {
		const Shift & s=pinfo.shift(i);
		shifts.append(toString(s.symbol())+::toString(s.action()));
	}
	return QString("%1:(%2)%3").arg(nonterminals.value(pinfo.leftSide())).arg(shifts.join(" ")).arg(::toString(pinfo.action()));
}

CFG::Production CFG::addProduction(const ProductionInfo & p) {
	Q_ASSERT(p.leftSide()<nonterminals.size() && p.leftSide()>=0);
	const int n(prods.size());
	prods.append(p);
	Production res(n);
	prodsIndex[p.leftSide()].insert(res);
	return res;
}

int CFG::count()const {
	return prods.size();
}

const CFG::ProductionInfo & CFG::productionInfo(Production p)const {
	return prods[p.id()];
}

bool CFG::hasProductionsFor(const Symbol & s)const {
	if(s.isTerminal())return false;
	return prodsIndex.value(s._id).size();
}

int CFG::terminalCount()const {
	return terminals.size();
}

int CFG::nonterminalCount()const {
	return nonterminals.size();
}

QVector<QSet<int> > CFG::leftMost()const {
	const int n(nonterminals.size());
	QVector<bool> eps(n, false);
	const int np(prods.size());
	foreach(const ProductionInfo & p, prods) {
		Q_ASSERT(p.leftSide()>=0 && p.leftSide()<eps.size());
		if(!p.size())eps[p.leftSide()]=true;
	}
	QVector<QSet<int> > res(n);
	QVector<QSet<int> > nts(n);
	for(int p=0; p<np; p++) {
		const ProductionInfo & prod=prods[p];
		const int left(prod.leftSide());
		Q_ASSERT(left>=0 && left<n);
		const int ns(prod.size());
		int s(0);
		for(; s<ns; s++) {
			const Symbol sym(prod.shift(s).symbol());
			if(sym.isTerminal()) {
				res[left].insert(sym._id);
				break;
			} else {
				Q_ASSERT(sym._id>=0 && sym._id<n);
				nts[left].insert(sym._id);
				if(!eps[sym._id])break;
			}
		}
		if(s==ns)res[left].insert(-1);
	}
	bool done(false);
	while(!done) {
		done=true;
		for(int i=0; i<n; i++) {
			QSet<int> & pri=res[i];
			const QSet<int> set_nt=nts[i];
			foreach(int nt, set_nt) {
				Q_ASSERT(nt>=0 && nt<n);
				QSet<int> sec(res[nt]);
				sec.remove(-1);
				const int before(pri.size());
				pri.unite(sec);
				if(pri.size()>before)done=false;
			}
		}
	}
	return res;
}

QSet<CFG::Symbol> CFG::startSymbols()const {
	QSet<Symbol> cands;
	const int n(nonterminals.size());
	for(int i=0; i<n; i++)cands.insert(Symbol(i, false));
	const int np(prods.size());
	for(int p=0; p<np; p++) {
		const ProductionInfo & prod=prods[p];
		const int ns(prod.size());
		for(int s=0; s<ns; s++) {
			const Symbol sym(prod.shift(s).symbol());
			if(sym.isTerminal())continue;
			cands.remove(sym);
		}
	}
	return cands;
}

namespace {
	class lr0item {
		private:
			CFG::Production prod;
			int _mark;
		public:
			lr0item():_mark(0) {}
			lr0item(const CFG::Production & p):prod(p), _mark(0) {}
			lr0item(const CFG::Production & p, int m):prod(p), _mark(m) {}
			lr0item next()const {
				return lr0item(prod, _mark+1);
			}
			bool operator == (const lr0item & i)const {
				return _mark==i._mark && prod==i.prod;
			}
			qint32 hash()const {
				return qHash(QPair<CFG::Production, int>(prod, _mark));
			}
			int mark()const {return _mark;}
			CFG::Production production()const {return prod;};
	};
	
	qint32 qHash(const lr0item & i) {
		return i.hash();
	}
	
	class lr1item {
		private:
			lr0item _core;
			QSet<int> ahead;
			lr1item(const lr0item & c, const QSet<int> & a):_core(c), ahead(a) {}
		public:
			lr1item() {}
			lr1item(const CFG::Production & p, const QSet<int> & a):_core(p), ahead(a) {}
			lr1item next()const {
				return lr1item(_core.next(), ahead);
			}
			bool operator == (const lr1item & i)const {
				return _core==i._core && ahead==i.ahead;
			}
			int mark()const {return _core.mark();}
			CFG::Production production()const {return _core.production();};
			const QSet<int> lookAheadSet()const {return ahead;}
			const lr0item & core()const {return _core;}
			qint32 hash()const {
				return qHash(QPair<qint32, qint32>(qHash(_core), qHash(ahead)));
			}
			lr1item united(const QSet<int> & la)const {
				lr1item res(*this);
				res.ahead.unite(la);
				return res;
			}
	};

	qint32 qHash(const lr1item & i) {
		return i.hash();
	}

}

PDA CFG::toPDA(bool lr1)const {
	QSet<Symbol> start_cands(startSymbols());
	if(start_cands.size()!=1) {
		return PDA();
	}
	const Symbol start_org(*(start_cands.begin()));
	QSet<int> ahead;
	ahead.insert(terminals.size());
	
	QVector<QSet<int> > leftmost(leftMost());
	
	//QHash<lr0item, QSet<int> > lookAheadSets;
	
	const QVector<ProductionInfo> prods(QVector<ProductionInfo>(this->prods)<<ProductionInfo(Symbol(nonterminals.size(), false), QList<Shift>()<<Shift(start_org, Action()), Action("*", QList<Arg>())));
	//lr1item first(Production(Symbol(nonterminals.size(), false), QList<Shift>()<<Shift(start_org, Action()), Action("*", QList<Arg>())), ahead);
	const lr1item first(Production(prods.size()-1), ahead);
	QLinkedList<lr1item> items;
	items.push_back(first);
		
	//QHash<lr0item, QSet<int> > lr0item2la;
	QVector<QSet<int> > prod2la(prods.size());
	QVector<QSet<Production> > ancestors(prods.size());
	
	int itemcnt(0);
	
	{
		QSet<lr1item> visitedItems;
		visitedItems.insert(first);
		while(items.size()) {
			const lr1item curItem(items.front());
			//lr0item2la[curItem.core()].unite(curItem.lookAheadSet());
			prod2la[curItem.core().production().id()].unite(curItem.lookAheadSet());
			Q_ASSERT(visitedItems.contains(curItem));
			items.pop_front();
			const ProductionInfo & pinfo=prods[curItem.production().id()];
			Q_ASSERT(curItem.mark()<=pinfo.size());
			if(curItem.mark()>=pinfo.size())continue;
			const Symbol curSym(pinfo.shift(curItem.mark()).symbol());
			if(!curSym.isTerminal()) {
				QSet<int> la;
				la.insert(-1);
				int m(curItem.mark()+1);
				while(m<pinfo.size() && la.contains(-1)) {
					la.remove(-1);
					const Symbol sym(pinfo.shift(m).symbol());
					if(sym.isTerminal()) {
						la.insert(sym._id);
					} else {
						la.unite(leftmost[sym._id]);
						Q_ASSERT(prodsIndex.contains(sym._id));
					}
					m++;
				}
				const bool inherits(la.contains(-1));
				if(inherits) {
					la.remove(-1);
					la.unite(curItem.lookAheadSet());
				}
				
				const QSet<Production> & dsts=prodsIndex[curSym._id];
				foreach(const Production & dst, dsts) {
					if(inherits)ancestors[dst.id()].insert(curItem.core().production());
					const lr1item dstItem(dst, la);
					QSet<lr1item>::const_iterator it(visitedItems.find(dstItem));
					if(it==visitedItems.end()) {
						visitedItems.insert(dstItem);
						items.push_back(dstItem);
					}
				}
			}
			const lr1item dstItem(curItem.next());
			Q_ASSERT(!visitedItems.contains(dstItem));
			visitedItems.insert(dstItem);
			items.push_back(dstItem);
		}
	}
	
	PDA pda;
	
	for(int pass(0); pass<2; ++pass) {
		bool ok(true);
		
		FA fa;
		items.clear();
		items.push_back(first);
		QVector<lr1item> state2item;
		QHash<lr1item, FA::State> item2state;
		item2state[first]=fa.addState();
		state2item.append(first);
		
		itemcnt=0;
		while(items.size()) {
			const lr1item curItem(items.front());
			//const lr1item lalr1Item(curItem.united( lr0item2la[lr0item(curItem.core().production())] ));
			Q_ASSERT(item2state.contains(curItem));
			const FA::State curState(item2state[curItem]);
			/*Q_ASSERT(item2state.contains(lalr1Item));
			const FA::State curState(item2state[lalr1Item]);*/
			items.pop_front();
			const ProductionInfo & pinfo=prods[curItem.production().id()];
			Q_ASSERT(curItem.mark()<=pinfo.size());
			if(curItem.mark()>=pinfo.size()) {
				continue;
			}
			const Symbol curSym(pinfo.shift(curItem.mark()).symbol());
			if(!curSym.isTerminal()) {
				QSet<int> la;
				la.insert(-1);
				int m(curItem.mark()+1);
				while(m<pinfo.size() && la.contains(-1)) {
					la.remove(-1);
					const Symbol sym(pinfo.shift(m).symbol());
					if(sym.isTerminal()) {
						la.insert(sym._id);
					} else {
						la.unite(leftmost[sym._id]);
					}
					m++;
				}
				if(la.contains(-1)) {
					la.remove(-1);
					la.unite(curItem.lookAheadSet());
				}
				
				const QSet<Production> & dsts=prodsIndex[curSym._id];
				foreach(const Production & dst, dsts) {
					QSet<int> laset(la);
					//laset.unite(lr0item2la[lr0item(dst)]);
					laset.unite(prod2la[dst.id()]);
					const lr1item dstItem(dst, laset);
					FA::State dstState;
					QHash<lr1item, FA::State>::const_iterator it(item2state.find(dstItem));
					if(it==item2state.end()) {
						dstState=fa.addState();
						fa.addMark(dstState, FA::Mark(dstState.id()));
						item2state[dstItem]=dstState;
						Q_ASSERT(dstState.id()==state2item.size());
						state2item.append(dstItem);
						items.push_back(dstItem);
						//items.push_back(lr1item(dst, la));
					} else {
						dstState=it.value();
					}
					fa.addETransition(curState, dstState);
				}
			}
			const lr1item dstItem(curItem.next());
			Q_ASSERT(!item2state.contains(dstItem));
			const FA::State dstState(item2state[dstItem]=fa.addState());
			//Q_ASSERT(!item2state.contains(lalr1Item.next()));
			//const FA::State dstState(item2state[lalr1Item.next()]=fa.addState());
			Q_ASSERT(dstState.id()==state2item.size());
			state2item.append(dstItem);
			fa.addMark(dstState, FA::Mark(dstState.id()));
			items.push_back(dstItem);
			fa.addTransition(curState, FA::Range(FA::Symbol(curSym._id+(curSym.isTerminal()?0:terminals.size()+1))), dstState);
		}
		
		fa=fa.deterministic();
		
		pda=PDA();
		pda.setFA(fa);
		
		for(int i(0); i<fa.count(); ++i) {
			const FA::State s(i);
			const QSet<FA::Mark> marks(fa.marks(s));
			foreach(FA::Mark m, marks) {
				const lr1item & item=state2item[m.id()];
				const ProductionInfo & pinfo=prods[item.production().id()];
				if(pinfo.size()==item.mark()) {
					const QSet<int> & ahead=item.lookAheadSet();
					foreach(int sym, ahead) {
						const PDA::Action act(pda.reduceAction(state2item[m.id()].core().production()));
						pda.addLookAheadAction(s, sym, act);
					}
				} else {
					const lr0item & core=item.core();
					const Symbol on(prods[core.production().id()].shift(core.mark()).symbol());
					PDA::Action act;
					if(pinfo.shift(item.mark()).action().isValid()) {
						act=pda.shiftAction(core.production(), core.mark());
					} else {
						pda.addNonAction(s, on._id+(on.isTerminal()?0:terminals.count()+1), pda.shift(core.production(), core.mark()));
					}
					pda.addLookAheadAction(s, on._id+(on.isTerminal()?0:terminals.count()+1), act);
				}
			}
		}
		
		if(!pass) {
			bool done(false);
			for(int i(0); i<prods.size(); ++i)ancestors[i].insert(Production(i));
			while(!done) {
				done=true;
				for(int i(0); i<prods.size(); ++i) {
					QSet<Production> & out=ancestors[i];
					const QSet<Production> in(out);
					foreach(const Production & p, in) {
						const int before(out.size());
						out.unite(ancestors[p.id()]);
						if(before!=out.size())done=false;
					}
				}
			}
			
			for(int i(0); i<fa.count(); ++i) {
				const FA::State s(i);
				for(int t(0); t<terminals.count(); ++t) {
					const QSet<PDA::Action> actions(pda.lookAheadActions(s, t));
					if(actions.size()>1) {
						ok=false;
						foreach(PDA::Action act, actions) {
							if(act.isShiftAction())continue;
							if(!act.isValid())continue;
							const CFG::Production prod(pda.production(act));
							//const lr0item item(prod);//, prods[prod.id()].size());
							//Q_ASSERT(lr0item2la.contains(item));
							//lr0item2la[item].remove(t);
							
							const QSet<Production> & a=ancestors[prod.id()];
							foreach(const Production & p, a)prod2la[p.id()].remove(t);
						}
					}
				}
			}
		}
		
		if(ok || !lr1)break;
	}
	return pda;
	
}

qint32 qHash(const CFG::Symbol & s) {
	return s.hash();
}

qint32 qHash(const CFG::Production & p) {
	return p.id();
}

//EOF
