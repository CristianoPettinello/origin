QT       -= gui

TARGET = Nidek.Libraries.UsbDriver
TEMPLATE = lib
#CONFIG += staticlib

DEFINES += USB_DRIVER_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    ../../../libraries/UsbDriver/src/HttDriver.cpp \
    ../../../libraries/UsbDriver/src/UsbDriver.cpp

HEADERS += \
    ../../../libraries/UsbDriver/include/HttDriver.h \
    ../../../libraries/UsbDriver/include/HttDriverGlobal.h \
    ../../../libraries/UsbDriver/include/UsbDriver.h \
    ../../../libraries/UsbDriver/include/UsbReturnCode.h

INCLUDEPATH += $$PWD/../../../libraries/UsbDriver/include
INCLUDEPATH += $$PWD/../../../libraries/Utils/include
INCLUDEPATH += $$PWD/../../../ext-dep/libusb1.0/include
INCLUDEPATH += $$PWD/../../../ext-dep/CyAPI/include

## Utils
win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MinGW_32bit/release/ -lNidek.Libraries.Utils
else:win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MinGW_32bit/debug/ -lNidek.Libraries.Utils
else:win32:!win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit/release/ -lNidek.Libraries.Utils
else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/ -lNidek.Libraries.Utils
else:unix:!macx: LIBS += -L$$PWD/../build-Nidek.Libraries.Utils/ -lNidek.Libraries.Utils

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MinGW_32bit/release/libNidek.Libraries.Utils.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MinGW_32bit/debug/libNidek.Libraries.Utils.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit/release/Nidek.Libraries.Utils.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/Nidek.Libraries.Utils.lib
else:unix:!macx: PRE_TARGETDEPS += $$PWD/../build-Nidek.Libraries.Utils/libNidek.Libraries.Utils.so

# DEFINES += LIBUSB
contains(DEFINES, LIBUSB) {
    ## libusb
    # Using with static library
    #win32:!win32-g++: LIBS += -L$$PWD/../../../ext-dep/libusb1.0/MS64/static -llibusb-1.0

    # Using with dynamic library
    win32-g++: LIBS += -L$$PWD/../../../ext-dep/libusb1.0/MS32/dll -llibusb-1.0
    win32:!win32-g++: LIBS += -L$$PWD/../../../ext-dep/libusb1.0/MS64/dll -llibusb-1.0
} else {
    ## CyAPI
    win32:!win32-g++: LIBS += -L$$PWD/../../../ext-dep/CyAPI/lib/x64 -lCyAPI
    win32:!win32-g++: LIBS += -L$$PWD/../../../ext-dep/CyAPI/extra/x64 -lSetupAPI
    win32:!win32-g++: LIBS += -L$$PWD/../../../ext-dep/CyAPI/extra/x64 -lUser32
    win32:!win32-g++: LIBS += -L$$PWD/../../../ext-dep/CyAPI/extra/x64 -llegacy_stdio_definitions
}


unix: DEFINES += UNIX
