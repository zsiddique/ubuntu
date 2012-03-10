include(../../../variables.pri)

TEMPLATE = app
LANGUAGE = C++
TARGET   = vccuelist_test

QT      += testlib xml gui

INCLUDEPATH += ../../../plugins/interfaces
INCLUDEPATH += ../../../engine/src
INCLUDEPATH += ../../src
DEPENDPATH  += ../../src

QMAKE_LIBDIR += ../../../engine/src
QMAKE_LIBDIR += ../../src
LIBS        += -lqlcengine -lqlcui

# Test sources
SOURCES += vccuelist_test.cpp
HEADERS += vccuelist_test.h
