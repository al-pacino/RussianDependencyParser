QT += core
QT -= gui

TARGET = main
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    ../../src/paradigm.cpp \
    ../../src/model.cpp \
    ../../src/main.cpp

HEADERS += \
    ../../src/paradigm.h \
    ../../src/model.h

INCLUDEPATH += \
    ../../include \
    ../../src
