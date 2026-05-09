QT       += core gui widgets

TARGET   = lab4
TEMPLATE = app
CONFIG  += c++17

SOURCES += \
    source/gui/main.cpp \
    source/gui/mainwindow.cpp

HEADERS += \
    source/gui/mainwindow.h \
    source/dsa.h

INCLUDEPATH += source
