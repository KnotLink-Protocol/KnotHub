INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/private
DEPENDPATH += $$PWD

QT += core-private
QUAZIP_HEADERS+= $$PWD/zipcompress.h

HEADERS += \
    $$PWD/private/zipcompress_p.h \
    $$PWD/private/zipcompressthread.h \
    $$PWD/zipcompress.h
SOURCES += \
    $$PWD/private/zipcompress_p.cpp \
    $$PWD/private/zipcompressthread.cpp \
    $$PWD/zipcompress.cpp

