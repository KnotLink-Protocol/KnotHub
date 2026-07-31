#-------------------------------------------------
#
# Project created by QtCreator 2026-06-14T18:28:10
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = KnotHubCore
TEMPLATE = app

VERSION = 0.2.1.0
QMAKE_TARGET_COMPANY    = HXH
QMAKE_TARGET_PRODUCT    = KnotHubCore
QMAKE_TARGET_DESCRIPTION = KnotHub 服务中枢 — 插件/配方编排引擎
QMAKE_TARGET_COPYRIGHT  = Copyright © 2026 HXH

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


# Windows API（advapi32：服务控制 + 注册表读写）
LIBS += -ladvapi32

# 源码方式使用需要设置为静态库
DEFINES +=   QUAZIP_STATIC
include($$PWD/quazip/3rdparty/zlib.pri)
include($$PWD/quazip/quazip.pri)
include($$PWD/quazip/zipop/zipop.pri)

include($$PWD/KnotLinkLib/KnotLinkLib.pri)

SOURCES += \
        main.cpp \
        knothubcore.cpp \
        daemon.cpp \
    NodeManager/nodemanager.cpp \
    NodeManager/standalonemanager.cpp \
    NodeManager/nodeloader.cpp \
    NodeManager/nodeinstaller.cpp \
    NodeManager/plugininfo.cpp \
    RecipeManager/recipemanager.cpp

HEADERS += \
        knothubcore.h \
        daemon.h \
    NodeManager/nodemanager.h \
    NodeManager/standalonemanager.h \
    NodeManager/nodeloader.h \
    NodeManager/nodeinstaller.h \
    NodeManager/plugininfo.h \
    RecipeManager/recipemanager.h

FORMS += \
        knothubcore.ui
