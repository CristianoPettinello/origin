TEMPLATE	= app
SOURCES		= main.cpp wingetopt.cpp
HEADERS         = wingetpt.h
TARGET		= ../bin/main

INCLUDEPATH += $$PWD/../../ext-dep/jsoncpp/include
INCLUDEPATH += $$PWD/../../libraries/UsbDriver/include
INCLUDEPATH += $$PWD/../../libraries/Utils/include
INCLUDEPATH += $$PWD/../../ext-dep/libusb1.0/include

## jsoncpp
win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../../solution/qt/build-jsoncpp-Desktop_Qt_5_9_3_MinGW_32bit/release/ -ljsoncpp
else:win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../solution/qt/build-jsoncpp-Desktop_Qt_5_9_3_MinGW_32bit/debug/ -ljsoncpp
else:win32:!win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../../solution/qt/build-jsoncpp-Desktop_Qt_5_9_3_MSVC2015_64bit/release/ -ljsoncpp
else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../solution/qt/build-jsoncpp-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/ -ljsoncpp
else:unix:!macx: LIBS += -L$$PWD/../../solution/qt/build-jsoncpp/ -ljsoncpp

## lib UsbDriver
win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MinGW_32bit/release/ -lNidek.Libraries.UsbDriver
else:win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MinGW_32bit/debug/ -lNidek.Libraries.UsbDriver
else:win32:!win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MSVC2015_64bit/release/ -lNidek.Libraries.UsbDriver
else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/ -lNidek.Libraries.UsbDriver
else:unix:!macx: LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver/ -lNidek.Libraries.UsbDriver

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MinGW_32bit/release/Nidek.Libraries.UsbDriver.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MinGW_32bit/debug/Nidek.Libraries.UsbDriver.a
# Using with dynamic library
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MSVC2015_64bit/release/Nidek.Libraries.UsbDriver.dll
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/Nidek.Libraries.UsbDriver.dll
# Using with static library
#else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MSVC2015_64bit/release/Nidek.Libraries.UsbDriver.lib
#else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/Nidek.Libraries.UsbDriver.lib
else:unix:!macx: PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver/Nidek.Libraries.UsbDriver.a

## Utils
win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MinGW_32bit/release/ -lNidek.Libraries.Utils
else:win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MinGW_32bit/debug/ -lNidek.Libraries.Utils
else:win32:!win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit/release/ -lNidek.Libraries.Utils
else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/ -lNidek.Libraries.Utils
else:unix:!macx: LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.Utils/ -lNidek.Libraries.Utils

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MinGW_32bit/release/libNidek.Libraries.Utils.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MinGW_32bit/debug/libNidek.Libraries.Utils.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit/release/Nidek.Libraries.Utils.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/Nidek.Libraries.Utils.lib
else:unix:!macx: PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.Utils/libNidek.Libraries.Utils.a

# Using with static library
#win32:!win32-g++: LIBS += -L$$PWD/../../ext-dep/libusb1.0/MS64/static -llibusb-1.0
# Using with dynamic library
win32:!win32-g++: LIBS += -L$$PWD/../../ext-dep/libusb1.0/MS64/dll -llibusb-1.0
