#ifndef UTILS_H
#define UTILS_H

class QString;

namespace qpg {
	QString unescape(const QString & s);
	QString escape(const QString & s);
	QString escape_c(const QString & s);
}

#endif
