QT += core widgets gui
CONFIG += c++17

TARGET = AutomataSimulator
TEMPLATE = app

SOURCES += main.cpp \
           nfa.cpp \
           dfa.cpp \
           tokenizer.cpp \
           pda.cpp \
           mainwindow.cpp

HEADERS += nfa.h \
           dfa.h \
           tokenizer.h \
           pda.h \
           mainwindow.h

# No additional libraries needed; using Qt's built-in features