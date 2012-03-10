include(../../variables.pri)

TEMPLATE = lib
LANGUAGE = C++
TARGET   = artnetout

INCLUDEPATH += ../interfaces
CONFIG      += plugin
QT          += xml
unix:CONFIG += link_pkgconfig
unix:PKGCONFIG += libartnet

# Input
HEADERS += artnetthread.h \
    configureartnetoutdialog.h \
    artnetout.h

FORMS += configureartnetoutdialog.ui

SOURCES += artnetthread.cpp \
    configureartnetoutdialog.cpp \
    artnetout.cpp

HEADERS += ../interfaces/qlcoutplugin.h

PRO_FILE = artnetout.pro

target.path = $$INSTALLROOT/$$OUTPUTPLUGINDIR
INSTALLS   += target
