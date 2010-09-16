#include "utils.h"

#include <QString>

namespace {
	bool isOctalDigit(ushort c) {
		return c>='0'&&c<='7';
	}
}

namespace qpg {
	QString unescape(const QString & in) {
		// 		qDebug("QString unescape(const QString & in:%s)", in.toUtf8().data());
		QString res("");
		const int n(in.length());
		int left(0);
		int right(0);
		while(right<n) {
			if(in.at(right)==QChar('\\')) {
				if(left<right) {
					res+=in.mid(left, right-left);
				}
				Q_ASSERT(right+1<in.length());
				ushort c(in.at(++right).unicode());
				switch(c) {
					case '0':
					{
						c=in.at(right+1).unicode();
						if(isOctalDigit(c)) {
							left=right+1;
							right+=3;
							c=(ushort)in.mid(left, right-left+1).toInt(0, 8);
						} else {
							c=0;
						}
						break;
					}
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					{
						left=right;
						for(int i=0; i<2; i++) {
							if(right+1>=n)break;
							c=in.at(right+1).unicode();
							if(isOctalDigit(c)) {
								right++;
							} else {
								break;
							}
						}
						c=(ushort)in.mid(left, right-left+1).toInt(0, 8);
						break;
					}
					case 'a':
						c='\a';
						break;
					case 'b':
						c='\b';
						break;
					case 't':
						c='\t';
						break;
					case 'n':
						c='\n';
						break;
					case 'v':
						c='\v';
						break;
					case 'f':
						c='\f';
						break;
					case 'r':
						c='\r';
						break;
					case 'u':
					case 'x':
						// 						qDebug("\tUNICODE-Escape!:%s", in.mid(right+1, 4).toUtf8().data());
						c=(ushort)in.mid(right+1, 4).toInt(0, 16);
						right+=4;
						break;
				}
				res+=c;
				left=right+1;
			}
			right++;
		}
		if(left<right) {
			res+=in.mid(left, right-left);
		}
		return res;
	}
	
	QString escape(QString s) {
		return s.replace(QChar('\\'), "\\\\").replace(QChar('"'), "\\\"");
	}
	
	const char *escseq[]={
		"\\0"  , "\\001", "\\002", "\\003", "\\004", "\\005", "\\006", "\\a",
		"\\b"  , "\\t",   "\\n",   "\\v",   "\\f",   "\\r",   "\\016", "\\017",
		"\\020", "\\021", "\\022", "\\023", "\\024", "\\025", "\\026", "\\027",
		"\\030", "\\031", "\\032", "\\033", "\\034", "\\035", "\\036", "\\037"
	};
		
	QString escape_c(const QString & s) {
		QString res;
		const int n(s.length());
		for(int i(0); i<n; ++i) {
			const QChar c(s.at(i));
			if(c.unicode()<32) {
				res+=escseq[c.unicode()];
			} else {
				switch(c.unicode()) {
					case '\\':
						res+="\\\\";
						break;
					case '"':
						res+="\\\"";
						break;
					default:
						res+=c;
				}
			}
		}
		return res;
	}
}

//EOF
