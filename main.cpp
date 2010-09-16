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
#include "REParser.h"
#include "Parser.h"

#include <stdio.h>
#include <QCoreApplication>
#include <QTextCodec>
#include "Recorder.h"
#include "Player.h"

namespace {
	void myMessageOutput(QtMsgType type, const char *msg) {
		switch (type) {
			case QtDebugMsg:
				fprintf(stderr, "Debug: %s\n", msg);
				break;
			case QtWarningMsg:
				fprintf(stderr, "Warning: %s\n", msg);
				break;
			case QtCriticalMsg:
				fprintf(stderr, "Critical: %s\n", msg);
				break;
			case QtFatalMsg:
				fprintf(stderr, "Fatal: %s\n", msg);
				*((int*)0)=0;
		}
	}
}

namespace {
	void printUsage(const QString & msg="") {
		QTextStream out(stdout);
		if(msg.size())out<<msg<<"\n";
		out<<"QPG v0.2\n";
		out<<"Usage:\n";
		out<<"\tqpg -h<headerfile> -o<sourcecodefile> <grammerfile>\n";
	}
	
	bool generatePlayer() {
		QFile file("Player.cpp");
		if(!file.open(QFile::WriteOnly|QFile::Text))return false;
		QTextStream ostream(&file);
		ostream<<"#include \"Player.h\"\n#include \"Parser.h\"\n#include \"REParser.h\"\n\n";
		ostream<<"void Player::replay(Parser & parser) {\n\tREParser & reparser=parser.reparser;\n\t";
		Recorder r;
		Parser p;
		p.setRecorder(&r);
		if(!p.parseFile(":/qpg.qpg")) {
			qDebug("Could not read :/qpg.qpg :\n%s", p.lastError().toUtf8().data());
			return false;
		}
		ostream<<r.result()<<"\n}\n\n";
		r.clear();
		ostream<<"void Player::replay_re(Parser & parser) {\n\tREParser & reparser=parser.reparser;\n\t";
 		if(!p.parseFile(":/re.qpg")) {
			qDebug("Could not read :/re.qpg :\n%s", p.lastError().toUtf8().data());
			return false;
		}
		ostream<<r.result()<<"\n}\n\n//EOF\n";
		return true;
	}
	
	bool generateParsersFromSource() {
		{
			Parser p;
			p.setOutputCodeFile("Parser_gen.cpp");
			p.setOutputHeaderFile("Parser_gen.h");
			if(!p.parseFile(":/qpg.qpg")) {
				qDebug("Could not read :/qpg.qpg :\n%s", p.lastError().toUtf8().data());
				return false;
			}
			if(!p.compile()) {
				qDebug("Could not compile :/qpg.qpg :\n%s", p.lastError().toUtf8().data());
				return false;
			}
		}
		{
			Parser p;
			p.setOutputCodeFile("REParser_gen.cpp");
			p.setOutputHeaderFile("REParser_gen.h");
			if(!p.parseFile(":/re.qpg")) {
				qDebug("Could not read :/re.qpg :\n%s", p.lastError().toUtf8().data());
				return false;
			}
			if(!p.compile()) {
				qDebug("Could not compile :/qpg.qpg :\n%s", p.lastError().toUtf8().data());
				return false;
			}
		}
		return true;
	}
	
	bool generateParsers() {
		{
			Parser p;
			p.setOutputCodeFile("Parser_gen.cpp");
			p.setOutputHeaderFile("Parser_gen.h");
			Player::replay(p);
			if(!p.compile()) {
				qDebug("Could not compile replayed :/qpg.qpg :\n%s", p.lastError().toUtf8().data());
				return false;
			}
		}
		{
			Parser p;
			p.setOutputCodeFile("REParser_gen.cpp");
			p.setOutputHeaderFile("REParser_gen.h");
			Player::replay_re(p);
			if(!p.compile()) {
				qDebug("Could not compile replayed :/re.qpg :\n%s", p.lastError().toUtf8().data());
				return false;
			}
		}
		return true;
	}
}

int main(int argc, char **argv) {
	qInstallMsgHandler(myMessageOutput);
	QCoreApplication app(argc, argv);
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	
	QStringList a(QCoreApplication::arguments());
	QString header;
	QString source;
	QString grammer;
	for(int i(1); i<a.size(); i++) {
		const QString arg(a.value(i));
		if(arg=="-b2") {
			if(a.size()!=2) {
				printUsage();
				return -1;
			}
			if(!generatePlayer())return -1;
			return 0;
		} else if(arg=="-b1") {
			if(a.size()!=2) {
				printUsage();
				return -1;
			}
			if(!generateParsers())return -1;
			return 0;
		} else if(arg=="-b3") {
			if(a.size()!=2) {
				printUsage();
				return -1;
			}
			if(!generateParsersFromSource())return -1;
			return 0;
		} else if(arg.startsWith("-h")) {
			if(header.length()) {
				printUsage("Duplicate option -h");
				return -1;
			}
			if(arg.length()>2) {
				header=arg.right(arg.length()-2);
			} else {
				i++;
				if(i==a.size()) {
					printUsage("Missing file name after -h option");
					return -1;
				}
				header=a.value(i);
			}
		} else if(arg.startsWith("-o")) {
			if(source.length()) {
				printUsage("Duplicate option -o");
				return -1;
			}
			if(arg.length()>2) {
				source=arg.right(arg.length()-2);
			} else {
				i++;
				if(i==a.size()) {
					printUsage("Missing file name after -o option");
					return -1;
				}
				source=a.value(i);
			}
		} else if(arg.startsWith("-")) {
			printUsage(QString("Unrecognized option:%1").arg(arg));
			return -1;
		} else {
			if(grammer.length()) {
				printUsage(QString("Unrecognized option:%1").arg(arg));
				return -1;
			}
			grammer=arg;
		}
	}
	if(!header.length() || !source.length() || !grammer.length()) {
		printUsage();
		return -1;
	}
	
	Parser p;

	p.setOutputCodeFile(source);
	p.setOutputHeaderFile(header);
	
	QTextStream err(stderr);
	
	if(!p.parseFile(grammer)) {
		err<<"Parsing of file'"<<grammer<<"'failed:\n"<<p.lastError()<<"\n";
		return -1;
	}
	if(!p.compile()) {
		err<<"Generation of parser failed:\n"<<p.lastError()<<"\n";
		return -1;
	}
	return 0;
}

//EOF
