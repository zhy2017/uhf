#-------------------------------------------------
#
# Project created by QtCreator 2018-01-17T20:09:34
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = uhf
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp \
    mytumbler.cpp \
    progressbarround.cpp \
    myPushButton.cpp \
    SendTask.cpp \
    amplifierwidget.cpp \
    clickedlabel.cpp

HEADERS  += widget.h \
    mytumbler.h \
    progressbarround.h \
    mypushbutton.h \
    SendTask.h \
    amplifierwidget.h \
    clickedlabel.h

FORMS    += widget.ui \
    amplifierwidget.ui

RESOURCES += \
    images.qrc
