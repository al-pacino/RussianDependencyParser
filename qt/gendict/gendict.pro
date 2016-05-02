QT += core
QT -= gui

TARGET = gendict
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    ../../src/gendict.cpp \
    ../../src/paradigm.cpp

HEADERS += \
    ../../src/paradigm.h

INCLUDEPATH += \
    ../../include \
    ../../src
