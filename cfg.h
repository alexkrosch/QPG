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
#ifndef CFG_H
#define CFG_H

#include <QList>
#include <QString>
#include <QMap>
#include <QHash>
#include <QSet>
#include <QVector>

class PDA;

class CFG {
	public:
		class Production;
		class Symbol {
			friend class CFG;
			friend class CFG::Production;
			private:
				bool term;
				int _id;
			public:
				Symbol():term(true), _id(-1) {}
				Symbol(int id, bool t):term(t), _id(id) {}
				bool isTerminal()const {return term;}
				bool isValid()const {return _id>=0;}
				bool operator < (const Symbol & s)const {
					if(!term && s.term)return true;
					if(term && !s.term)return false;
					return _id<s._id;
				}
				bool operator == (const Symbol & s)const {return _id==s._id && term==s.term;}
				qint32 hash()const {return _id;}
		};
		class Arg {
			private:
				QString lit;
				int ref;
				bool meta;
			public:
				int rid;
				Arg():ref(-1), rid(-1) {}
				Arg(const QString & l, int r=-1):lit(l), ref(-1), meta(false), rid(r) {}
				Arg(int r, int i=-1):ref(r), meta(false), rid(i) {}
				Arg(int r, bool m, int i=-1):ref(r), meta(m), rid(i) {}
				bool operator == (const Arg & a)const {return ref==a.ref && lit==a.lit && meta==a.meta;}
				bool operator != (const Arg & a)const {return ref!=a.ref || lit!=a.lit || meta!=a.meta;}
				bool isReference()const {return ref>0;}
				int reference()const {return ref;}
				const QString & literal()const {return lit;}
				bool isMetaReference()const {return meta;}
				qint32 hash()const;
		};
		class Action {
			private:
				QString func;
				QList<Arg> args;
				int piv;
			public:
				int rid;
				Action():piv(0), rid(-1) {}
				Action(const QString & f, const QList<Arg> & a, int r=-1):func(f), args(a), piv(0), rid(r) {}
				bool operator == (const Action & a)const {return func==a.func && args==a.args && piv==a.piv;}
				bool operator != (const Action & a)const {return func!=a.func || args!=a.args || piv!=a.piv;}
				void setPivot(int p) {piv=p;}
				int pivot()const {return piv;}
				int count()const {return args.size();}
				const Arg & arg(int i)const {return args[i];}
				bool isValid()const {return func.length()||piv>0||args.size();}
				const QString & function()const {return func;}
				int maxRef()const;
				int minRef()const;
				qint32 hash()const;
		};
		class Shift {
			private:
				Symbol sym;
				Action act;
			public:
				int rid;
				Shift():rid(-1) {}
				Shift(const Symbol & s, const Action & a, int r=-1):sym(s), act(a), rid(r) {}
				bool operator ==(const Shift & s)const {return sym==s.sym && act==s.act;}
				bool operator !=(const Shift & s)const {return !(sym==s.sym) || act!=s.act;}
				const Action & action()const {return act;}
				const Symbol & symbol()const {return sym;}
				qint32 hash()const;
		};
		class ProductionInfo {
			private:
				int left;
				QList<Shift> right;
				Action _action;
				//QString _flaws;
			public:
				ProductionInfo() {}
				ProductionInfo(const Symbol & l, const QList<Shift> & r, const Action & act):left(l._id), right(r), _action(act) {
					Q_ASSERT(!l.isTerminal());
				}
				//bool check();
				//QString flaws()const {return _flaws;}
				int leftSide()const {return left;}
				int size()const {return right.size();}
				const Shift & shift(int i)const {return right[i];}
				const Action & action()const {return _action;}
				qint32 hash()const;
		};
		class Production {
			private:
				int _id;
			public:
				Production():_id(-1) {}
				Production(int id):_id(id) {}
				bool operator == (const Production & p)const {
					return _id==p._id;
				}
				int id()const {return _id;}
				bool isValid()const {return _id>=0;}
		};
	private:
		QVector<QString> terminals;
		QVector<QString> nonterminals;
		QMap<QString, int> tindex;
		QMap<QString, int> nindex;
		QVector<ProductionInfo> prods;
		QHash<int, QSet<Production> > prodsIndex;
	public:
		Symbol addSymbol(const QString & name, bool terminal);
		Symbol findSymbol(const QString & name)const;
		QString toString(const Symbol & s)const;
		QString toString(const ProductionInfo & pinfo)const;
		Production addProduction(const ProductionInfo & p);
		int count()const;
		const ProductionInfo & productionInfo(Production p)const;
		bool hasProductionsFor(const Symbol & s)const;
		int terminalCount()const;
		int nonterminalCount()const;
		
		QVector<QSet<int> > leftMost()const;
		QSet<Symbol> startSymbols()const;
		
		PDA toPDA(bool lr1)const;
};

qint32 qHash(const CFG::Symbol & s);
qint32 qHash(const CFG::Production & p);
// qint32 qHash(const CFG::Action & a);
// qint32 qHash(const CFG::Shift & s);
// qint32 qHash(const CFG::Arg & a);


#endif
