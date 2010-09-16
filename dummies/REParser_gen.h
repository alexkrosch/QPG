#ifndef _REPARSER_GEN_H
#define _REPARSER_GEN_H

class QString;

struct REParser::TokenList {
	enum SC {
		SC_INITIAL,
		SC_cc
	};
	void appendCCBEGIN(bool) {}
	void appendCCEND() {}
	void appendCHARACTER(int) {}
	void appendIDENTIFIER(const QString &) {}
	void appendSTRING(const QString &) {}
	void appendQUANTIFIER(REParser::Quantifier) {}
	void setStartCondition(SC sc) {}
};

#endif
