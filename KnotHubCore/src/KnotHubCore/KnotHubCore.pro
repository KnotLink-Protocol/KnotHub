#-------------------------------------------------
#
# Project created by QtCreator 2026-06-14T18:28:10
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = KnotHubCore
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


# 源码方式使用需要设置为静态库
DEFINES +=   QUAZIP_STATIC
include($$PWD/quazip/3rdparty/zlib.pri)
include($$PWD/quazip/quazip.pri)
include($$PWD/quazip/zipop/zipop.pri)

include($$PWD/KnotLinkLib/KnotLinkLib.pri)

SOURCES += \
        main.cpp \
        knothubcore.cpp \
    NodeManager/nodemanager.cpp \
    NodeManager/nodeloader.cpp \
    NodeManager/nodeinstaller.cpp

HEADERS += \
        knothubcore.h \
    NodeManager/nodemanager.h \
    NodeManager/nodeloader.h \
    NodeManager/nodeinstaller.h

FORMS += \
        knothubcore.ui
