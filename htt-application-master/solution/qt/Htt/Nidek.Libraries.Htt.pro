#-------------------------------------------------
#
# Project created by QtCreator 2020-10-29T10:32:37
#
#-------------------------------------------------

QT       -= gui

TARGET = Nidek.Libraries.Htt
TEMPLATE = lib
#CONFIG += staticlib

DEFINES += HTT_LIBRARY KITT=$$CurrentKit:Name

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$PWD/../../../libraries/ImageProcessing/src/
INCLUDEPATH += $$PWD/../../../libraries/ImageProcessing/include/

HEADERS += \
    ../../../libraries/ImageProcessing/include/Htt.h \
    ../../../libraries/ImageProcessing/include/HttGlobal.h \
    ../../../libraries/ImageProcessing/include/HttProcessType.h \
    ../../../libraries/ImageProcessing/include/ModelType.h \
    ../../../libraries/ImageProcessing/include/Statistics.h \
    ../../../libraries/ImageProcessing/src/CustomBevelModel.h \
    ../../../libraries/ImageProcessing/src/Hough.h \
    ../../../libraries/ImageProcessing/src/HttPrivate.h \
    ../../../libraries/ImageProcessing/src/ImagePreprocessing.h \
    ../../../libraries/ImageProcessing/src/JsonUtils.h \
    ../../../libraries/ImageProcessing/src/MiniBevelModel.h \
    ../../../libraries/ImageProcessing/src/MiniBevelModelExt.h \
    ../../../libraries/ImageProcessing/src/Model.h \
    ../../../libraries/ImageProcessing/src/OptimizationEngine.h \
    ../../../libraries/ImageProcessing/src/Profile.h \
    ../../../libraries/ImageProcessing/src/Pso.h \
    ../../../libraries/ImageProcessing/src/ReturnCode.h \
    ../../../libraries/ImageProcessing/src/ScheimpflugTransform.h \
    ../../../libraries/ImageProcessing/src/TBevelModel.h \
    ../../../libraries/ImageProcessing/src/Tester.h \
    ../../../libraries/ImageProcessing/src/Timer.h \
    ../../../libraries/ImageProcessing/src/Version.h


SOURCES += \
    ../../../libraries/ImageProcessing/src/CustomBevelModel.cpp \
    ../../../libraries/ImageProcessing/src/Hough.cpp \
    ../../../libraries/ImageProcessing/src/Htt.cpp \
    ../../../libraries/ImageProcessing/src/HttPrivate.cpp \
    ../../../libraries/ImageProcessing/src/ImagePreprocessing.cpp \
    ../../../libraries/ImageProcessing/src/MiniBevelModel.cpp \
    ../../../libraries/ImageProcessing/src/MiniBevelModelExt.cpp \
    ../../../libraries/ImageProcessing/src/Model.cpp \
    ../../../libraries/ImageProcessing/src/OptimizationEngine.cpp \
    ../../../libraries/ImageProcessing/src/Profile.cpp \
    ../../../libraries/ImageProcessing/src/Pso.cpp \
    ../../../libraries/ImageProcessing/src/ScheimpflugTransform.cpp \
    ../../../libraries/ImageProcessing/src/Statistics.cpp \
    ../../../libraries/ImageProcessing/src/TBevelModel.cpp \
    ../../../libraries/ImageProcessing/src/Tester.cpp \
    ../../../libraries/ImageProcessing/src/Timer.cpp


unix {
    target.path = /usr/lib
    INSTALLS += target
}

## jsoncpp
win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MinGW_32bit/release/ -ljsoncpp
else:win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MinGW_32bit/debug/ -ljsoncpp
else:win32:!win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MSVC2015_64bit/release/ -ljsoncpp
else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/ -ljsoncpp
else:unix:!macx: LIBS += -L$$PWD/../build-jsoncpp/ -ljsoncpp

INCLUDEPATH += $$PWD/../../../ext-dep/jsoncpp/include
DEPENDPATH += $$PWD/../../../ext-dep/jsoncpp/include

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MinGW_32bit/release/libjsoncpp.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MinGW_32bit/debug/libjsoncpp.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MSVC2015_64bit/release/jsoncpp.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../build-jsoncpp-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/jsoncpp.lib
else:unix:!macx: PRE_TARGETDEPS += $$PWD/../build-jsoncpp/libjsoncpp.a

## utils
win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MinGW_32bit/release/ -lNidek.Libraries.Utils
else:win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MinGW_32bit/debug/ -lNidek.Libraries.Utils
else:win32:!win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit/release/ -lNidek.Libraries.Utils
else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/ -lNidek.Libraries.Utils
else:unix:!macx: LIBS += -L$$PWD/../build-Nidek.Libraries.Utils/ -lNidek.Libraries.Utils

INCLUDEPATH += $$PWD/../../../libraries/Utils/include
#DEPENDPATH += $$PWD/../../../libraries/Utils/include

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MinGW_32bit/release/libNidek.Libraries.Utils.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MinGW_32bit/debug/libNidek.Libraries.Utils.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit/release/Nidek.Libraries.Utils.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/Nidek.Libraries.Utils.lib
else:unix:!macx: PRE_TARGETDEPS += $$PWD/../build-Nidek.Libraries.Utils/libNidek.Libraries.Utils.so

unix: DEFINES += UNIX
