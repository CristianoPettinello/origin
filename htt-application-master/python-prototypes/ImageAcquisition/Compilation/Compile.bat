@echo off

:: Activate a proper Python environmente
::call activate pyenv37
call ../env/Scripts/Activate ::git-bash command

cd ..
@echo Current directory:  %cd%

SET sourceFolder=

@echo Clearing all output content...
::del /q "Compilation\Output\*.*"
rm -rf Compilation/Output/*
PAUSE

@echo Compiling the application...
:: use -w params in order to remove debug shell
pyinstaller --uac-admin --onedir --icon=Applications/UserApplication/Icons/NIDEK.ico --name="HTT_ImagingApplication" Applications/UserApplication/Application.py -w

@echo Moving build folders...
move dist Compilation/Output/
move build Compilation/Output/
move "HTT_ImagingApplication.spec" "Compilation/Output/HTTImagingApplication.spec"

@echo Copy of Config folder...
xcopy "Applications/UserApplication/Config" "Compilation/Output/dist/HTT_ImagingApplication/Config" /I

@echo Copy of Calibration folder...
xcopy "Applications/UserApplication/Calibration/kit1" "Compilation/Output/dist/HTT_ImagingApplication/Calibration/kit1" /I


PAUSE
