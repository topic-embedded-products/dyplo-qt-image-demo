#-------------------------------------------------
#
# Project created by QtCreator 2020-04-20T15:58:10
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dyplo-qt-image-demo
TEMPLATE = app

# Add dependency on libdyplo
CONFIG += link_pkgconfig
PKGCONFIG += dyplo

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    dyploimageprocessor.cpp

HEADERS += \
        mainwindow.h \
    dyploimageprocessor.h

FORMS += \
        mainwindow.ui

RESOURCES += \
    resources.qrc

target.path = /usr/bin
INSTALLS += target
