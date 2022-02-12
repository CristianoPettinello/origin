import sys
import os
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QMessageBox
from PyQt5.QtCore import QSettings, pyqtSignal, QFile, QByteArray, QJsonDocument
from win32api import GetSystemMetrics
from PIL.ImageQt import ImageQt
from datetime import datetime
import serial
import serial.tools.list_ports
from time import sleep
import logging

import Applications.UserApplication.ApplicationMainWindow_ui as AppMainWindow
from Libraries.CameraSoftwareManager.CameraSoftwareManager import *

import numpy as np
import pyHtt as Htt
import imageio
from PIL import Image
import statistics
import sys, traceback
import faulthandler

########################
###    Parameters    ###
########################
APP_VERSION                 = "01.00.03"
CONFIG_FILE_PATH            = "Config/config.ini"
SERIAL_READ_PERIOD          = 5
APP_LOGGING_LEVEL           = logging.DEBUG
APP_LOGGING_FOLDER          = "Logs"
APP_LOGGING_FILE            = "app.log"
DEFAULT_SETTINGS_FILE_PATH  = "Config/defaultSettings.json"
SETTINGS_FILE_PATH          = "Config/settings.json"
LIB_VERSION                 = Htt.getVersion()
########################


##############################
###    Global Variables    ###
##############################
# Main application
global app
##############################


# Application parameters
class HTTImagingParameters:
    # Camera software
    CameraSoftwareFolderPath = "C:/Program Files (x86)/C3842C"
    # Tracer
    TracerPortCOM           = 'COM1'
    TracerPortBitsPerSecond = 38400
    TracerPortDataBits      = 8
    TracerPortParity        = 'N'
    TracerPortStopBits      = 1
    TracerPortFlowControl   = None
    # Illumination
    IlluminationPortCOM                   = 'COM2'
    IlluminationPortBitsPerSecond         = 115200
    IlluminationPortDataBits              = 8
    IlluminationPortParity                = 'N'
    IlluminationPortStopBits              = 1
    IlluminationPortFlowControl           = None
    IlluminationMaximumLEDPower           = 1000
    IlluminationMaximumContinuousLEDPower = 600
    IlluminationLEDPower                  = 200
    # Acquisition
    AcquisitionSavingFolder = "C:/temp"
    AcquisitionLEDOnTime    = 5
    AcquisitionLEDOnDelay   = 0



class SerialPortSettingChoices:
    BitsPerSecondChoices = []
    DataBitsChoices      = []
    ParityChoices        = []
    StopBitsChoices      = []
    FlowControlChoices   = []


# Main application
class HTTImagingManagerApp(QtWidgets.QMainWindow, AppMainWindow.Ui_MainWindow):


    # Signal to be used to get data received from the tracer
    tracer_data_ready = pyqtSignal(bytes)
    # Signal to be used to get data received from the illumination peripheral
    illumination_data_ready = pyqtSignal(bytes)


    def __init__(self):

        faulthandler.enable()

        # Initialization starts
        self.initialization = True
        
        # Calibration flag
        self.calibrationState = False

        logging.info("###########################################")
        logging.info("##########   Application Start")
        logging.info("##########   v" + APP_VERSION + " - Image Processing Library v" + LIB_VERSION)
        logging.info("###########################################")

        QtWidgets.QMainWindow.__init__(self)
        AppMainWindow.Ui_MainWindow.__init__(self)
        # UI setup
        self.setupUi(self)

        # Event-handler connections
        self.pushButtonCameraSoftwareLaunch.clicked.connect(self.pushButtonCameraSoftwareLaunch_clicked)
        self.pushButtonCameraSoftwareClose.clicked.connect(self.pushButtonCameraSoftwareClose_clicked)
        self.pushButtonManualGrab.clicked.connect(self.pushButtonManualGrab_clicked)
        self.pushButtonFrameProgressiveRestart.clicked.connect(self.pushButtonFrameProgressiveRestart_clicked)
        self.pushButtonImageSavingFolderBrowse.clicked.connect(self.pushButtonImageSavingFolderBrowse_clicked)
        self.pushButtonExecutableFolderBrowse.clicked.connect(self.pushButtonExecutableFolderBrowse_clicked)
        self.lineEditSessionName.textChanged.connect(self.lineEditSessionName_textChanged)
        self.pushButtonTracerConnect.clicked.connect(self.pushButtonTracerConnect_clicked)
        self.pushButtonTracerDisconnect.clicked.connect(self.pushButtonTracerDisconnect_clicked)
        self.pushButtonIlluminationConnect.clicked.connect(self.pushButtonIlluminationConnect_clicked)
        self.pushButtonIlluminationDisconnect.clicked.connect(self.pushButtonIlluminationDisconnect_clicked)
        self.tabWidgetConnectionConfiguration.currentChanged.connect(self.tabWidgetConnectionConfiguration_currentChanged)
        self.spinBoxIlluminationPower.valueChanged.connect(self.spinBoxIlluminationPower_valueChanged)
        self.checkBoxIlluminationContinuouslyOn.stateChanged.connect(self.checkBoxIlluminationContinuouslyOn_stateChanged)
        self.spinBoxLEDOnTime.valueChanged.connect(self.spinBoxLEDOnTime_valueChanged)
        self.spinBoxLEDOnDelay.valueChanged.connect(self.spinBoxLEDOnDelay_valueChanged)
        self.pushButtonSettingsReset.clicked.connect(self.pushButtonSettingsReset_clicked)
        self.comboBoxCalibrationFrame.currentIndexChanged.connect(self.comboBoxCalibrationFrame_currentIndexChanged)
        self.pushButtonCalibration.clicked.connect(self.pushButtonCalibration_clicked)
        self.pushButtonValidation.clicked.connect(self.pushButtonValidation_clicked)
        self.checkBoxValidationRepetitions.stateChanged.connect(self.checkBoxValidationRepetitions_stateChanged)
        self.spinBoxValidationRepetitions.valueChanged.connect(self.spinBoxValidationRepetitions_valueChanged)
        self.checkBoxProcessingImageRepetitions.stateChanged.connect(self.checkBoxProcessingImageRepetitions_stateChanged)
        self.spinBoxRepetitions.valueChanged.connect(self.spinBoxRepetitions_valueChanged)
        self.checkBoxCheckIntersectionsCalibration.stateChanged.connect(self.common_jsonValueChanged)
        self.spinBoxLossFildMarginCalibration.valueChanged.connect(self.common_jsonValueChanged)
        self.checkBoxCheckIntersectionsValidation.stateChanged.connect(self.common_jsonValueChanged)
        self.spinBoxLossFildMarginValidation.valueChanged.connect(self.common_jsonValueChanged)
        ## Settings
        self.comboBoxChannelSelected.currentIndexChanged.connect(self.common_jsonValueChanged)
        self.comboBoxModelSelected.currentIndexChanged.connect(self.common_jsonValueChanged)
        self.doubleSpinBoxRightBoundary.valueChanged.connect(self.common_jsonValueChanged)
        self.doubleSpinBoxUpperBoundary.valueChanged.connect(self.common_jsonValueChanged)
        self.doubleSpinBoxLeftBoundary.valueChanged.connect(self.common_jsonValueChanged)
        self.doubleSpinBoxLowerBoundary.valueChanged.connect(self.common_jsonValueChanged)
        self.doubleSpinBoxProfileOffset.valueChanged.connect(self.common_jsonValueChanged)
        self.spinBoxNIterations.valueChanged.connect(self.common_jsonValueChanged)
        self.spinBoxNParticles.valueChanged.connect(self.common_jsonValueChanged)
        self.spinBoxOptimizationLossFieldMargin.valueChanged.connect(self.common_jsonValueChanged)
        self.spinBoxSearchSpaceWinSizeX.valueChanged.connect(self.common_jsonValueChanged)
        self.spinBoxSearchSpaceWinSizeY.valueChanged.connect(self.common_jsonValueChanged)
        self.doubleSpinBoxSearchSpaceMinAlpha.valueChanged.connect(self.common_jsonValueChanged)
        self.doubleSpinBoxSearchSpaceMaxAlpha.valueChanged.connect(self.common_jsonValueChanged)
        self.doubleSpinBoxSearchSpaceMinMMultiplier.valueChanged.connect(self.common_jsonValueChanged)
        self.doubleSpinBoxSearchSpaceMaxMMultiplier.valueChanged.connect(self.common_jsonValueChanged)
        self.pushButtonLoadTestImage.clicked.connect(self.pushButtonLoadTestImage_clicked)
        self.pushButtonClearTestImage.clicked.connect(self.pushButtonClearTestImage_clicked)
        self.pushButtonProcessTestImage.clicked.connect(self.pushButtonProcessTestImage_clicked)
        self.checkBoxTestImageRepetitions.stateChanged.connect(self.checkBoxTestImageRepetitions_stateChanged)
        self.spinBoxTestImageRepetitions.valueChanged.connect(self.spinBoxTestImageRepetitions_valueChanged)
        self.pushButtonExportProcessingTestImage.clicked.connect(self.pushButtonExportProcessingTestImage_clicked)

        # Show application version on the title
        self.setWindowTitle(self.windowTitle() + " - v" + APP_VERSION + " - Image Processing Library v" + LIB_VERSION)

        # Set application window icon
        self.setWindowIcon(QtGui.QIcon(":/Icons/Icons/NIDEK.ico"))

        self.labelTracerCommandIcon.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)
        self.labelProcessingIcon.setAttribute(QtCore.Qt.WA_TranslucentBackground, True)

        # Initialize frame view
        self.scene = QtWidgets.QGraphicsScene()
        self.scene.setSceneRect(0, 0, C3842_FRAME_WIDTH, C3842_FRAME_WIDTH)
        pxm = QtGui.QPixmap(C3842_FRAME_WIDTH, C3842_FRAME_HEIGHT)
        pxm.fill(QtGui.QColor("black"))
        self.scene.addItem(QtWidgets.QGraphicsPixmapItem(pxm))
        self.graphicsViewImage.setScene(self.scene)
        # self.graphicsViewImage.scale(0.8, 0.8)

        # Initialize serial port setting combo boxes
        self.serialPortSettingChoices = self.createSerialPortSettingChoices()
        # Tracer
        self.comboBoxTracerPortBitsPerSecond.clear()
        self.comboBoxTracerPortBitsPerSecond.addItems([str(x) for x in self.serialPortSettingChoices.BitsPerSecondChoices])
        self.comboBoxTracerPortDataBits.clear()
        self.comboBoxTracerPortDataBits.addItems([str(x) for x in self.serialPortSettingChoices.DataBitsChoices])
        self.comboBoxTracerPortParity.clear()
        self.comboBoxTracerPortParity.addItems(self.serialPortSettingChoices.ParityChoices)
        self.comboBoxTracerPortStopBits.clear()
        self.comboBoxTracerPortStopBits.addItems([str(x) for x in self.serialPortSettingChoices.StopBitsChoices])
        self.labelTracerPortFlowControl.setHidden(True)
        self.comboBoxTracerPortFlowControl.setHidden(True)
        self.comboBoxTracerCOMPort.clear()
        self.comboBoxTracerCOMPort.addItems([x.device for x in serial.tools.list_ports.comports()])
        # Illumination
        self.comboBoxIlluminationPortBitsPerSecond.clear()
        self.comboBoxIlluminationPortBitsPerSecond.addItems([str(x) for x in self.serialPortSettingChoices.BitsPerSecondChoices])
        self.comboBoxIlluminationPortDataBits.clear()
        self.comboBoxIlluminationPortDataBits.addItems([str(x) for x in self.serialPortSettingChoices.DataBitsChoices])
        self.comboBoxIlluminationPortParity.clear()
        self.comboBoxIlluminationPortParity.addItems(self.serialPortSettingChoices.ParityChoices)
        self.comboBoxIlluminationPortStopBits.clear()
        self.comboBoxIlluminationPortStopBits.addItems([str(x) for x in self.serialPortSettingChoices.StopBitsChoices])
        self.labelIlluminationPortFlowControl.setHidden(True)
        self.comboBoxIlluminationPortFlowControl.setHidden(True)
        self.comboBoxIlluminationCOMPort.clear()
        self.comboBoxIlluminationCOMPort.addItems([x.device for x in serial.tools.list_ports.comports()])
        # Settings
        self.comboBoxChannelSelected.clear()
        self.comboBoxChannelSelected.addItem('Red', 0)
        self.comboBoxChannelSelected.addItem('Green', 1)
        self.comboBoxChannelSelected.addItem('Blue', 2)
        self.groupBoxSearchSpace.setVisible(False)
        self.comboBoxModelSelected.clear()
        self.comboBoxModelSelected.addItem('MINI_BEVEL', 0)
        self.comboBoxModelSelected.addItem('T_BEVEL', 1)
        self.comboBoxModelSelected.addItem('CUSTOM_BEVEL', 2)
        # Calibration
        self.comboBoxCalibrationFrame.clear()
        self.spinBoxValidationRepetitions.setEnabled(self.checkBoxValidationRepetitions.isChecked())

        # Move the UI to right
        V_MARG = 80
        scr_w = GetSystemMetrics(0)
        scr_h = GetSystemMetrics(1)
        app_w = scr_w / 2
        app_h = scr_h - V_MARG
        app_x = scr_w / 2
        app_y = int(V_MARG / 2)
        self.setGeometry(app_x, app_y, app_w, app_h)


        # Reset the connection states
        self.cam_software_result_code = HTTCameraResultCode.HTT_CAM_GENERIC_ERROR
        self.cam_software_connected   = False
        self.tracer_connected         = False
        self.illumination_connected   = False
        # Load the configuration
        self.parameters = HTTImagingParameters()
        ret, self.parameters = self.loadConfig()
        if not ret:
            QMessageBox.critical(self,
                                 'Problem with configuration loading',
                                 "Unable to load all, or some, information of the configuration file, or configuration file path is wrong.",
                                 QMessageBox.Ok)
            self.close()
        # Load JSON settings
        file = SETTINGS_FILE_PATH if os.path.exists(SETTINGS_FILE_PATH) else DEFAULT_SETTINGS_FILE_PATH
        ret, self.jsonSettings = self.loadJsonSettings(file)
        if not ret:
            QMessageBox.critical(self,
                                 'Problem with json settings loading',
                                 "Unable to load all, or some, information of the configuration file, or configuration file path is wrong.",
                                 QMessageBox.Ok)

        # Update the UI with the configuration information
        # Processing Image section
        self.checkBoxProcessingImageRepetitions.setChecked(False)
        self.spinBoxRepetitions.setEnabled(self.checkBoxProcessingImageRepetitions.isChecked())
        # Executable folder
        self.lineEditExecutableFolder.setText(self.parameters.CameraSoftwareFolderPath)
        # Image saving folder
        self.lineEditImageSavingFolder.setText(self.parameters.AcquisitionSavingFolder)
        # Tracer
        self.comboBoxTracerCOMPort.setCurrentIndex(self.comboBoxTracerCOMPort.findText(self.parameters.TracerPortCOM, QtCore.Qt.MatchFixedString))
        self.comboBoxTracerPortBitsPerSecond.setCurrentIndex(self.comboBoxTracerPortBitsPerSecond.findText(str(self.parameters.TracerPortBitsPerSecond)))
        self.comboBoxTracerPortDataBits.setCurrentIndex(self.comboBoxTracerPortDataBits.findText(str(self.parameters.TracerPortDataBits)))
        self.comboBoxTracerPortParity.setCurrentIndex(self.comboBoxTracerPortParity.findText(self.parameters.TracerPortParity))
        self.comboBoxTracerPortStopBits.setCurrentIndex(self.comboBoxTracerPortStopBits.findText(str(self.parameters.TracerPortStopBits)))
        # Illumination
        self.comboBoxIlluminationCOMPort.setCurrentIndex(self.comboBoxIlluminationCOMPort.findText(self.parameters.IlluminationPortCOM, QtCore.Qt.MatchFixedString))
        self.comboBoxIlluminationPortBitsPerSecond.setCurrentIndex(self.comboBoxIlluminationPortBitsPerSecond.findText(str(self.parameters.IlluminationPortBitsPerSecond)))
        self.comboBoxIlluminationPortDataBits.setCurrentIndex(self.comboBoxIlluminationPortDataBits.findText(str(self.parameters.IlluminationPortDataBits)))
        self.comboBoxIlluminationPortParity.setCurrentIndex(self.comboBoxIlluminationPortParity.findText(self.parameters.IlluminationPortParity))
        self.comboBoxIlluminationPortStopBits.setCurrentIndex(self.comboBoxIlluminationPortStopBits.findText(str(self.parameters.IlluminationPortStopBits)))
        self.spinBoxIlluminationPower.setMaximum(self.parameters.IlluminationMaximumLEDPower)
        self.labelIlluminationPower.setText("Power [0-"+str(self.parameters.IlluminationMaximumLEDPower)+"]:")
        self.spinBoxIlluminationPower.setValue(self.parameters.IlluminationLEDPower)
        self.spinBoxLEDOnTime.setValue(self.parameters.AcquisitionLEDOnTime)
        self.spinBoxLEDOnDelay.setValue(self.parameters.AcquisitionLEDOnDelay)
        # Settings
        self.visualizeJsonSettings()
        self.pushButtonClearTestImage.setEnabled(False)
        self.pushButtonProcessTestImage.setEnabled(False)
        self.pushButtonExportProcessingTestImage.setEnabled(False)
        # Calibration
        self.calibrationState = self.jsonSettings["scheimpflug_transformation_calibration"]["calibrated"].toBool()
        self.comboBoxCalibrationFrameChoices = self.createCalibrationFrameChoices()
        self.comboBoxCalibrationFrame.addItems([str(x) for x in self.comboBoxCalibrationFrameChoices])
        self.comboBoxCalibrationFrame.setCurrentIndex(self.comboBoxCalibrationFrame.findText(self.jsonSettings["scheimpflug_transformation_calibration"]["current"].toString()))
        self.updateCalibrationFrameParams()
        self.calibrationFrameName = self.jsonSettings["scheimpflug_transformation"]["frame"].toString()
        self.lineEditCalibrationEpsilon.setText(str(self.jsonSettings["scheimpflug_transformation_calibration"]["epsilon"].toDouble()))
        self.spinBoxCalibrationMaxRetries.setValue(self.jsonSettings["scheimpflug_transformation_calibration"]["maxRetries"].toInt())
        self.checkBoxCheckIntersectionsCalibration.setChecked(self.jsonSettings["optimization"]["checkIntersectionsCalibration"].toBool())
        self.spinBoxLossFildMarginCalibration.setValue(self.jsonSettings["optimization"]["lossFieldMarginCalibration"].toInt())
        self.checkBoxCheckIntersectionsValidation.setChecked(self.jsonSettings["optimization"]["checkIntersectionsValidation"].toBool())
        self.spinBoxLossFildMarginValidation.setValue(self.jsonSettings["optimization"]["lossFieldMarginValidation"].toInt())

        # Clean state labels
        self.labelGrabbingState.setText("")
        self.labelCameraSoftwareState.setText("")
        self.labelCameraSoftwareState.setText("")
        self.labelCameraSoftwareState.setText("")
        self.labelLEDInfo.setText("")
        self.labelSettingsState.setText("")
        self.labelCalibrationState.setText("")
        self.updateCalibrationState()
        self.createCalibrationTextOutput(self.calibrationState)

        # Start camera software, by virtually clicking the launching button
        self.cam_software_manager = HTTCameraManager()
        self.pushButtonCameraSoftwareLaunch.click()

        # Create the timer for tracer command icon
        self.tracer_command_label_timer = QtCore.QTimer()
        self.tracer_command_label_timer.setSingleShot(True)
        self.tracer_command_label_timer.timeout.connect(self.tracerCommandLabelTimer_timeout)

        # Connect to the tracer, by virtually clicking the connection button
        self.tracer_serial = serial.Serial()
        self.tracer_read_timer = QtCore.QTimer()
        self.tracer_read_timer.timeout.connect(self.tracerReadTimerTimeout)
        self.tracer_data_ready.connect(self.tracerCommandCaught)
        self.pushButtonTracerConnect.click()

        # Connect to the illumination peripheral, by virtually clicking the connection button
        self.illumination_serial = serial.Serial()
        self.illumination_read_timer = QtCore.QTimer()
        self.illumination_read_timer.timeout.connect(self.illuminationReadTimerTimeout)
        self.pushButtonIlluminationConnect.click()

        # Create the timer for grabbing label
        self.grab_label_timer = QtCore.QTimer()
        self.grab_label_timer.setSingleShot(True)
        self.grab_label_timer.timeout.connect(self.grabLabelTimer_timeout)

        # Create the timer for settings label
        self.settings_label_timer = QtCore.QTimer()
        self.settings_label_timer.setSingleShot(True)
        self.settings_label_timer.timeout.connect(self.settingsLabelTimerTimeout)

        # Create the timer for calibration label
        self.calibration_label_timer = QtCore.QTimer()
        self.calibration_label_timer.setSingleShot(True)
        self.calibration_label_timer.timeout.connect(self.calibrationLabelTimerTimeout)

        # Initialize the tracer command icon
        self.labelTracerCommandIcon.setPixmap(QtGui.QPixmap(":/Icons/Icons/Black-circle-small.png"))

        # Initialize the processing icon
        self.labelProcessingIcon.setPixmap(QtGui.QPixmap(":/Icons/Icons/Black-circle-small.png"))

        # # Focus on main window
        # self.setFocusPolicy(QtCore.Qt.StrongFocus)
        # self.raise_()
        # self.activateWindow()

        # Initialization ends
        self.initialization = False

        # Save json settings
        self.saveJsonSettings()

    def visualizeJsonSettings(self):
        """ Handler for visualize the JSON settings """
        ## Channel Section
        self.comboBoxChannelSelected.setCurrentIndex(self.jsonSettings["channel_section"]["selectedChannel"].toInt())
        ## Model Section
        self.comboBoxModelSelected.setCurrentText(self.jsonSettings["model"].toString())
        ## Thresholding
        self.doubleSpinBoxRightBoundary.setValue(self.jsonSettings["thresholding"]["rightBoundary"].toInt())
        self.doubleSpinBoxLeftBoundary.setValue(self.jsonSettings["thresholding"]["leftBoundary"].toInt())
        self.doubleSpinBoxUpperBoundary.setValue(self.jsonSettings["thresholding"]["upperBoundary"].toInt())
        self.doubleSpinBoxLowerBoundary.setValue(self.jsonSettings["thresholding"]["lowerBoundary"].toInt())
        self.doubleSpinBoxProfileOffset.setValue(self.jsonSettings["thresholding"]["profileOffset"].toInt())
        ## Pso
        self.spinBoxNIterations.setValue(self.jsonSettings["pso"]["nIterations"].toInt())
        self.spinBoxNParticles.setValue(self.jsonSettings["pso"]["nParticles"].toInt())
        ## Optimization
        self.spinBoxOptimizationLossFieldMargin.setValue(self.jsonSettings["optimization"]["lossFieldMargin"].toInt())
        ## Search Space
        searchSpaceIndex = self.comboBoxModelSelected.currentIndex()
        print(searchSpaceIndex)
        self.spinBoxSearchSpaceWinSizeX.setValue(self.jsonSettings["search_space"][searchSpaceIndex]["winSizeX"].toInt())
        self.spinBoxSearchSpaceWinSizeY.setValue(self.jsonSettings["search_space"][searchSpaceIndex]["winSizeY"].toInt())
        self.doubleSpinBoxSearchSpaceMinAlpha.setValue(self.jsonSettings["search_space"][searchSpaceIndex]["minAlpha"].toDouble())
        self.doubleSpinBoxSearchSpaceMaxAlpha.setValue(self.jsonSettings["search_space"][searchSpaceIndex]["maxAlpha"].toDouble())
        self.doubleSpinBoxSearchSpaceMinMMultiplier.setValue(self.jsonSettings["search_space"][searchSpaceIndex]["minMMultiplier"].toDouble())
        self.doubleSpinBoxSearchSpaceMaxMMultiplier.setValue(self.jsonSettings["search_space"][searchSpaceIndex]["maxMMultiplier"].toDouble())

    def updateCalibrationState(self):
        """ Handler for update the calibration state """
        if self.calibrationState:
            self.labelTracerCalibrationState.setText("Calibrated")
        else:
            self.labelTracerCalibrationState.setText("Not calibrated")
        # self.createCalibrationTextOutput(self.calibrationState)

    def closeEvent(self, event):
        """ Handler for the UI close event """

        # Save the configuration parameters
        self.saveConfig()

        # If the illumination peripheral is connected
        # and the LED mustn't be always on
        if self.illumination_connected:
            # Switch the illumination LED off
            cmd = "SET_PWR: 0\r"
            try:
                self.illumination_serial.write(cmd.encode())
            except:
                traceback.print_exc(file=sys.stdout)
                pass

        # Close camera software
        if self.cam_software_connected:
            self.cam_software_manager.closeCameraExe()

        logging.info("==========   Application closed   =========")
        logging.info("===========================================")



    @QtCore.pyqtSlot()
    def pushButtonCameraSoftwareLaunch_clicked(self):
        """ Handler for the pushButtonCameraSoftwareLaunch clicked event
            Launch camera software and manage the result in the UI """

        exe_folder_path = self.lineEditExecutableFolder.text()
        self.cam_software_result_code = self.cam_software_manager.launchCameraExe(folder_path=exe_folder_path)
        message = ""
        if HTTCameraResultCode.HTT_CAM_SUCCESSFUL == self.cam_software_result_code:
            self.cam_software_connected = True
            message = "Camera software correctly started"
        elif HTTCameraResultCode.HTT_CAM_NOT_CONNECTED == self.cam_software_result_code:
            self.cam_software_connected = False
            message = "Camera not connected"
        elif HTTCameraResultCode.HTT_CAM_PROCESS_ERROR == self.cam_software_result_code:
            self.cam_software_connected = False
            message = "Unable to start camera software"
        elif HTTCameraResultCode.HTT_CAM_CANNOT_TOUCH_WND == self.cam_software_result_code:
            self.cam_software_connected = True
            message = "Camera software correctly started"
        else:
            self.cam_software_connected = False
            message = "Generic error condition during camera software starting"

        # Show the launching state in the proper label
        self.labelCameraSoftwareState.setText(message)
        if self.cam_software_connected:
            self.labelCameraSoftwareState.setStyleSheet("QLabel {color: green}")
        else:
            self.labelCameraSoftwareState.setStyleSheet("QLabel {color: red}")

        # Show an error message box if the launching wasn't successful
        if not self.cam_software_connected:
            if not self.initialization:
                QMessageBox.critical(self,
                                     'Problem with camera software starting',
                                     message,
                                     QMessageBox.Ok)

        # Save camera exe path if the launching was successful
        if self.cam_software_connected:
            self.parameters.CameraSoftwareFolderPath = exe_folder_path

        # Enabling/disabling of the launch/close buttons
        if self.cam_software_connected:
            self.pushButtonCameraSoftwareLaunch.setEnabled(False)
            self.pushButtonCameraSoftwareClose.setEnabled(True)
            self.groupBoxExecutableFolder.setEnabled(False)
        else:
            self.pushButtonCameraSoftwareLaunch.setEnabled(True)
            self.pushButtonCameraSoftwareClose.setEnabled(False)
            self.groupBoxExecutableFolder.setEnabled(True)

        # Close camera software process if the launching wasn't successful
        if not self.cam_software_connected:
            self.cam_software_manager.closeCameraExe()


    @QtCore.pyqtSlot()
    def pushButtonCameraSoftwareClose_clicked(self):
        """ Handler for the pushButtonCameraSoftwareClose clicked event
            Close camera software and manage the result in the UI """

        self.cam_software_manager.closeCameraExe()

        self.pushButtonCameraSoftwareLaunch.setEnabled(True)
        self.pushButtonCameraSoftwareClose.setEnabled(False)
        self.groupBoxExecutableFolder.setEnabled(True)

        self.labelCameraSoftwareState.setText("Launch camera software to grab the images")
        self.labelCameraSoftwareState.setStyleSheet("QLabel {color: black}")


    @QtCore.pyqtSlot()
    def pushButtonManualGrab_clicked(self):
        """ Handler for the pushButtonManualGrab clicked event """

        # Grab the image!
        self.grabShowSaveImage(enable_message_box=True)


    @QtCore.pyqtSlot()
    def grabLabelTimer_timeout(self):
        self.labelGrabbingState.setText("")


    @QtCore.pyqtSlot()
    def settingsLabelTimerTimeout(self):
        self.labelSettingsState.setText("")

    @QtCore.pyqtSlot()
    def calibrationLabelTimerTimeout(self):
        self.labelCalibrationState.setText("")

    @QtCore.pyqtSlot()
    def pushButtonFrameProgressiveRestart_clicked(self):
        """ Handler for the pushButtonFrameProgressiveRestart clicked event """

        self.spinBoxFrameProgressive.setValue(-1)


    @QtCore.pyqtSlot()
    def pushButtonExecutableFolderBrowse_clicked(self):
        """ Handler for the pushButtonExecutableFolderBrowse clicked event """

        path = QtWidgets.QFileDialog.getExistingDirectory(self, directory=self.lineEditExecutableFolder.text(), caption="Select executable folder")
        if path:
            self.lineEditExecutableFolder.setText(path)


    @QtCore.pyqtSlot()
    def lineEditSessionName_textChanged(self):
        """ Handler for the lineEditSessionName text-changed event """

        # Reset frame progressive
        self.spinBoxFrameProgressive.setValue(-1)


    @QtCore.pyqtSlot()
    def pushButtonImageSavingFolderBrowse_clicked(self):
        """ Handler for the pushButtonImageSavingFolderBrowse clicked event """

        path = QtWidgets.QFileDialog.getExistingDirectory(self, directory=self.lineEditImageSavingFolder.text(),
                                                          caption="Select saving folder")
        if path:
            self.lineEditImageSavingFolder.setText(path)

    @QtCore.pyqtSlot()
    def pushButtonLoadTestImage_clicked(self):
        """ Handler for the pushButtonLoadTestImage clicked event """
        path = QtWidgets.QFileDialog.getOpenFileName(self, directory=self.lineEditTestImageLoaded.text(),
                                                          caption="Select image for manual grab")

        if path:
            self.lineEditTestImageLoaded.setText(path[0])
            self.pushButtonClearTestImage.setEnabled(True)
            self.pushButtonProcessTestImage.setEnabled(True)
            self.checkBoxTestImageRepetitions.setEnabled(True)
            self.pushButtonExportProcessingTestImage.setEnabled(False)

    @QtCore.pyqtSlot()
    def pushButtonClearTestImage_clicked(self):
        """ Handler for the pushButtonClearTestImage clicked event """
        self.lineEditTestImageLoaded.setText("")
        self.pushButtonClearTestImage.setEnabled(False)
        self.pushButtonProcessTestImage.setEnabled(False)
        self.checkBoxTestImageRepetitions.setEnabled(False)

    @QtCore.pyqtSlot()
    def pushButtonProcessTestImage_clicked(self):
        """ Handler for the pushButtonProcessTestImage clicked event """
        self.labelSettingsState.setStyleSheet("QLabel {color: black}")
        self.labelSettingsState.setText("Running...")
        self.textBrowserTestImageOutput.setText("")
        app.processEvents()
        imagePath = self.lineEditTestImageLoaded.text()
        filename = imagePath.rsplit('/', 1)[1]
        # LTT12-M801_pos=R100_H=+14.7_deltaN=+00.0_LED=0005.png
        # Extract deltaN and H from the filename
        H = 0
        deltaN = 0
        tokens = filename.split('_')
        for token in tokens:
            if token[0]=="H":
                H = float(token.split('=')[1])
            if token[0]=="d" and token[1]=="e" and token[2]=="l" and token[3]=="t" and token[4]=="a" and token[5]=="N":
                deltaN = float(token.split('=')[1])
        image_np = imageio.imread(imagePath)

        modelType = self.comboBoxModelSelected.currentIndex()
        if self.checkBoxTestImageRepetitions.isChecked():
            rep = self.spinBoxTestImageRepetitions.value()
            array1 = []
            array2 = []
            array3 = []
            array4 = []
            arrayImages = []
            for i in range(rep):
                image_validated_np = Htt.process(image_np, SETTINGS_FILE_PATH, -deltaN, H, False, modelType)
                measures = Htt.getMeasures()
                array1.append(measures[2])
                array2.append(measures[3])
                array3.append(measures[4])
                if modelType == 2:  # CustomBevelModel
                        array4.append(measures[5])
                arrayImages.append(image_validated_np)
                i += 1
            indexArray = self.createProcessTestImageTextOutputWithRepetitions(modelType, array1, array2, array3, array4)
            self.labelSettingsState.setStyleSheet("QLabel {color: green}")
            self.labelSettingsState.setText("Process procedure succeded")
            self.settings_label_timer.stop()
            self.settings_label_timer.start(3000)
            image_validated = Image.fromarray(arrayImages[indexArray])
            self.showImage(image_validated)
        else:
            image_validated_np = Htt.process(image_np, SETTINGS_FILE_PATH, -deltaN, H, False, modelType)
            if len(image_validated_np) == 0:
                self.labelSettingsState.setStyleSheet("QLabel {color: red}")
                self.labelSettingsState.setText("Process procedure failed")
                self.settings_label_timer.stop()
                self.settings_label_timer.start(3000)
            else: 
                self.labelSettingsState.setStyleSheet("QLabel {color: green}")
                self.labelSettingsState.setText("Process procedure succeded")
                self.settings_label_timer.stop()
                self.settings_label_timer.start(3000)
                self.createProcessTestImageTextOutput(modelType)
                image_validated = Image.fromarray(image_validated_np)
                self.showImage(image_validated)
                self.pushButtonExportProcessingTestImage.setEnabled(True)

    @QtCore.pyqtSlot()
    def checkBoxTestImageRepetitions_stateChanged(self):
        """ Handler for the checkBoxTestImageRepetitions state changed """
        self.spinBoxTestImageRepetitions.setEnabled(self.checkBoxTestImageRepetitions.isChecked())

    @QtCore.pyqtSlot()
    def spinBoxTestImageRepetitions_valueChanged(self):
        """ Handler for the spinBoxTestImageRepetitions valued changed """
        val = self.spinBoxTestImageRepetitions.value()
        self.spinBoxTestImageRepetitions.setValue(val if (val % 2) != 0 else val + 1)

    @QtCore.pyqtSlot()
    def pushButtonExportProcessingTestImage_clicked(self):
        """ Handler for the pushButtonExportProcessingTestImage clicked event """
        path = QtWidgets.QFileDialog.getExistingDirectory(self, caption="Select saving folder")
        if path:
            imagePath = self.lineEditTestImageLoaded.text()
            filename = imagePath.rsplit('/', 1)[1].split(".png")[0] + "_proc_"
            def getChannel(index):
                switcher = {
                    0: "R",
                    1: "G",
                    2: "B"
                }
                return  switcher.get(index, "")
            filename += "ch=" + getChannel(self.comboBoxChannelSelected.currentIndex())
            self.qimage.save(path + "/" + filename + ".png", "PNG")


            # print(self.textBrowserTestImageOutput.toPlainText())
            file = QFile(path + "/" + filename + ".txt")
            file.open(QtCore.QFile.WriteOnly)
            file.write(self.textBrowserTestImageOutput.toPlainText().encode())
            file.close()

    @QtCore.pyqtSlot()
    def pushButtonTracerConnect_clicked(self):
        """ Handler for the pushButtonTracerConnect clicked event """

        # Check combo selections
        sel_ok = True
        sel_ok = sel_ok and (self.comboBoxTracerCOMPort.currentIndex() >= 0)
        sel_ok = sel_ok and (self.comboBoxTracerPortBitsPerSecond.currentIndex() >= 0)
        sel_ok = sel_ok and (self.comboBoxTracerPortDataBits.currentIndex() >= 0)
        sel_ok = sel_ok and (self.comboBoxTracerPortParity.currentIndex() >= 0)
        sel_ok = sel_ok and (self.comboBoxTracerPortStopBits.currentIndex() >= 0)
        if not sel_ok:
            # Save disconnected state
            self.tracer_connected = False
            # Show the error label
            self.labelTracerState.setText("Set all port parameters to connect to the tracer")
            self.labelTracerState.setStyleSheet("QLabel {color: red}")
            # Show the error message box
            if not self.initialization:
                QMessageBox.critical(self,
                                     'Cannot connect',
                                     "Set all port parameters to connect to the tracer!",
                                     QMessageBox.Ok)
            # Update enabling/disabling of corresponding UI elements
            self.pushButtonTracerConnect.setEnabled(True)
            self.pushButtonTracerDisconnect.setEnabled(False)
            self.groupBoxTracerPortSettings.setEnabled(True)
            return

        # Try to connect
        try:
            port = self.comboBoxTracerCOMPort.currentText()
            baud = int(self.comboBoxTracerPortBitsPerSecond.currentText())
            byte = int(self.comboBoxTracerPortDataBits.currentText())
            pari = self.comboBoxTracerPortParity.currentText()
            stop = float(self.comboBoxTracerPortStopBits.currentText())
            self.tracer_serial.port     = port
            self.tracer_serial.baudrate = baud
            self.tracer_serial.bytesize = byte
            self.tracer_serial.parity   = pari
            self.tracer_serial.stopbits = stop
            self.tracer_serial.timeout  = 1
            logging.info("Connecting to the port with the following parameters:")
            logging.info(" - COM port:\t"        + port)
            logging.info(" - Bits per second:\t" + str(baud))
            logging.info(" - Data bits:\t"       + str(byte))
            logging.info(" - Parity:\t"          + pari)
            logging.info(" - Stop bits:\t"       + str(stop))
            self.tracer_serial.open()
            logging.info("Correctly connected")

            # Save connected state
            self.tracer_connected = True

            # Start the receiver
            self.tracer_read_timer.start(SERIAL_READ_PERIOD)

            # Save port settings
            self.parameters.TracerPortCOM           = port
            self.parameters.TracerPortBitsPerSecond = baud
            self.parameters.TracerPortDataBits      = byte
            self.parameters.TracerPortParity        = pari
            self.parameters.TracerPortStopBits      = stop

            # Update label
            self.labelTracerState.setText("Correctly connected to the tracer")
            self.labelTracerState.setStyleSheet("QLabel {color: green}")
            # Update enabling/disabling of corresponding UI elements
            self.pushButtonTracerConnect.setEnabled(False)
            self.pushButtonTracerDisconnect.setEnabled(True)
            self.groupBoxTracerPortSettings.setEnabled(False)
        except:
            logging.error("Error during connection")
            traceback.print_exc(file=sys.stdout)

            # Save disconnected state
            self.tracer_connected = False
            # Show the error label
            self.labelTracerState.setText("Error during connection to the tracer")
            self.labelTracerState.setStyleSheet("QLabel {color: red}")
            # Show the error message box
            if not self.initialization:
                QMessageBox.critical(self,
                                     'Cannot connect',
                                     "Error during connection to the tracer",
                                     QMessageBox.Ok)
            # Update enabling/disabling of corresponding UI elements
            self.pushButtonTracerConnect.setEnabled(True)
            self.pushButtonTracerDisconnect.setEnabled(False)
            self.groupBoxTracerPortSettings.setEnabled(True)


    @QtCore.pyqtSlot()
    def pushButtonTracerDisconnect_clicked(self):
        """ Handler for the pushButtonTracerDisconnect clicked event """

        # Try to disconnect
        try:
            # Stop the receiver
            self.tracer_read_timer.stop()

            # Disconnect
            self.tracer_serial.close()
            logging.info("Correctly disconnected")

            # Save disconnected state
            self.tracer_connected = False

            # Update enabling/disabling of corresponding UI elements
            self.pushButtonTracerConnect.setEnabled(True)
            self.pushButtonTracerDisconnect.setEnabled(False)
            self.groupBoxTracerPortSettings.setEnabled(True)
            # Update label
            self.labelTracerState.setText("Connect to the tracer to receive the grabbing commands")
            self.labelTracerState.setStyleSheet("QLabel {color: black}")
            # Update the list of COM ports
            avail_ports = [x.device for x in serial.tools.list_ports.comports()]
            sel_port    = self.comboBoxTracerCOMPort.currentText()
            self.comboBoxTracerCOMPort.clear()
            self.comboBoxTracerCOMPort.addItems(avail_ports)
            # Keep previously selected port, if available
            self.comboBoxTracerCOMPort.setCurrentText(sel_port)
        except:
            logging.error("Error during disconnection")
            traceback.print_exc(file=sys.stdout)
            # Show the error message box
            QMessageBox.critical(self,
                                 'Cannot disconnect',
                                 "Error during disconnection from the tracer",
                                 QMessageBox.Ok)


    @QtCore.pyqtSlot()
    def pushButtonIlluminationConnect_clicked(self):
        """ Handler for the pushButtonIlluminationConnect clicked event """

        # Check combo selections
        sel_ok = True
        sel_ok = sel_ok and (self.comboBoxIlluminationCOMPort.currentIndex() >= 0)
        sel_ok = sel_ok and (self.comboBoxIlluminationPortBitsPerSecond.currentIndex() >= 0)
        sel_ok = sel_ok and (self.comboBoxIlluminationPortDataBits.currentIndex() >= 0)
        sel_ok = sel_ok and (self.comboBoxIlluminationPortParity.currentIndex() >= 0)
        sel_ok = sel_ok and (self.comboBoxIlluminationPortStopBits.currentIndex() >= 0)
        if not sel_ok:
            # Save disconnected state
            self.illumination_connected = False
            # Show the error label
            self.labelIlluminationState.setText("Set all port parameters to connect to the illumination peripheral")
            self.labelIlluminationState.setStyleSheet("QLabel {color: red}")
            # Show the error message box
            if not self.initialization:
                QMessageBox.critical(self,
                                     'Cannot connect',
                                     "Set all port parameters to connect to the illumination peripheral!",
                                     QMessageBox.Ok)
            self.pushButtonIlluminationConnect.setEnabled(True)
            self.pushButtonIlluminationDisconnect.setEnabled(False)
            self.groupBoxIlluminationPortSettings.setEnabled(True)
            self.groupBoxIlluminationLED.setEnabled(False)
            return

        # Try to connect
        try:
            port = self.comboBoxIlluminationCOMPort.currentText()
            baud = int(self.comboBoxIlluminationPortBitsPerSecond.currentText())
            byte = int(self.comboBoxIlluminationPortDataBits.currentText())
            pari = self.comboBoxIlluminationPortParity.currentText()
            stop = float(self.comboBoxIlluminationPortStopBits.currentText())
            self.illumination_serial.port     = port
            self.illumination_serial.baudrate = baud
            self.illumination_serial.bytesize = byte
            self.illumination_serial.parity   = pari
            self.illumination_serial.stopbits = stop
            self.illumination_serial.timeout  = 1
            logging.info("Connecting to the port with the following parameters:")
            logging.info(" - COM port:\t"        + port)
            logging.info(" - Bits per second:\t" + str(baud))
            logging.info(" - Data bits:\t"       + str(byte))
            logging.info(" - Parity:\t"          + pari)
            logging.info(" - Stop bits:\t"       + str(stop))
            self.illumination_serial.open()
            logging.info("Correctly connected")

            # Save connected state
            self.illumination_connected = True

            # Start the receiver,
            # in order to digest the commands coming anyway
            self.illumination_read_timer.start(SERIAL_READ_PERIOD)
            # Save port settings
            self.parameters.IlluminationPortCOM           = port
            self.parameters.IlluminationPortBitsPerSecond = baud
            self.parameters.IlluminationPortDataBits      = byte
            self.parameters.IlluminationPortParity        = pari
            self.parameters.IlluminationPortStopBits      = stop

            # Update label
            self.labelIlluminationState.setText("Correctly connected to the illumination peripheral")
            self.labelIlluminationState.setStyleSheet("QLabel {color: green}")
            # Update enabling/disabling of corresponding UI elements
            self.pushButtonIlluminationConnect.setEnabled(False)
            self.pushButtonIlluminationDisconnect.setEnabled(True)
            self.groupBoxIlluminationPortSettings.setEnabled(False)
            self.groupBoxIlluminationLED.setEnabled(True)
        except:
            logging.info("Error during connection")
            traceback.print_exc(file=sys.stdout)
            # Save disconnected state
            self.illumination_connected = False
            # Show the error label
            self.labelIlluminationState.setText("Error during connection to the illumination peripheral")
            self.labelIlluminationState.setStyleSheet("QLabel {color: red}")
            # Show the error message box
            if not self.initialization:
                QMessageBox.critical(self,
                                     'Cannot connect',
                                     "Error during connection to the illumination peripheral",
                                     QMessageBox.Ok)
            # Update enabling/disabling of corresponding UI elements
            self.pushButtonIlluminationConnect.setEnabled(True)
            self.pushButtonIlluminationDisconnect.setEnabled(False)
            self.groupBoxIlluminationPortSettings.setEnabled(True)
            self.groupBoxIlluminationLED.setEnabled(False)


    @QtCore.pyqtSlot()
    def pushButtonIlluminationDisconnect_clicked(self):
        """ Handler for the pushButtonIlluminationDisconnect clicked event """

        # Try to disconnect
        try:
            # Stop the receiver
            self.illumination_read_timer.stop()

            # Disconnect
            self.illumination_serial.close()
            logging.info("Correctly disconnected")

            # Save disconnected state
            self.illumination_connected = False

            # Update enabling/disabling of corresponding UI elements
            self.pushButtonIlluminationConnect.setEnabled(True)
            self.pushButtonIlluminationDisconnect.setEnabled(False)
            self.groupBoxIlluminationPortSettings.setEnabled(True)
            self.groupBoxIlluminationLED.setEnabled(False)
            # Update label
            self.labelIlluminationState.setText("Connect to the illumination peripheral to control the light")
            self.labelIlluminationState.setStyleSheet("QLabel {color: black}")
            # Update the list of COM ports
            avail_ports = [x.device for x in serial.tools.list_ports.comports()]
            sel_port    = self.comboBoxIlluminationCOMPort.currentText()
            self.comboBoxIlluminationCOMPort.clear()
            self.comboBoxIlluminationCOMPort.addItems(avail_ports)
            # Keep previously selected port, if available
            self.comboBoxIlluminationCOMPort.setCurrentText(sel_port)
        except:
            logging.error("Error during disconnection")
            traceback.print_exc(file=sys.stdout)
            # Show the error message box
            QMessageBox.critical(self,
                                 'Cannot disconnect',
                                 "Error during disconnection from the illumination peripheral",
                                 QMessageBox.Ok)


    @QtCore.pyqtSlot()
    def tabWidgetConnectionConfiguration_currentChanged(self):
        """ Handler for the tabWidgetConnectionConfiguration current-changed event """

        if self.tabWidgetConnectionConfiguration.currentIndex() == 1:
            # Tracer
            # If the tracer is not connected,
            # update the list of COM ports
            if not self.tracer_connected:
                avail_ports = [x.device for x in serial.tools.list_ports.comports()]
                sel_port    = self.comboBoxTracerCOMPort.currentText()
                self.comboBoxTracerCOMPort.clear()
                self.comboBoxTracerCOMPort.addItems(avail_ports)
                # Keep previously selected port, if available
                self.comboBoxTracerCOMPort.setCurrentText(sel_port)
        elif self.tabWidgetConnectionConfiguration.currentIndex() == 2:
            # Illumination
            # If the illumination peripheral is not connected,
            # update the list of COM ports
            if not self.illumination_connected:
                avail_ports = [x.device for x in serial.tools.list_ports.comports()]
                sel_port    = self.comboBoxIlluminationCOMPort.currentText()
                self.comboBoxIlluminationCOMPort.clear()
                self.comboBoxIlluminationCOMPort.addItems(avail_ports)
                # Keep previously selected port, if available
                self.comboBoxIlluminationCOMPort.setCurrentText(sel_port)


    @QtCore.pyqtSlot()
    def spinBoxIlluminationPower_valueChanged(self):
        """ Handler for the spinBoxIlluminationPower value-changed event """

        # Manage the illumination power value
        # if not self.initialization:
        if self.checkBoxIlluminationContinuouslyOn.isChecked():
            if self.illumination_connected:
                # Set current illumination value
                value = self.spinBoxIlluminationPower.value()
                cmd   = "SET_PWR: " + str(value) + "\r"
                try:
                    self.illumination_serial.write(cmd.encode())
                    logging.info("Correctly set LED power to " + str(value))

                    # Save the value set
                    self.parameters.IlluminationLEDPower = value
                except:
                    logging.error("Error during setting LED power to " + str(value))
                    traceback.print_exc(file=sys.stdout)
                    pass

    @QtCore.pyqtSlot()
    def checkBoxIlluminationContinuouslyOn_stateChanged(self):
        """ Handler for the checkBoxIlluminationContinuouslyOn state-changed event """

        if self.checkBoxIlluminationContinuouslyOn.isChecked():
            # LED continuously on

            # Limit the power
            max_power = self.parameters.IlluminationMaximumContinuousLEDPower
            self.spinBoxIlluminationPower.setMaximum(max_power)
            self.labelLEDInfo.setText("( Power is limited to " + str(max_power) + " in continuous mode )")

            app.processEvents()

            # Set current value,
            # if the illumination peripheral is connected
            if self.illumination_connected:
                value = self.spinBoxIlluminationPower.value()
                cmd   = "SET_PWR: " + str(value) + "\r"
                try:
                    self.illumination_serial.write(cmd.encode())
                    logging.info("Correctly set LED power to " + str(value))
                except:
                    logging.error("Error during setting LED power to " + str(value))
                    traceback.print_exc(file=sys.stdout)
                    pass
        else:
            # LED on only during acquisition

            # The maximum allowed power is contained in the config file
            self.spinBoxIlluminationPower.setMaximum(self.parameters.IlluminationMaximumLEDPower)
            self.labelLEDInfo.setText("")

            app.processEvents()

            # Turn the LED off,
            # if the illumination peripheral is connected
            if self.illumination_connected:
                value = 0
                cmd   = "SET_PWR: " + str(value) + "\r"
                try:
                    self.illumination_serial.write(cmd.encode())
                except:
                    traceback.print_exc(file=sys.stdout)
                    pass

    @QtCore.pyqtSlot()
    def spinBoxLEDOnTime_valueChanged(self):
        """ Handler for the spinBoxLEDOnTime value-changed event """

        # Save the value
        self.parameters.AcquisitionLEDOnTime = self.spinBoxLEDOnTime.value()

    @QtCore.pyqtSlot()
    def spinBoxLEDOnDelay_valueChanged(self):
        """ Handler for the spinBoxLEDOnDelay value-changed event """

        # Save the value
        self.parameters.AcquisitionLEDOnDelay = self.spinBoxLEDOnDelay.value()

    @QtCore.pyqtSlot()
    def pushButtonSettingsReset_clicked(self):
        """ Handler for the pushButtonSettingsReset value-changed event """
        # Load JSON settings
        ret, self.jsonSettings = self.loadJsonSettings(DEFAULT_SETTINGS_FILE_PATH)
        if not ret:
            QMessageBox.critical(self,
                                 'Problem with json settings loading',
                                 "Unable to load all, or some, information of the configuration file, or configuration file path is wrong.",
                                 QMessageBox.Ok)
        else:
            self.calibrationState = False
            self.updateCalibrationState()
            self.saveJsonSettings()
            self.createCalibrationTextOutput(self.calibrationState)
            self.visualizeJsonSettings()
            # Show successful result in the label
            self.labelSettingsState.setStyleSheet("QLabel {color: green}")
            self.labelSettingsState.setText("Default Json configuration correctly restored")
            self.settings_label_timer.stop()
            self.settings_label_timer.start(2000)

    @QtCore.pyqtSlot()
    def comboBoxCalibrationFrame_currentIndexChanged(self):
        """ Handler for the comboBoxCalibrationFrame value-changed event """
        self.updateCalibrationFrameParams()
        self.saveJsonSettings()

    @QtCore.pyqtSlot()
    def pushButtonCalibration_clicked(self):
        """ Handler for the pushButtonCalibration value-changed event """
        self.labelCalibrationState.setStyleSheet("QLabel {color: black}")
        self.labelCalibrationState.setText("Running...")
        app.processEvents()
        imagePath = self.lineEditCalibrationImage.text()
        filename = imagePath.rsplit('/', 1)[1]
        # LTT12-M801_pos=R100_H=+14.7_deltaN=+00.0_LED=0005.png
        # Extract deltaN and H from the filename
        tokens = filename.split('_')
        for token in tokens:
            if token[0]=="H":
                H = float(token.split('=')[1])
            if token[0]=="d" and token[1]=="e" and token[2]=="l" and token[3]=="t" and token[4]=="a" and token[5]=="N":
                deltaN = float(token.split('=')[1])
        print(filename, H, deltaN)
        image_np = imageio.imread(imagePath)
        ret = Htt.calibrate(image_np, SETTINGS_FILE_PATH, -deltaN, H)
        print(ret)
        if ret == False:
            self.labelCalibrationState.setStyleSheet("QLabel {color: red}")
            self.labelCalibrationState.setText("Calibration procedure failed")
            self.calibration_label_timer.stop()
            self.calibration_label_timer.start(3000)
        else:
            self.labelCalibrationState.setStyleSheet("QLabel {color: green}")
            self.labelCalibrationState.setText("Calibration procedure succeded")
            self.calibration_label_timer.stop()
            self.calibration_label_timer.start(3000)
            sleep(1)
            ret, self.jsonSettings = self.loadJsonSettings(SETTINGS_FILE_PATH)
            if not ret:
                QMessageBox.critical(self,
                                    'Problem with json settings loading',
                                    "Unable to load all, or some, information of the configuration file, or configuration file path is wrong.",
                                    QMessageBox.Ok)
            else:
                self.calibrationState = True
                self.calibrationFrameName = self.comboBoxCalibrationFrame.currentText()
                self.updateCalibrationState()
                self.saveJsonSettings()
                sleep(1)
                ret, self.jsonSettings = self.loadJsonSettings(SETTINGS_FILE_PATH)
                self.createCalibrationTextOutput(self.calibrationState)

    @QtCore.pyqtSlot()
    def pushButtonValidation_clicked(self):
        """ Handler for the pushButtonValidation value-changed event """
        self.labelCalibrationState.setStyleSheet("QLabel {color: black}")
        self.labelCalibrationState.setText("Running...")
        app.processEvents()
        imagePath = self.lineEditCalibrationImage.text()
        filename = imagePath.rsplit('/', 1)[1]
        # LTT12-M801_pos=R100_H=+14.7_deltaN=+00.0_LED=0005.png
        # Extract deltaN and H from the filename
        tokens = filename.split('_')
        for token in tokens:
            if token[0]=="H":
                H = float(token.split('=')[1])
            if token[0]=="d" and token[1]=="e" and token[2]=="l" and token[3]=="t" and token[4]=="a" and token[5]=="N":
                deltaN = float(token.split('=')[1])
        image_np = imageio.imread(imagePath)

        modelType = 0 # 0:MiniBevelModel
        if self.checkBoxValidationRepetitions.isChecked():
            rep = self.spinBoxValidationRepetitions.value()
            arrayB = []
            arrayM = []
            arrayD = []
            arrayImages = []
            for i in range(rep):
                image_validated_np = Htt.process(image_np, SETTINGS_FILE_PATH, -deltaN, H, True, modelType)
                measures = Htt.getMeasures()
                arrayB.append(measures[2])
                arrayM.append(measures[3])
                arrayD.append(measures[4])
                arrayImages.append(image_validated_np)
                i += 1
            indexArrayB = self.createValidationTextOutputWithRepetitions(arrayB, arrayM, arrayD)
            self.labelCalibrationState.setStyleSheet("QLabel {color: green}")
            self.labelCalibrationState.setText("Validation procedure succeded")
            self.calibration_label_timer.stop()
            self.calibration_label_timer.start(3000)
            image_validated = Image.fromarray(arrayImages[indexArrayB])
            self.showImage(image_validated)
        else:
            image_validated_np = Htt.process(image_np, SETTINGS_FILE_PATH, -deltaN, H, True, modelType)
            if len(image_validated_np) == 0:
                self.labelCalibrationState.setStyleSheet("QLabel {color: red}")
                self.labelCalibrationState.setText("Validation procedure failed")
                self.calibration_label_timer.stop()
                self.calibration_label_timer.start(3000)
            else: 
                self.labelCalibrationState.setStyleSheet("QLabel {color: green}")
                self.labelCalibrationState.setText("Validation procedure succeded")
                self.calibration_label_timer.stop()
                self.calibration_label_timer.start(3000)
                self.createValidationTextOutput()
                image_validated = Image.fromarray(image_validated_np)
                self.showImage(image_validated)

    @QtCore.pyqtSlot()
    def checkBoxValidationRepetitions_stateChanged(self):
        self.spinBoxValidationRepetitions.setEnabled(self.checkBoxValidationRepetitions.isChecked())

    @QtCore.pyqtSlot()
    def spinBoxValidationRepetitions_valueChanged(self):
        val = self.spinBoxValidationRepetitions.value()
        self.spinBoxValidationRepetitions.setValue(val if (val % 2) != 0 else val + 1)

    @QtCore.pyqtSlot()
    def checkBoxProcessingImageRepetitions_stateChanged(self):
        self.spinBoxRepetitions.setEnabled(self.checkBoxProcessingImageRepetitions.isChecked())

    @QtCore.pyqtSlot()
    def common_jsonValueChanged(self):
        self.saveJsonSettings()

    @QtCore.pyqtSlot()
    def spinBoxRepetitions_valueChanged(self):
        val = self.spinBoxRepetitions.value()
        self.spinBoxRepetitions.setValue(val if (val % 2) != 0 else val + 1)

    @QtCore.pyqtSlot()
    def tracerReadTimerTimeout(self):

        if self.tracer_connected:
            if self.tracer_serial:
                if self.tracer_serial.in_waiting:
                    try:
                        # Get data
                        data = self.tracer_serial.read_all()
                        print("> tracerReadTimerTimeout - data read: " + str(data))
                        # Emit them
                        self.tracer_data_ready.emit(data)
                    except:
                        traceback.print_exc(file=sys.stdout)
                        pass


    @QtCore.pyqtSlot(bytes)
    def tracerCommandCaught(self, data):

        # Check if there is an ACK (0xfe) command coming
        ack_found = False
        for i in range(len(data)):
            if data[i] == 0xfe:
                ack_found = True
                break

        if ack_found:

            logging.debug("Grabbing command come from the tracer")

            # Update the tracer command icon (green)
            self.labelTracerCommandIcon.setPixmap(QtGui.QPixmap(":/Icons/Icons/Green-circle-small.png"))

            app.processEvents()

            # Start the timer to show the green icon
            self.tracer_command_label_timer.stop()
            self.tracer_command_label_timer.start(100)

            # Wait for a while before switching the LED on...
            sleep(self.parameters.AcquisitionLEDOnDelay / 1000)

            # Grab the image!
            # (and manage its showing and saving)
            self.grabShowSaveImage()


    @QtCore.pyqtSlot()
    def tracerCommandLabelTimer_timeout(self):
        # Update the tracer command icon (black)
        self.labelTracerCommandIcon.setPixmap(QtGui.QPixmap(":/Icons/Icons/Black-circle-small.png"))

        app.processEvents()


    @QtCore.pyqtSlot()
    def illuminationReadTimerTimeout(self):

        if self.illumination_connected:
            if self.illumination_serial:
                if self.illumination_serial.in_waiting:
                    try:
                        # Get data
                        data = self.illumination_serial.read_all()
                        print("> illuminationReadTimerTimeout - data read: " + str(data))
                        # Emit them
                        self.illumination_data_ready.emit(data)
                    except:
                        traceback.print_exc(file=sys.stdout)
                        pass


    def loadConfig(self, file_path=CONFIG_FILE_PATH):
        """ Load the parameters contained in the configuration file """

        parameters = HTTImagingParameters()
        try:
            self.settings = QSettings(file_path, QSettings.IniFormat)

            parameters.CameraSoftwareFolderPath              =     self.settings.value('CameraSoftware/CameraSoftwareFolderPath')

            parameters.TracerPortCOM                         =     self.settings.value('Tracer/TracerPortCOM')
            parameters.TracerPortBitsPerSecond               = int(self.settings.value('Tracer/TracerPortBitsPerSecond'))
            parameters.TracerPortDataBits                    = int(self.settings.value('Tracer/TracerPortDataBits'))
            parameters.TracerPortParity                      =     self.settings.value('Tracer/TracerPortParity')
            parameters.TracerPortStopBits                    = int(self.settings.value('Tracer/TracerPortStopBits'))
            parameters.TracerPortFlowControl                 = None

            parameters.IlluminationPortCOM                   =     self.settings.value('Illumination/IlluminationPortCOM')
            parameters.IlluminationPortBitsPerSecond         = int(self.settings.value('Illumination/IlluminationPortBitsPerSecond'))
            parameters.IlluminationPortDataBits              = int(self.settings.value('Illumination/IlluminationPortDataBits'))
            parameters.IlluminationPortParity                =     self.settings.value('Illumination/IlluminationPortParity')
            parameters.IlluminationPortStopBits              = int(self.settings.value('Illumination/IlluminationPortStopBits'))
            parameters.IlluminationPortFlowControl           = None
            parameters.IlluminationMaximumLEDPower           = int(self.settings.value('Illumination/IlluminationMaximumLEDPower'))
            parameters.IlluminationLEDPower                  = int(self.settings.value('Illumination/IlluminationLEDPower'))
            parameters.IlluminationMaximumContinuousLEDPower = int(self.settings.value('Illumination/IlluminationMaximumContinuousLEDPower'))

            parameters.AcquisitionSavingFolder               =     self.settings.value('Acquisition/AcquisitionSavingFolder')
            parameters.AcquisitionLEDOnTime                  = int(self.settings.value('Acquisition/AcquisitionLEDOnTime'))
            parameters.AcquisitionLEDOnDelay                 = int(self.settings.value('Acquisition/AcquisitionLEDOnDelay'))

            logging.info("Setting correctly loaded")
            return True, parameters
        except:
            logging.error("Error during settings loading")
            traceback.print_exc(file=sys.stdout)
            return False, None


    def saveConfig(self, file_path=CONFIG_FILE_PATH):
        """ Save the parameters in the configuration file """

        try:
            self.settings.setValue('CameraSoftware/CameraSoftwareFolderPath', self.parameters.CameraSoftwareFolderPath)

            self.settings.setValue('Tracer/TracerPortCOM', self.parameters.TracerPortCOM)
            self.settings.setValue('Tracer/TracerPortBitsPerSecond', self.parameters.TracerPortBitsPerSecond)
            self.settings.setValue('Tracer/TracerPortDataBits', self.parameters.TracerPortDataBits)
            self.settings.setValue('Tracer/TracerPortParity', self.parameters.TracerPortParity)
            self.settings.setValue('Tracer/TracerPortStopBits', self.parameters.TracerPortStopBits)

            self.settings.setValue('Illumination/IlluminationPortCOM', self.parameters.IlluminationPortCOM)
            self.settings.setValue('Illumination/IlluminationPortBitsPerSecond', self.parameters.IlluminationPortBitsPerSecond)
            self.settings.setValue('Illumination/IlluminationPortDataBits', self.parameters.IlluminationPortDataBits)
            self.settings.setValue('Illumination/IlluminationPortParity', self.parameters.IlluminationPortParity)
            self.settings.setValue('Illumination/IlluminationPortStopBits', self.parameters.IlluminationPortStopBits)
            self.settings.setValue('Illumination/IlluminationLEDPower', self.parameters.IlluminationLEDPower)

            self.settings.setValue('Acquisition/AcquisitionSavingFolder', self.parameters.AcquisitionSavingFolder)
            self.settings.setValue('Acquisition/AcquisitionLEDOnTime', self.parameters.AcquisitionLEDOnTime)
            self.settings.setValue('Acquisition/AcquisitionLEDOnDelay', self.parameters.AcquisitionLEDOnDelay)

            logging.info("Setting correctly saved")
            return True
        except:
            logging.error("Error during settings saving")
            traceback.print_exc(file=sys.stdout)
            return False

    def loadJsonSettings(self, file_path=DEFAULT_SETTINGS_FILE_PATH):
        try:
            file = QFile(file_path)
            file.open(QtCore.QFile.ReadOnly)
            rawData = QByteArray(file.readAll())
            doc = QJsonDocument.fromJson(rawData)
            jsonSettings = doc.object()
            file.close()
            return True, jsonSettings
        except:
            traceback.print_exc(file=sys.stdout)
            return False, None

    def saveJsonSettings(self):
        if not self.initialization:
            try:
                doc = QJsonDocument(self.jsonSettings)
                root = doc.object()

                ## Channel section
                channel=root["channel_section"].toObject()
                channel["selectedChannel"] = self.comboBoxChannelSelected.currentIndex()
                root["channel_section"]=channel

                ## Model section
                root["model"] = self.comboBoxModelSelected.currentText()

                ## Thresholding
                thresholding = root["thresholding"].toObject()
                thresholding["rightBoundary"] = self.doubleSpinBoxRightBoundary.value()
                thresholding["leftBoundary"] = self.doubleSpinBoxLeftBoundary.value()
                thresholding["upperBoundary"] = self.doubleSpinBoxUpperBoundary.value()
                thresholding["lowerBoundary"] = self.doubleSpinBoxLowerBoundary.value()
                thresholding["profileOffset"] = self.doubleSpinBoxProfileOffset.value()
                root["thresholding"]=thresholding

                ## Pso
                pso = root["pso"].toObject()
                pso["nIterations"] = self.spinBoxNIterations.value()
                pso["nParticles"] = self.spinBoxNParticles.value()
                root["pso"]=pso

                ## Optimization
                optimization = root["optimization"].toObject()
                optimization["lossFieldMargin"] = self.spinBoxOptimizationLossFieldMargin.value()
                optimization["lossFieldMarginCalibration"] = self.spinBoxLossFildMarginCalibration.value()
                optimization["lossFieldMarginValidation"] = self.spinBoxLossFildMarginValidation.value()
                optimization["checkIntersectionsCalibration"] = self.checkBoxCheckIntersectionsCalibration.isChecked()
                optimization["checkIntersectionsValidation"] = self.checkBoxCheckIntersectionsValidation.isChecked()
                root["optimization"]=optimization

                ## Search Space
                searchSpaceIndex = self.comboBoxModelSelected.currentIndex()
                searchSpaceArray = root["search_space"].toArray()
                searchSpace = searchSpaceArray[searchSpaceIndex].toObject()
                searchSpace["winSizeX"] = self.spinBoxSearchSpaceWinSizeX.value()
                searchSpace["winSizeY"] = self.spinBoxSearchSpaceWinSizeY.value()
                searchSpace["minAlpha"] = self.doubleSpinBoxSearchSpaceMinAlpha.value()
                searchSpace["maxAlpha"] = self.doubleSpinBoxSearchSpaceMaxAlpha.value()
                searchSpace["minMMultiplier"] = self.doubleSpinBoxSearchSpaceMinMMultiplier.value()
                searchSpace["maxMMultiplier"] = self.doubleSpinBoxSearchSpaceMaxMMultiplier.value()
                searchSpaceArray[searchSpaceIndex] = searchSpace
                root["search_space"]=searchSpaceArray

                ## Scheimpflug Transformation Calibration
                stc = root["scheimpflug_transformation_calibration"].toObject()
                stc["calibrated"] = self.calibrationState
                stc["current"] = self.comboBoxCalibrationFrame.currentText()
                root["scheimpflug_transformation_calibration"]=stc

                ## Scheimpflug Transformation
                if self.calibrationState:
                    st = root["scheimpflug_transformation"].toObject()
                    st["frame"] = self.calibrationFrameName
                    root["scheimpflug_transformation"] = st


                # ## Test
                # test = root["test"].toObject()
                # test["repetitionsEnabled"] = self.checkBoxProcessingImageRepetitions.isChecked()
                # test["repetitions"] = self.spinBoxRepetitions.value()
                # root["test"] = test

                # Overwrite root json object
                doc.setObject(root)
                file = QFile(SETTINGS_FILE_PATH)
                file.open(QtCore.QFile.WriteOnly)
                file.write(doc.toJson())
                file.close()
                logging.info("Json Setting correctly saved")
                return True
            except:
                traceback.print_exc(file=sys.stdout)
                logging.error("Error during settings saving")
                return False
        return True

    def assembleImageName(self):
        # if self.lineEditImageLoaded.text() != "":
        #     filename = self.lineEditImageLoaded.text().strip()
        #     image_name=filename.rsplit('/', 1)[1].split('.png')[0]
        # else:
        ### String name format examples
        # frame05_LED=0100_R=-12_H=+11_pos=050.png
        # bevel806_LED=0005_R=+01_H=+08_pos=110.png
        image_name=""
        session_name = self.lineEditSessionName.text().strip() # remove leading and trailing whitespaces
        if len(session_name) > 0:
            image_name += session_name + "_"

        position = self.lineEditPositionTracer.text().strip()
        if len(position) > 0:
            image_name += "pos=" + position.zfill(4) + "_"

        H = self.doubleSpinBoxStilusAngleH.value()
        if H >= 0:
            strH = str(f'{H:04.1f}') + "_"
            image_name += "H=+" + strH
        else:
            strH = str(f'{H:05.1f}') + "_"
            image_name += "H=" + strH

        deltaN = self.doubleSpinBoxStilusAngleDeltaN.value()
        if deltaN >= 0:
            strDeltaN = str(f'{deltaN:04.1f}') + "_"
            image_name += "deltaN=+" + strDeltaN
        else:
            strDeltaN = str(f'{deltaN:05.1f}') + "_"
            image_name += "deltaN=" + strDeltaN

        image_name += "LED=" + str(self.spinBoxIlluminationPower.value()).zfill(4)

        if self.checkBoxSaveFrameProgressive.isChecked():
            image_name += "_P" + str(self.spinBoxFrameProgressive.value()).zfill(3)
        image_name_proc = image_name + "_proc_"
        def getChannel(index):
            switcher = {
                0: "R",
                1: "G",
                2: "B"
            }
            return  switcher.get(index, "")
        image_name_proc += "ch=" + getChannel(self.comboBoxChannelSelected.currentIndex())

        note = self.lineEditNote.text().strip()
        if len(note) > 0:
            image_name += "_note=" + note
            image_name_proc += "_note=" + note

        return [image_name, image_name_proc]


    def saveImage(self, image, folder_path, file_name):
        """ Save an image in a given folder with a given name,
            with .png extension """

        file_path = os.path.join(folder_path, file_name + ".png")
        alreadyExsist = False
        if os.path.exists(file_path):
            alreadyExsist = True
            ret = QMessageBox.question(self,
                        'Overwrite image',
                        "The image already exists. Do you want to overwrite it?",
                        QMessageBox.Yes | QMessageBox.No)
            if (ret == QMessageBox.No):
                return [alreadyExsist, False]
            else:
                os.remove(file_path)
                logging.info(file_name + ".png image removed")
        try:
            image.save(file_path)
            logging.info(file_name + ".png image correctly saved")
            return [alreadyExsist, True]
        except:
            logging.error("Cannot save " + file_name + ".png image")
            traceback.print_exc(file=sys.stdout)
            return [alreadyExsist, False]


    def showImage(self, image):
        """ Show an image in the expected UI area """

        try:
            self.qimage  = ImageQt(image)
            self.qpixmap = QtGui.QPixmap.fromImage(self.qimage)
            self.scene.clear()
            self.scene.addPixmap(self.qpixmap)
            self.graphicsViewImage.update()
        except:
            traceback.print_exc(file=sys.stdout)
            pass

    def resetBevelMeasures(self):
        """ Reset text browser """
        self.textBrowserBevelMeasures.clear()
        app.processEvents()

    def fillBevelMeasuresTextBox(self):
        measures = Htt.getMeasures()
        m = ""
        if not self.calibrationState:
            m += "*** WARNING: SYSTEM NOT CALIBRATED ***\n"
        m += "Bevel measures:\n"
        m += "B: " + str(round(measures[1],3)) + " mm\n"
        m += "M: " + str(round(measures[3],3)) + " mm\n"
        m += "D: " + str(round(measures[4],3)) + " mm\n"
        self.textBrowserBevelMeasures.setText(m)
        app.processEvents()

    def fillBevelMeasuresTextBoxWithRepetition(self, arrayB, arrayM, arrayD):
        arrayIndexSorted = np.argsort(arrayB)
        medianArrayIndexSorted = statistics.median(arrayIndexSorted)
        indexMedianB = arrayIndexSorted[medianArrayIndexSorted]

        text = ""
        if not self.calibrationState:
            text += "*** WARNING: SYSTEM NOT CALIBRATED ***\n"
        text += "Bevel measures: median (min, max) [mm]\n"
        text += "B: " + str(round(arrayB[indexMedianB], 3)) + "\t (" + str(round(min(arrayB), 3)) + "\t" + str(round(max(arrayB), 3)) + ")\n"
        text += "M: " + str(round(arrayM[indexMedianB], 3)) + "\t (" + str(round(min(arrayM), 3)) + "\t" + str(round(max(arrayM), 3)) + ")\n"
        text += "D: " + str(round(arrayD[indexMedianB], 3)) + "\t (" + str(round(min(arrayD), 3)) + "\t" + str(round(max(arrayD), 3)) + ")\n"
        text += "\n"
        self.textBrowserBevelMeasures.setText(text)
        app.processEvents()
        return indexMedianB

    def grabShowSaveImage(self, enable_message_box=False):
        """ It manage the image grabbing and the corresponding showing and saving """
        self.resetBevelMeasures()

        # If the illumination peripheral is connected
        # and the LED mustn't be always on
        if self.illumination_connected and \
                not self.checkBoxIlluminationContinuouslyOn.isChecked():
            # Switch the illumination LED on
            value = self.spinBoxIlluminationPower.value()
            cmd = "SET_PWR: " + str(value) + "\r"
            try:
                self.illumination_serial.write(cmd.encode())

                # Save the value set
                self.parameters.IlluminationLEDPower = value
            except:
                traceback.print_exc(file=sys.stdout)
                pass

        # Wait for a while before grabbing,
        # to allow a proper illumination...
        sleep(self.parameters.AcquisitionLEDOnTime / 1000)

        # if self.lineEditImageLoaded.text() == "":
        #     # (Try to) Grab the image
        #     ret, image = self.cam_software_manager.grabCameraFrame(debug=False)
        #     # Convert image into numpy array
        #     image_np = np.array(image)
        # else:
        #     # Load image 
        #     image_np = imageio.imread(self.lineEditImageLoaded.text())
        #     ret = HTTCameraResultCode.HTT_CAM_SUCCESSFUL
        # (Try to) Grab the image
        ret, image = self.cam_software_manager.grabCameraFrame(debug=False)

        if self.checkBoxProcessingImage.isChecked():
            # Convert image into numpy array
            image_np = np.array(image)

            # #XXX (ONLY FOR TEST) Overwrite image with test image
            # ret = HTTCameraResultCode.HTT_CAM_SUCCESSFUL
            # image_tmp = "test.png"
            # image_np = imageio.imread(image_tmp)

            # Update the processing icon (green)
            self.labelProcessingIcon.setPixmap(QtGui.QPixmap(":/Icons/Icons/Green-circle-small.png"))
            app.processEvents()

            # Call Htt.process and convert float numpy.array into uint8
            deltaN = self.doubleSpinBoxStilusAngleDeltaN.value()
            H = self.doubleSpinBoxStilusAngleH.value()
            try:
                modelType = self.comboBoxModelSelected.currentIndex()
                if self.checkBoxProcessingImageRepetitions.isChecked():
                    # print("process manual grab - repetition")
                    # Repetitions enabled
                    rep = self.spinBoxRepetitions.value()
                    arrayB = []
                    arrayM = []
                    arrayD = []
                    arrayImages = []
                    for i in range(rep):
                        image_proc_np = Htt.process(image_np, SETTINGS_FILE_PATH, -deltaN, H, False, modelType)
                        if len(image_proc_np) == 0:
                            self.textBrowserBevelMeasures.setText("Image processing failed")
                            self.imageProcessingSucceded = False
                            image_proc_np = image_np
                            break
                        else:
                            measures = Htt.getMeasures()
                            arrayB.append(measures[1])
                            arrayM.append(measures[3])
                            arrayD.append(measures[4])
                            arrayImages.append(image_proc_np)
                            i += 1
                    indexArrayB = self.fillBevelMeasuresTextBoxWithRepetition(arrayB, arrayM, arrayD)
                    image_proc = Image.fromarray(arrayImages[indexArrayB])
                    self.imageProcessingSucceded = True
                else:
                        # print("process manual grab - single shot")
                        image_proc_np = Htt.process(image_np, SETTINGS_FILE_PATH, deltaN, H, False, modelType)
                        if len(image_proc_np) == 0:
                            self.textBrowserBevelMeasures.setText("Image processing failed")
                            self.imageProcessingSucceded = False
                            image_proc_np = image_np
                        else: 
                            image_proc = Image.fromarray(image_proc_np)
                            self.imageProcessingSucceded = True

                            # Get optimization engine measure
                            self.fillBevelMeasuresTextBox()
            except:
                traceback.print_exc(file=sys.stdout)
                self.imageProcessingSucceded = False
                image_proc_np = image_np

            # Update the processing icon (black)
            self.labelProcessingIcon.setPixmap(QtGui.QPixmap(":/Icons/Icons/Black-circle-small.png"))
            app.processEvents()

        # If the illumination peripheral is connected
        # and the LED mustn't be always on
        if self.illumination_connected and \
                not self.checkBoxIlluminationContinuouslyOn.isChecked():
            # Switch the illumination LED off
            cmd = "SET_PWR: 0\r"
            try:
                self.illumination_serial.write(cmd.encode())
            except:
                traceback.print_exc(file=sys.stdout)
                pass

        # Save the image and show it within the application window
        if HTTCameraResultCode.HTT_CAM_SUCCESSFUL == ret:
            # Show the image
            if not self.checkBoxProcessingImage.isChecked():
                self.showImage(image)
            elif self.imageProcessingSucceded == True:
                self.showImage(image_proc)
            # Increment progressive
            self.spinBoxFrameProgressive.setValue(self.spinBoxFrameProgressive.value() + 1)
            # Save the image
            [image_name, image_name_proc] = self.assembleImageName()
            [overwrite, ret] = self.saveImage(image, self.lineEditImageSavingFolder.text(), image_name)
            if self.checkBoxProcessingImage.isChecked() and self.imageProcessingSucceded == True:
                self.saveImage(image_proc, self.lineEditImageSavingFolder.text(), image_name_proc)
            if ret:
                saveStr = "Image correctly saved" if overwrite == False else "Image correctly updated"
                # Show successful result in the label
                self.labelGrabbingState.setStyleSheet("QLabel {color: green}")
                self.labelGrabbingState.setText(saveStr)
                self.grab_label_timer.stop()
                self.grab_label_timer.start(2000)
                # Save saving folder path
                self.parameters.AcquisitionSavingFolder = self.lineEditImageSavingFolder.text()
            else:
                if (overwrite == False):
                    if enable_message_box:
                        QMessageBox.critical(self,
                                            'Saving problem',
                                            "Unable to save the image.\n\nCheck saving folder path.",
                                            QMessageBox.Ok)
                    # Show not successful result in the label
                    self.labelGrabbingState.setStyleSheet("QLabel {color: red}")
                    self.labelGrabbingState.setText("Cannot save the image. Check saving folder path.")
                    self.grab_label_timer.stop()
                    self.grab_label_timer.start(2000)
                # else: (overwrite==True) -> The user has decided not to overwrite the image 
        elif HTTCameraResultCode.HTT_CAM_PROCESS_ERROR == ret:
            if enable_message_box:
                QMessageBox.critical(self,
                                     'A problem occurred',
                                     "Unable to grab the image.\n\nTry to launch camera software again.",
                                     QMessageBox.Ok)
            # Show not successful result in the label
            self.labelGrabbingState.setStyleSheet("QLabel {color: red}")
            self.labelGrabbingState.setText("Cannot save the image. Try to launch camera software again.")
            self.grab_label_timer.stop()
            self.grab_label_timer.start(2000)
        elif HTTCameraResultCode.HTT_CAM_APP_WND_NOT_FOCUS == ret:
            if enable_message_box:
                QMessageBox.critical(self,
                                     'A problem occurred',
                                     "Unable to grab the image.\n\nKeep the application window selected.",
                                     QMessageBox.Ok)
            # Show not successful result in the label
            self.labelGrabbingState.setStyleSheet("QLabel {color: red}")
            self.labelGrabbingState.setText("Cannot save the image. Keep the application window selected.")
            self.grab_label_timer.stop()
            self.grab_label_timer.start(2000)
        else:
            if enable_message_box:
                QMessageBox.critical(self,
                                     'A problem occurred',
                                     "Unable to grab the image.\n\nA generic error occurred.",
                                     QMessageBox.Ok)
            # Show not successful result in the label
            self.labelGrabbingState.setStyleSheet("QLabel {color: red}")
            self.labelGrabbingState.setText("Cannot save the image. A generic error occurred.")
            self.grab_label_timer.stop()
            self.grab_label_timer.start(2000)

        app.processEvents()




    def createSerialPortSettingChoices(self):
        """ Fill the lists to be used to fill the serial port setting combo boxes """

        choices = SerialPortSettingChoices()
        choices.BitsPerSecondChoices = [75, 110, 134, 150, 300, 600, 1200, 1800, 2400, 4800, 7200, 9600, 14400, 19200, 38400, 57600, 115200, 128000]
        choices.DataBitsChoices      = [4, 5, 6, 7, 8]
        choices.ParityChoices        = ['N', 'E', 'O', 'M', 'S']
        choices.StopBitsChoices      = [1, 1.5, 2]
        choices.FlowControlChoices   = []

        return choices

    def createCalibrationFrameChoices(self):
        """ Fill the list to be used to fill the calibration frame combo box """
        choices = []
        for frame in self.jsonSettings["scheimpflug_transformation_calibration"]["frames"].toObject():
            choices.append(frame)
        return choices

    def updateCalibrationFrameParams(self):
        calibrationFrame = self.jsonSettings["scheimpflug_transformation_calibration"]["frames"][self.comboBoxCalibrationFrame.currentText()].toObject()
        self.doubleSpinBoxCalibrationB.setValue(calibrationFrame["B"].toDouble())
        self.doubleSpinBoxCalibrationM.setValue(calibrationFrame["M"].toDouble())
        self.doubleSpinBoxCalibrationAngle1.setValue(calibrationFrame["angle1"].toDouble())
        self.doubleSpinBoxCalibrationAngle2.setValue(calibrationFrame["angle2"].toDouble())
        self.lineEditCalibrationImage.setText(calibrationFrame["image"].toString())

    def createCalibrationTextOutput(self, calibrated):
        if not calibrated:
            st_chart = self.jsonSettings["scheimpflug_transformation_chart"].toObject()
            text = "*** SYSTEM NOT CALIBRATED ***\n\n"
            text += "SCHEIMPFLUG TRANSFORMATION FROM CHART\n"
            text += "alpha: " + str(st_chart["alpha"].toDouble()) + "\t  "
            text += "beta: " + str(st_chart["beta"].toDouble()) + "\n"
            text += "cx0: " + str(st_chart["cx0"].toDouble()) + "\t  "
            text += "cy0: " + str(st_chart["cy0"].toDouble()) + "\n"
            text += "delta: " + str(st_chart["delta"].toDouble()) + "\t  "
            text += "hFoV: " + str(st_chart["hFoV"].toDouble()) + "\n"
            text += "hs: " + str(st_chart["hs"].toDouble()) + "\t  "
            text += "magnification: " + str(st_chart["magnification"].toDouble()) + "\n"
            text += "p1: " + str(st_chart["p1"].toDouble()) + "\t  "
            text += "p2: " + str(st_chart["p2"].toDouble()) + "\n"
            text += "phi: " + str(st_chart["phi"].toDouble()) + "\t  "
            text += "px: " + str(st_chart["px"].toDouble()) + "\n"
            text += "pxFoV: " + str(st_chart["pxFoV"].toDouble()) + "\t  "
            text += "theta: " + str(st_chart["theta"].toDouble()) + "\n"
            text += "tt: " + str(st_chart["tt"].toDouble()) + "\n"
        else:
            st = self.jsonSettings["scheimpflug_transformation"].toObject()
            text = "SCHEIMPFLUG TRANSFORMATION FROM CALIBRATION\n"
            text += "alpha: " + str(round(st["alpha"].toDouble(),3)) + "\t  "
            text += "beta: " + str(round(st["beta"].toDouble(),3)) + "\n"
            text += "cx0: " + str(st["cx0"].toDouble()) + "\t  "
            text += "cy0: " + str(st["cy0"].toDouble()) + "\n"
            text += "delta: " + str(round(st["delta"].toDouble(),3)) + "\t  "
            text += "hFoV: " + str(round(st["hFoV"].toDouble(),3)) + "\n"
            text += "hs: " + str(st["hs"].toDouble()) + "\t  "
            text += "magnification: " + str(round(st["magnification"].toDouble(),3)) + "\n"
            text += "p1: " + str(st["p1"].toDouble()) + "\t  "
            text += "p2: " + str(st["p2"].toDouble()) + "\n"
            text += "phi: " + str(st["phi"].toDouble()) + "\t  "
            text += "px: " + str(st["px"].toDouble()) + "\n"
            text += "pxFoV: " + str(st["pxFoV"].toDouble()) + "\t  "
            text += "theta: " + str(round(st["theta"].toDouble(),3)) + "\n"
            text += "tt: " + str(round(st["tt"].toDouble(),3)) + "\n"
            text += "\n"
            text += "Reference Frame: " + st["frame"].toString() + "\n"
        self.textBrowserCalibrationOutput.setText(text)

    def createValidationTextOutput(self):
        text = ""
        if not self.calibrationState:
            text += "*** SYSTEM NOT CALIBRATED ***\n\n"
        stc = self.jsonSettings["scheimpflug_transformation_calibration"].toObject()
        frame = stc["frames"][self.comboBoxCalibrationFrame.currentText()].toObject()
        text += "REAL BEVEL MEASURES\n"
        text += "B: " + str(frame["B"].toDouble()) + " mm\t"
        text += "M: " + str(frame["M"].toDouble()) + " mm\n"
        text += "angle1: " + str(frame["angle1"].toDouble()) + "°\t"
        text += "angle2: " + str(frame["angle2"].toDouble()) + "°\n"
        text += "\n"
        measures = Htt.getMeasures()
        text += "COMPUTED BEVEL MEASURES\n"
        text += "B: " + str(round(measures[2],3)) + " mm\t"
        text += "M: " + str(round(measures[3],3)) + " mm\n"
        text += "\n"
        text += "ERROR\n"
        text +="B: " + str(round((frame["B"].toDouble() - measures[2])*1000, 3)) + " um\t"
        text +="M: " + str(round((frame["M"].toDouble() - measures[3])*1000, 3)) + " um\n"
        self.textBrowserValidationOutput.setText(text)

    def createValidationTextOutputWithRepetitions(self, arrayB, arrayM, arrayD):
        arrayIndexSorted = np.argsort(arrayB)
        medianArrayIndexSorted = statistics.median(arrayIndexSorted)
        indexMedianB = arrayIndexSorted[medianArrayIndexSorted]

        text = ""
        if not self.calibrationState:
            text += "*** SYSTEM NOT CALIBRATED ***\n\n"
        stc = self.jsonSettings["scheimpflug_transformation_calibration"].toObject()
        frame = stc["frames"][self.comboBoxCalibrationFrame.currentText()].toObject()
        text += "REAL BEVEL MEASURES\n"
        text += "B: " + str(frame["B"].toDouble()) + " mm\t"
        text += "M: " + str(frame["M"].toDouble()) + " mm\n"
        text += "angle1: " + str(frame["angle1"].toDouble()) + "°\t"
        text += "angle2: " + str(frame["angle2"].toDouble()) + "°\n"
        text += "\n"
        text += "COMPUTED BEVEL MEASURES: median (min, max) [mm]\n"
        text += "B: " + str(round(arrayB[indexMedianB], 3)) + "\t (" + str(round(min(arrayB), 3)) + "\t" + str(round(max(arrayB), 3)) + ")\n"
        text += "M: " + str(round(arrayM[indexMedianB], 3)) + "\t (" + str(round(min(arrayM), 3)) + "\t" + str(round(max(arrayM), 3)) + ")\n"
        # text += "D: " + str(round(arrayD[indexMedianB], 6)) + "\t (" + str(round(min(arrayD), 6)) + "\t" + str(round(max(arrayD), 6)) + ")\n"
        text += "\n"
        text += "ERRORS: median (min, max) [um]\n"
        B = frame["B"].toDouble()
        M = frame["M"].toDouble()
        text += "B: " + str(round((B - arrayB[indexMedianB])*1000, 1)) + "\t (" + str(round((B - min(arrayB))*1000, 1)) + "\t"
        text += str(round((B - max(arrayB))*1000, 1)) + ")\n"
        text += "M: " + str(round((M - arrayM[indexMedianB])*1000, 1)) + "\t (" + str(round((M - min(arrayM))*1000, 1)) + "\t"
        text += str(round((M - max(arrayM))*1000, 1)) + ")\n"
        self.textBrowserValidationOutput.setText(text)
        return indexMedianB

    def createProcessTestImageTextOutput(self, modelType):
        text = ""
        if not self.calibrationState:
            text += "*** SYSTEM NOT CALIBRATED ***\n\n"
        measures = Htt.getMeasures()
        text += "COMPUTED BEVEL MEASURES\n"
        if modelType == 0:  # MiniBevelModel
            text += "B: " + str(round(measures[2],3)) + " mm\n"
            text += "M: " + str(round(measures[3],3)) + " mm\n"
            text += "D: " + str(round(measures[4],3)) + " mm\n"
        elif modelType == 1:  # TBevelModel
            text += "B: " + str(round(measures[2],3)) + " mm\n"
            text += "M: " + str(round(measures[3],3)) + " mm\n"
            text += "D: " + str(round(measures[4],3)) + " mm\n"
        else: # CustomBevelModel
            text += "D: " + str(round(measures[2],3)) + " mm\n"
            text += "E: " + str(round(measures[3],3)) + " mm\n"
            text += "F: " + str(round(measures[4],3)) + " mm\n"
            text += "G: " + str(round(measures[5],3)) + " mm\n"
        
        self.textBrowserTestImageOutput.setText(text)

    def createProcessTestImageTextOutputWithRepetitions(self, modelType, array1, array2, array3, array4):
        arrayIndexSorted = np.argsort(array1)
        medianArrayIndexSorted = statistics.median(arrayIndexSorted)
        indexMedian = arrayIndexSorted[medianArrayIndexSorted]

        text = ""
        if not self.calibrationState:
            text += "*** SYSTEM NOT CALIBRATED ***\n\n"
        text += "COMPUTED BEVEL MEASURES: median (min, max) [mm]\n"
        if modelType == 0:  # MiniBevelModel
            text += "B: " + str(round(array1[indexMedian], 3)) + "\t (" + str(round(min(array1), 3)) + "\t" + str(round(max(array1), 3)) + ")\n"
            text += "M: " + str(round(array2[indexMedian], 3)) + "\t (" + str(round(min(array2), 3)) + "\t" + str(round(max(array2), 3)) + ")\n"
            text += "D: " + str(round(array3[indexMedian], 3)) + "\t (" + str(round(min(array3), 3)) + "\t" + str(round(max(array3), 3)) + ")\n"
        elif modelType == 1:  # TBevelModel
            text += "B: " + str(round(array1[indexMedian], 3)) + "\t (" + str(round(min(array1), 3)) + "\t" + str(round(max(array1), 3)) + ")\n"
            text += "M: " + str(round(array2[indexMedian], 3)) + "\t (" + str(round(min(array2), 3)) + "\t" + str(round(max(array2), 3)) + ")\n"
            text += "D: " + str(round(array3[indexMedian], 3)) + "\t (" + str(round(min(array3), 3)) + "\t" + str(round(max(array3), 3)) + ")\n"
        else: # CustomBevelModel
            text += "D: " + str(round(array1[indexMedian], 3)) + "\t (" + str(round(min(array1), 3)) + "\t" + str(round(max(array1), 3)) + ")\n"
            text += "E: " + str(round(array2[indexMedian], 3)) + "\t (" + str(round(min(array2), 3)) + "\t" + str(round(max(array2), 3)) + ")\n"
            text += "F: " + str(round(array3[indexMedian], 3)) + "\t (" + str(round(min(array3), 3)) + "\t" + str(round(max(array3), 3)) + ")\n"
            text += "G: " + str(round(array4[indexMedian], 3)) + "\t (" + str(round(min(array4), 3)) + "\t" + str(round(max(array4), 3)) + ")\n"
        self.textBrowserTestImageOutput.setText(text)
        return indexMedian

if __name__ == "__main__":

    ### Logging initialization
    if not os.path.exists(APP_LOGGING_FOLDER):
        os.makedirs(APP_LOGGING_FOLDER)
    logging.basicConfig( \
        level=APP_LOGGING_LEVEL, \
        format='%(asctime)s\t%(levelname)s\t%(filename)s\t%(funcName)s\t%(message)s', \
        filename=os.path.join(APP_LOGGING_FOLDER, APP_LOGGING_FILE), \
        filemode='a')


    app = QtWidgets.QApplication(sys.argv)

    # Show splash screen
    splash = QtWidgets.QSplashScreen(QtGui.QPixmap(":/Icons/Icons/NIDEK.ico"))
    splash.show()

    # Start/show the application
    window = HTTImagingManagerApp()
    window.show()

    # Close the splash
    splash.finish(window)

    try:
        sys.exit(app.exec_())
    except Exception as e:
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        logging.error("Exception!!!: " + "Type: " + str(exc_type) + ", File name: " + fname + ", Line number: " + str(exc_tb.tb_lineno))

