#ifndef _PARSER_GEN_H
#define _PARSER_GEN_H

class QString;

struct Parser::TokenList {
	void appendSTRING(const QString &, int) {}
	void appendIDENTIFIER(const QString &, int) {}
	void appendINTEGER(int, int) {}
	void appendSIGNED(const QString &, int) {}
	void appendFLOAT(const QString &, int) {}
	void appendREGEXP(const QString &, int) {}
};

#endif
