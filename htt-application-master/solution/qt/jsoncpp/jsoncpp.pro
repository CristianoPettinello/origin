#-------------------------------------------------
#
# Project created by QtCreator 2020-10-29T10:39:18
#
#-------------------------------------------------

QT       -= gui

TARGET = jsoncpp
TEMPLATE = lib
CONFIG += staticlib

DEFINES += JSONCPP_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../../../ext-dep/jsoncpp/include/

SOURCES += \
    ../../../ext-dep/jsoncpp/src/lib_json/json_reader.cpp \
    ../../../ext-dep/jsoncpp/src/lib_json/json_value.cpp \
    ../../../ext-dep/jsoncpp/src/lib_json/json_valueiterator.inl \
    ../../../ext-dep/jsoncpp/src/lib_json/json_writer.cpp

HEADERS += \
    ../../../ext-dep/jsoncpp/include/json/allocator.h \
    ../../../ext-dep/jsoncpp/include/json/assertions.h \
    ../../../ext-dep/jsoncpp/include/json/config.h \
    ../../../ext-dep/jsoncpp/include/json/forwards.h \
    ../../../ext-dep/jsoncpp/include/json/json.h \
    ../../../ext-dep/jsoncpp/include/json/json_features.h \
    ../../../ext-dep/jsoncpp/include/json/reader.h \
    ../../../ext-dep/jsoncpp/include/json/value.h \
    ../../../ext-dep/jsoncpp/include/json/version.h \
    ../../../ext-dep/jsoncpp/include/json/writer.h \
    ../../../ext-dep/jsoncpp/src/lib_json/json_tool.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
