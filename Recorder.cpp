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
#include "Recorder.h"

Recorder::Recorder():nextid(0) {}

int Recorder::uniqueId() {
// 	Q_ASSERT(nextid!=345);
	return nextid++;
}

void Recorder::addLine(const QString & s) {
	lines.append(s);
}

QString Recorder::result()const {
	QStringList l(lines);
//	l.append("");
	return l.join("\n\t");
}

void Recorder::clear() {
	lines.clear();
}

//EOF
