QT += core
QT -= gui

TARGET = gendict
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    ../../gendict.cpp \
    ../../paradigm.cpp

HEADERS += \
    ../../paradigm.h

INCLUDEPATH += \
    ../../
