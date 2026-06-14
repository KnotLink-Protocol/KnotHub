CONFIG += c++11
TEMPLATE = lib
TARGET = quazip
CONFIG += shared
DEFINES +=   QUAZIP_BUILD
include($$PWD/3rdparty/zlib.pri)
include($$PWD/quazip.pri)
include($$PWD/filezip/filezip.pri)
linux {
    QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN/'"
}
headerfiles.files = $$QUAZIP_HEADERS
####修改此处 想要拷贝到的库路径和头文件路径
sdkPath=$$PWD/../quazipSdk/
####
headerfiles.path = $$sdkPath/include
DESTDIR = $$sdkPath/lib
INSTALLS += headerfiles
