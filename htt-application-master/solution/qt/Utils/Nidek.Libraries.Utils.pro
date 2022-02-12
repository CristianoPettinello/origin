QT       -= gui

TARGET = Nidek.Libraries.Utils
TEMPLATE = lib

#CONFIG += staticlib
DEFINES += UTILS_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    ../../../libraries/Utils/src/Image.cpp \
    ../../../libraries/Utils/src/Log.cpp \
    ../../../libraries/Utils/src/Settings.cpp \
    ../../../libraries/Utils/src/Utils.cpp

HEADERS += \
    ../../../libraries/Utils/include/Image.h \
    ../../../libraries/Utils/include/Log.h \
    ../../../libraries/Utils/include/Settings.h \
    ../../../libraries/Utils/include/UtilsGlobal.h \
    ../../../libraries/Utils/include/Utils.h

INCLUDEPATH += ../../../libraries/Utils/include/
INCLUDEPATH += ../../../ext-dep/jsoncpp/include

## jsoncpp
win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MinGW_32bit/release/ -ljsoncpp
else:win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MinGW_32bit/debug/ -ljsoncpp
else:win32:!win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MSVC2015_64bit/release/ -ljsoncpp
else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/ -ljsoncpp
else:unix:!macx: LIBS += -L$$PWD/../build-jsoncpp/ -ljsoncpp -lpng -llog4cpp

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MinGW_32bit/release/libjsoncpp.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MinGW_32bit/debug/libjsoncpp.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MSVC2015_64bit/release/jsoncpp.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/jsoncpp.lib
else:unix:!macx: PRE_TARGETDEPS += $$PWD/../build-jsoncpp/libjsoncpp.a

unix: DEFINES += UNIX
