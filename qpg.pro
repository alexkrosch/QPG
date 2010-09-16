
TEMPLATE = app
TARGET = 
DEPENDPATH += . dummies
INCLUDEPATH += . dummies
CONFIG += debug


# Input
HEADERS += fa.h REParser.h Parser.h Parser_gen.h cfg.h pda.h Recorder.h Player.h utils.h
SOURCES += main.cpp fa.cpp REParser.cpp Parser.cpp Parser_gen.cpp cfg.cpp pda.cpp Recorder.cpp Player.cpp utils.cpp

HEADERS += REParser_gen.h
SOURCES += REParser_gen.cpp

RESOURCES += grammers.qrc