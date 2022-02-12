@echo off
set ROOT=..
set BUILDDIR=%userprofile%\Desktop\HTT
set APPDIR=%ROOT%\applications\build-Nidek.Solution.Htt-Desktop_Qt_5_9_3_MSVC2015_64bit
set LIBDIR=%ROOT%\solution\qt\

set QTBINPATH=C:\Qt\Qt5.9.3\5.9.3\msvc2015_64\bin
set QTPLUGINSPATH=C:\Qt\Qt5.9.3\5.9.3\msvc2015_64\plugins

@REM Copy configuration files
xcopy %APPDIR%\config %BUILDDIR%\config /d /y

@REM Copy the application and all the dll required
xcopy %APPDIR%\release\Htt.exe %BUILDDIR% /d /y
xcopy %LIBDIR%\build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit\release\Nidek.Libraries.Utils.dll %BUILDDIR% /d /y
xcopy %LIBDIR%\build-Nidek.Libraries.UsbDriver-Desktop_Qt_5_9_3_MSVC2015_64bit\release\Nidek.Libraries.UsbDriver.dll %BUILDDIR% /d /y
xcopy %LIBDIR%\build-Nidek.Libraries.Htt-Desktop_Qt_5_9_3_MSVC2015_64bit\release\Nidek.Libraries.Htt.dll %BUILDDIR% /d /y
xcopy %LIBDIR%\build-Nidek.Libraries.Utils-Desktop_Qt_5_9_3_MSVC2015_64bit\release\Nidek.Libraries.Utils.dll %BUILDDIR% /d /y

@REM Qt dependencies
xcopy %QTBINPATH%\Qt5Charts.dll %BUILDDIR% /d /y
xcopy %QTBINPATH%\Qt5Widgets.dll %BUILDDIR% /d /y
xcopy %QTBINPATH%\Qt5Gui.dll %BUILDDIR% /d /y
xcopy %QTBINPATH%\Qt5Core.dll %BUILDDIR% /d /y
xcopy %QTPLUGINSPATH%\platforms %BUILDDIR%\platforms /d /y

@REM firmware
xcopy %APPDIR%\..\Htt\resources\firmware\htt_cam.img %BUILDDIR%\firmware\htt_cam.img /d /y

@REM DRIVER
xcopy "C:\Program Files (x86)\Cypress\EZ-USB FX3 SDK\1.3\driver" %BUILDDIR%\driver /s /d /y