include (../../../variables.pri)

TEMPLATE = lib
LANGUAGE = C++
TARGET   = enttecdmxusbout

CONFIG      += plugin
QT          += gui core
INCLUDEPATH += ../../interfaces

win32 {
    CONFIG += ftd2xx
}

# FTD2XX is a proprietary interface by FTDI Ltd. and would therefore taint the
# 100% FLOSS codebase of QLC if distributed along with QLC sources. Download
# the latest driver package from http://www.ftdichip.com/Drivers/D2XX.htm and
# follow the instructions on that page for its installation on your platform.
#
# Use normal slashes "/" instead of Windows' backslashes "\" for paths !!!!
#
CONFIG(ftd2xx) {
    win32 {
        FTD2XXDIR    = C:/CDM20812
        LIBS        += -L$$FTD2XXDIR/i386 -lftd2xx
        INCLUDEPATH += $$FTD2XXDIR
    } else {
        LIBS        += -lftd2xx
    }
    DEFINES     += FTD2XX
} else {
    CONFIG      += link_pkgconfig
    PKGCONFIG   += libftdi libusb
}

HEADERS += ../../interfaces/qlcoutplugin.h
HEADERS += enttecdmxusbwidget.h \
           qlcftdi.h \
           enttecdmxusbout.h \
           enttecdmxusbpro.h \
           enttecdmxusbopen.h

SOURCES += enttecdmxusbwidget.cpp \
           enttecdmxusbout.cpp \
           enttecdmxusbpro.cpp \
           enttecdmxusbopen.cpp

CONFIG(ftd2xx) {
    SOURCES += qlcftdi-ftd2xx.cpp
} else {
    SOURCES += qlcftdi-libftdi.cpp
}

unix:!macx {
    QT      += dbus
    DEFINES += DBUS_ENABLED

    # Rules to make ENTTEC devices readable & writable by normal users
    udev.path  = /etc/udev/rules.d
    udev.files = z65-enttec-dmxusb.rules
    INSTALLS  += udev
}

PRO_FILE      = src.pro
TRANSLATIONS += Enttec_DMX_USB_Output_de_DE.ts
TRANSLATIONS += Enttec_DMX_USB_Output_es_ES.ts
TRANSLATIONS += Enttec_DMX_USB_Output_fi_FI.ts
TRANSLATIONS += Enttec_DMX_USB_Output_fr_FR.ts
include(../../../i18n.pri)

# This must be after "TARGET = " and before target installation so that
# install_name_tool can be run before target installation
macx:include(../../../macx/nametool.pri)

# Plugin installation
target.path = $$INSTALLROOT/$$OUTPUTPLUGINDIR
INSTALLS   += target
