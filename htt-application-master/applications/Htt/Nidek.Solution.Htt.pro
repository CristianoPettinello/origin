#-------------------------------------------------
#
# Project created by QtCreator 2020-10-28T14:16:10
#
#-------------------------------------------------

QT       += core gui
QT       += charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Htt
TEMPLATE = app

INCLUDEPATH += include

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

FORMS += \
    forms/MainWindow.ui

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/ImageViewer.cpp \
    src/ImageViewerController.cpp \
    src/HttProcessor.cpp \
    src/CalibrationWizard.cpp \
    src/ControlGroupBox.cpp \
#    src/HwController.cpp \
#    src/Drivers/CameraDriver.cpp \
#    src/Drivers/DummyCamera.cpp \
#    src/Drivers/DummyLed.cpp \
#    src/Drivers/LedDriver.cpp \
#    src/Drivers/OnSemiCamera.cpp \
    src/Drivers/HttDriverBridge.cpp \
    src/HwData.cpp \
    src/CalibrationReportGenerator.cpp \
    src/LiveAcquisition.cpp \
    src/FramesController.cpp \
    src/GrabbingGroupBox.cpp \
    src/PeakFinder.cpp

HEADERS += \
    include/MainWindow.h \
    include/ImageViewer.h \
    include/ImageViewerController.h \
    include/MeasuresStateMachine.h \
    include/HttProcessor.h \
    include/HttUtils.h \
    include/CalibrationWizard.h \
    include/ControlGroupBox.h \
    include/Notifications.h \
    include/CalibrationStateMachine.h \
    include/ModalityWork.h \
    include/HwController.h \
#    include/Drivers/CameraDriver.h \
#    include/Drivers/DummyCamera.h \
#    include/Drivers/DummyLed.h \
#    include/Drivers/LedDriver.h \
#    include/Drivers/OnSemiCamera.h \
    include/Drivers/HttDriverBridge.h \
    include/HwData.h \
    include/CalibrationReportGenerator.h \
    include/LiveAcquisition.h \
    include/FramesController.h \
    include/GrabbingGroupBox.h \
    include/PeakFinder.h \
    include/Globals.h

RESOURCES += \
    resources.qrc \
    resources/icons.qrc

INCLUDEPATH += $$PWD/../../ext-dep/jsoncpp/include
INCLUDEPATH += $$PWD/../../ext-dep/libusb1.0/include
INCLUDEPATH += $$PWD/../../ext-dep/CyAPI/include
INCLUDEPATH += $$PWD/../../libraries/ImageProcessing/include
INCLUDEPATH += $$PWD/../../libraries/ImageProcessing/src
INCLUDEPATH += $$PWD/../../libraries/UsbDriver/include
INCLUDEPATH += $$PWD/../../libraries/Utils/include

## Htt library
win32-g++:CONFIG(release, debug|release): LIBS += --L$$PWD/../../solution/qt/build-Nidek.Libraries.Htt-Desktop_Qt_5_9_3_MinGW_32bit/release/ -lNidek.Libraries.Htt
else:win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.Htt-Desktop_Qt_5_9_3_MinGW_32bit/debug/ -lNidek.Libraries.Htt
else:win32:!win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.Htt-Desktop_Qt_5_9_3_MSVC2015_64bit/release/ -lNidek.Libraries.Htt
else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.Htt-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/ -lNidek.Libraries.Htt
else:unix:!macx: LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.Htt -lNidek.Libraries.Htt -lpng -llog4cpp

## jsoncpp
win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../../solution/qt/build-jsoncpp-Desktop_Qt_5_9_3_MinGW_32bit/release/ -ljsoncpp
else:win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../solution/qt/build-jsoncpp-Desktop_Qt_5_9_3_MinGW_32bit/debug/ -ljsoncpp
else:win32:!win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../../solution/qt/build-jsoncpp-Desktop_Qt_5_9_3_MSVC2015_64bit/release/ -ljsoncpp
else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../solution/qt/build-jsoncpp-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/ -ljsoncpp
else:unix:!macx: LIBS += -L$$PWD/../../solution/qt/build-jsoncpp/ -ljsoncpp

## UsbDriver
win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MinGW_32bit/release/ -lNidek.Libraries.UsbDriver
else:win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MinGW_32bit/debug/ -lNidek.Libraries.UsbDriver
else:win32:!win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MSVC2015_64bit/release/ -lNidek.Libraries.UsbDriver
else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/ -lNidek.Libraries.UsbDriver
else:unix:!macx: LIBS += -L$$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver/ -lNidek.Libraries.UsbDriver

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MinGW_32bit/release/libNidek.Libraries.UsbDriver.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MinGW_32bit/debug/libNidek.Libraries.UsbDriver.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MSVC2015_64bit/release/Nidek.Libraries.UsbDriver.dll
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MSVC2015_64bit/debug/Nidek.Libraries.UsbDriver.dll
else:unix:!macx: PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.UsbDriver/libNidek.Libraries.UsbDriver.so

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
else:unix:!macx: PRE_TARGETDEPS += $$PWD/../../solution/qt/build-Nidek.Libraries.Utils/libNidek.Libraries.Utils.so


# DEFINES += LIBUSB
contains(DEFINES, LIBUSB) {
    ## libusb-1.0
    win32-g++: LIBS += -L$$PWD/../../ext-dep/libusb1.0/MS32/dll -llibusb-1.0
    else:win32:!win32-g++: LIBS += -L$$PWD/../../ext-dep/libusb1.0/MS64/dll -llibusb-1.0
    else:unix:!macx: LIBS += -lusb-1.0
} else {
    ## CyAPI
    win32:!win32-g++: LIBS += -L$$PWD/../../ext-dep/CyAPI/lib/x64 -lCyAPI
    win32:!win32-g++: LIBS += -L$$PWD/../../ext-dep/CyAPI/extra/x64 -lSetupAPI
    win32:!win32-g++: LIBS += -L$$PWD/../../ext-dep/CyAPI/extra/x64 -lUser32
    win32:!win32-g++: LIBS += -L$$PWD/../../ext-dep/CyAPI/extra/x64 -llegacy_stdio_definitions
}


unix: DEFINES += UNIX
