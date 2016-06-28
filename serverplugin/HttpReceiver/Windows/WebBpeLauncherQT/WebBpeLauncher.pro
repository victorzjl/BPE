#-------------------------------------------------
#
# Project created by QtCreator 2016-03-01T22:27:51
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = WebBpeLauncher
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    servicewidget.cpp \
    delbutton.cpp \
    serviceconfigdialog.cpp \
    widgetcontainer.cpp \
    service.cpp \
    serviceconfigmanager.cpp

HEADERS  += mainwindow.h \
    servicewidget.h \
    delbutton.h \
    serviceconfigdialog.h \
    widgetcontainer.h \
    service.h \
    serviceconfig.h \
    serviceconfigmanager.h

FORMS    += mainwindow.ui \
    servicewidget.ui \
    serviceconfigdialog.ui

RESOURCES += \
    pictures.qrc

RC_FILE += \
    WebBpeLauncher.rc

DISTFILES += \
    WebBpeLauncher.rc
