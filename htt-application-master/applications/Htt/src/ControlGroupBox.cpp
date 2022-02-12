#include "ControlGroupBox.h"
#include "HttUtils.h"
#include <QDebug>
#include <QLabel>
#include <QTimer>

using namespace Nidek::Solution::HTT::UserApplication;

ControlGroupBox::ControlGroupBox(shared_ptr<HwController> hwController, QWidget* parent /*= Q_NULLPT*/)
    : m_hwController(hwController),
      m_grid(new QGridLayout),
      m_cameraGrid(new QGridLayout),
      m_ledGrid(new QGridLayout),
      m_processingGrid(new QGridLayout),
      m_cameraControlGroupBox(new QGroupBox("CAMERA")),
      m_ledControlGroupBox(new QGroupBox("LED")),
      m_processingGroupBox(new QGroupBox("PROCESSING")),
      m_cameraIsRunning(false)
//   m_imageAcquisitionRequest(false)
{
    this->setLayout(m_grid);
    this->setTitle("CONTROLS");

    m_grid->addWidget(m_cameraControlGroupBox, 0, 0, 1, -1);
    m_grid->addWidget(m_ledControlGroupBox, 1, 0, 1, -1);
    m_grid->addWidget(m_processingGroupBox, 2, 0, 1, -1);

    // CAMERA
    m_cameraControlGroupBox->setLayout(m_cameraGrid);

    // CheckBox used to start/stop camera
    m_checkBoxLiveCamera = new QCheckBox("Start/stop acquisition");
    m_checkBoxLiveCamera->setChecked(false);
    m_cameraGrid->addWidget(m_checkBoxLiveCamera, 0, 0, 1, 3);
    connect(m_checkBoxLiveCamera, SIGNAL(clicked(bool)), this, SLOT(checkBoxLiveCameraClicked(bool)));

    // FIXME: ComboBox used to select the bevel used to perfrom the processing
    m_bevelsComboBox = new QComboBox();
    QStringList itemsList = (QStringList() << "miniBevel"
                                           << "customBevel"
                                           << "TBevel");
    m_bevelsComboBox->addItems(itemsList);
    m_cameraGrid->addWidget(m_bevelsComboBox, 0, 3, 1, 2);
    connect(m_bevelsComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(bevelComboBoxCurrentIndexChanged(int)));

    // CheckBox used to start/stop the real time processing
    m_checkBoxProcessingRealTime = new QCheckBox("Processing real time");
    m_cameraGrid->addWidget(m_checkBoxProcessingRealTime, 0, 5, 1, 1);
    connect(m_checkBoxProcessingRealTime, SIGNAL(stateChanged(int)), this,
            SLOT(checkBoxProcessingRealTimeChanged(int)));

    // SpinBox used to manage the camera gain
    m_gainSpinBox = new QSpinBox(0);
    m_gainSpinBox->setEnabled(false);
    m_gainSpinBox->setRange(0, 63);
    m_gainSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_gainSpinBox->setValue(m_hwController->cameraGain());

    m_targetGainSpinBox = new QSpinBox();
    m_targetGainSpinBox->setRange(1, 63);

    m_cameraGrid->addWidget(new QLabel("Gain"), 1, 0, 1, 2);
    m_cameraGrid->addWidget(m_gainSpinBox, 1, 2, 1, 1);
    m_cameraGrid->addWidget(new QLabel("Admitted input range [1-63]"), 2, 0, 1, 2);
    m_cameraGrid->addWidget(m_targetGainSpinBox, 2, 2, 1, 1);

    m_targetGainSpinBox->setKeyboardTracking(false);
    connect(m_targetGainSpinBox, SIGNAL(valueChanged(int)), this, SLOT(gainSpinBloxValueChanged()));

    // SpinBox used to manage the camera exposure time
    m_exposureTimeSpinBox = new QSpinBox(0);
    m_exposureTimeSpinBox->setEnabled(false);
    m_exposureTimeSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_exposureTimeSpinBox->setValue(m_hwController->exposureTime());

    m_targetExposureTimeSpinBox = new QSpinBox();
    m_targetExposureTimeSpinBox->setRange(1, 33);
    m_targetExposureTimeSpinBox->setValue(33);

    // m_cameraGrid->addWidget(new QLabel("Exposure Time [ms]\nRange [1-33]"), 1, 3, 1, 1);
    // m_cameraGrid->addWidget(m_exposureTimeSpinBox, 1, 4, 1, 1);
    // m_cameraGrid->addWidget(m_targetExposureTimeSpinBox, 1, 5, 1, 1);
    m_cameraGrid->addWidget(new QLabel("Exposure Time [ms]"), 1, 3, 1, 2);
    m_cameraGrid->addWidget(m_exposureTimeSpinBox, 1, 5, 1, 1);
    m_cameraGrid->addWidget(new QLabel("Admitted input range [1-33]"), 2, 3, 1, 2);
    m_cameraGrid->addWidget(m_targetExposureTimeSpinBox, 2, 5, 1, 1);

    m_targetExposureTimeSpinBox->setKeyboardTracking(false);
    connect(m_targetExposureTimeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(exposureTimeSpinBoxValueChanged()));

    // PushButton used to perform a manual grab
    m_pushButtonManualGrab = new QPushButton("Manual Grab");
    // m_pushButtonManualGrab->setVisible(false);
    // m_pushButtonManualGrab->setEnabled(false);
    m_cameraGrid->addWidget(m_pushButtonManualGrab, 3, 0, 1, 3);
    connect(m_pushButtonManualGrab, SIGNAL(clicked(bool)), this, SLOT(pushButtonClickedManualGrab(bool)));

    // PushButton used to perform a manual lens fitting
    m_pushButtonManualLensFitting = new QPushButton("Manual Lens Fitting");
    m_cameraGrid->addWidget(m_pushButtonManualLensFitting, 3, 3, 1, 3);
    connect(m_pushButtonManualLensFitting, SIGNAL(clicked(bool)), this, SLOT(pushButtonClickedManualLensFitting(bool)));

    // PushButton used to perform a pre-processing and generare a profile
    m_pushButtonProfileDetection = new QPushButton("Profile Detection");
    m_cameraGrid->addWidget(m_pushButtonProfileDetection, 3, 3, 1, 3);
    connect(m_pushButtonProfileDetection, SIGNAL(clicked(bool)), this, SLOT(pushButtonClickedProfileDetection(bool)));

    // // PushButton used to acquire a single frame
    // m_pushButtonAcquireImage = new QPushButton("Acquire Image");
    // m_cameraGrid->addWidget(m_pushButtonAcquireImage, 2, 0, 1, -1);
    // connect(m_pushButtonAcquireImage, SIGNAL(clicked(bool)), this, SLOT(pushButtonClickedAcquireImage(bool)));

    // LED
    m_ledControlGroupBox->setLayout(m_ledGrid);

    // Load JSON parameters for th HW board
    m_hwBoard = HttUtils::loadJsonFile("config/hwBoard.json");
    QJsonObject led = m_hwBoard["led"].toObject();
    m_pCwThreshold = led["Pcw"].toInt();
    m_pLatchThreshold = led["Platch"].toInt();
    m_pNullThreshold = led["Pnull"].toInt();
    m_initLedMode = QString("Operation mode: [1 - %1] cw / [%2 - %3] latch / [%4 - %5] triggered")
                        .arg(m_pCwThreshold)
                        .arg(m_pCwThreshold + 1)
                        .arg(m_pLatchThreshold)
                        .arg(m_pLatchThreshold + 1)
                        .arg(m_pNullThreshold);

    // CheckBox used to start/stop the led
    m_checkBoxStartStopLed = new QCheckBox("On/Off");
    m_ledGrid->addWidget(m_checkBoxStartStopLed, 0, 0, 1, 2);
    m_clock = new QTimer();
    m_ledGrid->addWidget(&m_countdownLabel, 0, 1, 1, 1);
    m_ledOperationMode = new QLabel(m_initLedMode);
    m_ledGrid->addWidget(m_ledOperationMode, 0, 2, 1, 4);
    connect(m_checkBoxStartStopLed, SIGNAL(clicked(bool)), this, SLOT(checkBoxStartStopLedClicked(bool)));
    connect(m_clock, SIGNAL(timeout()), this, SLOT(updateCountdown()));

    // DoubleSpinBox used to manage the current led power
    m_currentLedPowerDoubleSpinBox = new QDoubleSpinBox(0);
    m_currentLedPowerDoubleSpinBox->setRange(0, 100);
    m_currentLedPowerDoubleSpinBox->setDecimals(0);
    m_currentLedPowerDoubleSpinBox->setEnabled(false);
    m_currentLedPowerDoubleSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_currentLedPowerDoubleSpinBox->setValue(m_hwController->ledPower());

    m_targetLedPowerDoubleSpinBox = new QDoubleSpinBox();
    m_targetLedPowerDoubleSpinBox->setRange(1, led["Pnull"].toInt());
    m_targetLedPowerDoubleSpinBox->setDecimals(0);
    m_targetLedPowerDoubleSpinBox->setValue(led["default_power"].toInt());

    m_ledGrid->addWidget(new QLabel("Current power [%]"), 1, 0, 1, 2);
    m_ledGrid->addWidget(m_currentLedPowerDoubleSpinBox, 1, 2, 1, 1);
    m_ledGrid->addWidget(new QLabel("Target  power [%]"), 1, 3, 1, 2);
    m_ledGrid->addWidget(m_targetLedPowerDoubleSpinBox, 1, 5, 1, 1);

    m_targetLedPowerDoubleSpinBox->setKeyboardTracking(false);
    connect(m_targetLedPowerDoubleSpinBox, SIGNAL(valueChanged(double)), this,
            SLOT(ledPowerDoubleSpinBloxValueChanged()));

    // // Initialize gain and exposure time values for the camera
    // m_gainSpinBox->setValue(m_hwController->cameraGain());
    // m_exposureTimeSpinBox->setValue(m_hwController->exposureTime());

    // // Load JSON parameters for th HW board
    // m_hwBoard = HttUtils::loadJsonFile("config/hwBoard.json");

    // // LED
    // QJsonObject led = m_hwBoard["led"].toObject();
    // m_currentLedPowerDoubleSpinBox->setValue(led["default_power"].toInt());
    // m_targetLedPowerDoubleSpinBox->setValue(led["default_power"].toInt());

    // PROCESSING
    m_processingGroupBox->setLayout(m_processingGrid);
    // Load JSON parameters
    QJsonObject calibrationJson = HttUtils::loadJsonFile("config/calibration.json");
    QJsonObject htt = calibrationJson["htt"].toObject();
    QJsonObject deltaN = htt["deltaN"].toObject();
    QJsonObject H = htt["H"].toObject();

    m_deltaNLabel = new QLabel("deltaN");
    m_deltaNSpinBox = new QDoubleSpinBox();
    m_deltaNSpinBox->setRange(deltaN["min"].toDouble(), deltaN["max"].toDouble());
    m_deltaNSpinBox->setDecimals(1);

    m_hLabel = new QLabel("H");
    m_hSpinBox = new QDoubleSpinBox();
    m_hSpinBox->setRange(H["min"].toDouble(), H["max"].toDouble());
    m_hSpinBox->setDecimals(1);
    m_hSpinBox->setValue(11.0);

    m_processingGrid->addWidget(m_hLabel, 0, 0, 1, 2);
    m_processingGrid->addWidget(m_hSpinBox, 0, 2, 1, 1);
    m_processingGrid->addWidget(m_deltaNLabel, 0, 3, 1, 3);
    m_processingGrid->addWidget(m_deltaNSpinBox, 0, 5, 1, 1);

    m_startCalibrationButton = new QPushButton("Start Calculation");
    m_processingGrid->addWidget(m_startCalibrationButton, 1, 0, 1, -1);
    connect(m_startCalibrationButton, SIGNAL(clicked(bool)), this, SLOT(pushButtonClickedStartCalibration(bool)));
}

ControlGroupBox::~ControlGroupBox()
{
    if (m_clock)
    {
        if (m_clock->isActive())
            m_clock->stop();

        delete m_clock;
    }
}

float ControlGroupBox::getDeltaN()
{
    return m_deltaNSpinBox->value();
}
float ControlGroupBox::getH()
{
    return m_hSpinBox->value();
}

void ControlGroupBox::reset()
{
    // Force the stop of the camera
    if (m_cameraIsRunning)
        m_hwController->stopCamera();

    blockSignals(true);

    // GroupBox visibility
    this->setVisible(true);
    m_cameraControlGroupBox->setVisible(true);
    m_ledControlGroupBox->setVisible(true);
    m_processingGroupBox->setVisible(false);

    // Widgets default visibility
    m_checkBoxLiveCamera->setVisible(false);
    m_bevelsComboBox->setVisible(false);
    m_checkBoxProcessingRealTime->setVisible(false);
    m_pushButtonManualGrab->setVisible(false);
    m_pushButtonManualLensFitting->setVisible(false);
    m_pushButtonProfileDetection->setVisible(false);
    // m_pushButtonAcquireImage->setVisible(false);
    m_gainSpinBox->setVisible(false);
    m_targetGainSpinBox->setVisible(false);
    m_exposureTimeSpinBox->setVisible(false);
    m_targetExposureTimeSpinBox->setVisible(false);
    m_checkBoxStartStopLed->setVisible(false);
    m_countdownLabel.setVisible(false);
    m_targetLedPowerDoubleSpinBox->setVisible(false);
    m_currentLedPowerDoubleSpinBox->setVisible(false);
    m_startCalibrationButton->setVisible(false);
    m_hLabel->setVisible(false);
    m_hSpinBox->setVisible(false);
    m_deltaNLabel->setVisible(false);
    m_deltaNSpinBox->setVisible(false);

    // Checkbox default status
    m_checkBoxLiveCamera->setChecked(false);
    m_checkBoxProcessingRealTime->setChecked(false);
    m_checkBoxStartStopLed->setChecked(false);

    // Checkbox default enabled
    m_checkBoxProcessingRealTime->setEnabled(false);

    blockSignals(false);
}

/* private slots*/
void ControlGroupBox::setModality(Modality m)
{
    m_modality = m;
    qDebug() << "ControlGroupBox::setModality";

    reset();

    switch (m_modality)
    {
    case CALIBRATION_IDLE:
    case CALIBRATION_5:
    {
        this->setVisible(false);
        break;
    }
    case CALIBRATION_1:
    case CALIBRATION_7:
    {
        qDebug() << "MOD CtrlGrpBox: STEP1 or STEP7";
        m_cameraControlGroupBox->setVisible(false);
        m_checkBoxStartStopLed->setVisible(true);
        m_targetLedPowerDoubleSpinBox->setVisible(true);
        m_currentLedPowerDoubleSpinBox->setVisible(true);
        break;
    }
    case CALIBRATION_2:
    case CALIBRATION_3:
    {
        qDebug() << "MOD CtrlGrpBox: STEP2 or STEP3";
        m_checkBoxLiveCamera->setVisible(true);
        // m_pushButtonAcquireImage->setVisible(true);
        m_gainSpinBox->setVisible(true);
        m_targetGainSpinBox->setVisible(true);
        m_exposureTimeSpinBox->setVisible(true);
        m_targetExposureTimeSpinBox->setVisible(true);
        m_checkBoxStartStopLed->setVisible(true);
        m_targetLedPowerDoubleSpinBox->setVisible(true);
        m_currentLedPowerDoubleSpinBox->setVisible(true);
        break;
    }
    case CALIBRATION_4:
    {
        qDebug() << "MOD CtrlGrpBox: STEP4";
        // Enable correct groupBox
        m_cameraControlGroupBox->setVisible(false);
        m_ledControlGroupBox->setVisible(false);
        m_processingGroupBox->setVisible(true);
        m_startCalibrationButton->setVisible(true);
        break;
    }
    case CALIBRATION_6:
    {
        qDebug() << "MOD CtrlGrpBox: STEP6";
        m_checkBoxLiveCamera->setVisible(true);
        // m_pushButtonAcquireImage->setVisible(true);
        m_gainSpinBox->setVisible(true);
        m_targetGainSpinBox->setVisible(true);
        m_exposureTimeSpinBox->setVisible(true);
        m_targetExposureTimeSpinBox->setVisible(true);
        m_checkBoxStartStopLed->setVisible(true);
        m_targetLedPowerDoubleSpinBox->setVisible(true);
        m_currentLedPowerDoubleSpinBox->setVisible(true);
        break;
    }
    case CALIBRATION_8:
    {
        qDebug() << "MOD CtrlGrpBox: STEP8";
        m_cameraControlGroupBox->setVisible(true);
        m_checkBoxLiveCamera->setVisible(true);
        m_checkBoxStartStopLed->setVisible(true);
        m_gainSpinBox->setVisible(true);
        m_targetGainSpinBox->setVisible(true);
        m_exposureTimeSpinBox->setVisible(true);
        m_targetExposureTimeSpinBox->setVisible(true);
        m_checkBoxStartStopLed->setVisible(true);
        m_targetLedPowerDoubleSpinBox->setVisible(true);
        m_currentLedPowerDoubleSpinBox->setVisible(true);
        break;
    }
    case CALIBRATION_9:
    {
        qDebug() << "MOD CtrlGrpBox: STEP9";
        m_checkBoxLiveCamera->setVisible(true);
        m_gainSpinBox->setVisible(true);
        m_targetGainSpinBox->setVisible(true);
        m_exposureTimeSpinBox->setVisible(true);
        m_targetExposureTimeSpinBox->setVisible(true);
        m_checkBoxStartStopLed->setVisible(true);
        m_targetLedPowerDoubleSpinBox->setVisible(true);
        m_currentLedPowerDoubleSpinBox->setVisible(true);
        m_startCalibrationButton->setVisible(true);
        m_processingGroupBox->setVisible(true);
        m_hLabel->setVisible(true);
        m_hSpinBox->setVisible(true);
        m_deltaNLabel->setVisible(true);
        m_deltaNSpinBox->setVisible(true);
        break;
    }
    case CALIBRATION_10:
    {
        qDebug() << "MOD CtrlGrpBox: STEP10";
        m_checkBoxLiveCamera->setVisible(true);
        m_pushButtonManualGrab->setVisible(true);
        m_pushButtonProfileDetection->setVisible(true);
        m_gainSpinBox->setVisible(true);
        m_targetGainSpinBox->setVisible(true);
        m_exposureTimeSpinBox->setVisible(true);
        m_targetExposureTimeSpinBox->setVisible(true);
        m_checkBoxStartStopLed->setVisible(true);
        m_targetLedPowerDoubleSpinBox->setVisible(true);
        m_currentLedPowerDoubleSpinBox->setVisible(true);
        m_startCalibrationButton->setVisible(true);
        break;
    }
    case LIVE:
    {
        qDebug() << "MOD CtrlGrpBox: LIVE";
        m_checkBoxLiveCamera->setVisible(true);
        m_bevelsComboBox->setVisible(true);
        m_checkBoxProcessingRealTime->setVisible(true);
        m_pushButtonManualGrab->setVisible(true);
        m_pushButtonManualLensFitting->setVisible(true);
        m_gainSpinBox->setVisible(true);
        m_targetGainSpinBox->setVisible(true);
        m_exposureTimeSpinBox->setVisible(true);
        m_targetExposureTimeSpinBox->setVisible(true);
        m_checkBoxStartStopLed->setVisible(true);
        m_targetLedPowerDoubleSpinBox->setVisible(true);
        m_currentLedPowerDoubleSpinBox->setVisible(true);
        m_startCalibrationButton->setVisible(true);

        m_checkBoxProcessingRealTime->setEnabled(true);
        break;
    }
    default:
        qDebug() << "MOD CtrlGrpBox: DEFAULT";
        break;
    }
}

void ControlGroupBox::cameraStarted()
{
    // Update checkBox
    m_checkBoxLiveCamera->blockSignals(true);
    m_checkBoxLiveCamera->setChecked(true);
    m_checkBoxLiveCamera->blockSignals(false);
    emit doActiveFrameController(true);

    m_cameraIsRunning = true;
    emit doEnableProgressNextButton(false);
}

void ControlGroupBox::cameraStopped()
{
    m_cameraIsRunning = false;
    // Notify calibration wizard
    // emit doEnableProgressNextButton(m_imageAcquisitionRequest);
    emit doEnableProgressNextButton(true);

    // Update checkBox
    m_checkBoxLiveCamera->blockSignals(true);
    m_checkBoxLiveCamera->setChecked(false);
    m_checkBoxLiveCamera->blockSignals(false);
    emit doActiveFrameController(false);
}

void ControlGroupBox::cameraGainChanged()
{
    int value = m_hwController->cameraGain();
    qDebug() << "cameraGainChanged: " << QString::number(value);
    m_gainSpinBox->setValue(value);
}

void ControlGroupBox::exposureTimeChanged()
{
    int value = m_hwController->exposureTime();
    qDebug() << "exposureTimeChanged: " << QString::number(value);
    m_exposureTimeSpinBox->setValue(value);
}

void ControlGroupBox::ledPowerChanged(double power)
{
    qDebug() << "ledPowerChanged: " << QString::number(power);
    m_currentLedPowerDoubleSpinBox->setValue(power);
    if (power == 0)
        m_ledOperationMode->setText(m_initLedMode);
    else if (power <= m_pCwThreshold)
        m_ledOperationMode->setText("CW mode");
    else if (power <= m_pLatchThreshold)
        m_ledOperationMode->setText("Latch mode");
    else if (power <= m_pNullThreshold)
        m_ledOperationMode->setText("Triggered mode");
    else
        m_ledOperationMode->setText("Forbidden");
}
void ControlGroupBox::setWaitingInterval(int value)
{
    m_countdownTimer = (value + 500) / 1000;
    int min = m_countdownTimer / 60;
    int sec = m_countdownTimer % 60;
    m_clock->start(1000);

    m_countdownLabel.setVisible(true);
    QString s = QStringLiteral("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
    m_countdownLabel.setText(s);
}
void ControlGroupBox::ledTimeoutTimerExpired()
{
    // qDebug() << "GUI ledTimeoutTimerExpired";
    m_checkBoxStartStopLed->blockSignals(true);
    m_checkBoxStartStopLed->setChecked(false);
    m_checkBoxStartStopLed->setEnabled(false);
    m_checkBoxStartStopLed->blockSignals(false);
}
void ControlGroupBox::ledWaitingTimerExpired()
{
    // qDebug() << "GUI ledWaitingTimerExpired";
    m_checkBoxStartStopLed->blockSignals(true);
    m_checkBoxStartStopLed->setChecked(false);
    m_checkBoxStartStopLed->setEnabled(true);
    m_checkBoxStartStopLed->blockSignals(false);
    m_countdownLabel.setVisible(false);
}

void ControlGroupBox::checkBoxLiveCameraClicked(bool checked)
{
    if (checked)
    {
        emit doResetImageViewerController();
        emit doSetViewerModality(m_modality);

        if (!m_cameraIsRunning)
        {
            bool started = m_hwController->startCamera(HwController::GrabbingMode::ContinuousAcquisition);
            if (!started)
            {
                m_checkBoxLiveCamera->blockSignals(true);
                m_checkBoxLiveCamera->setChecked(false);
                m_checkBoxLiveCamera->blockSignals(false);
            }
        }
    } else
    {
        if (m_cameraIsRunning)
        {
            bool stopped = m_hwController->stopCamera();
            if (!stopped)
            {
                m_checkBoxLiveCamera->blockSignals(true);
                m_checkBoxLiveCamera->setChecked(true);
                m_checkBoxLiveCamera->blockSignals(false);
            }
        }

        m_checkBoxProcessingRealTime->blockSignals(true);
        m_checkBoxProcessingRealTime->setEnabled(false);
        m_checkBoxProcessingRealTime->blockSignals(false);
    }
}
void ControlGroupBox::bevelComboBoxCurrentIndexChanged(int index)
{
// TODO: how to inform the library?
#if 0
    qDebug() << index;
    stop();

    QString name = modelTypeToString((ModelType)index);
    m_hwController->setCameraImageMode(name);

    double H, deltaN;
    ModelType modelType;
    m_hwController->getCameraParams(name, H, deltaN, modelType);
    m_hSpinBox->setValue(H);
    m_deltaNSpinBox->setValue(deltaN);

    reset();
    start();
    // blockSignals(false);
    // m_hwController->startCamera(CameraDriver::ContinuousAcquisition);
    // emit doSetViewerModality(Modality::LIVE);
#endif
}
void ControlGroupBox::checkBoxProcessingRealTimeChanged(int state)
{
    emit doChangeProcessingRealTimeState(state == Qt::Checked);

    // // Initilize image viewer
    // if (state != Qt::Checked)
    //     emit doSetViewerModality(Modality::LIVE);
}
void ControlGroupBox::gainSpinBloxValueChanged()
{
    m_hwController->setCameraGain(m_targetGainSpinBox->value());
    // bool ret = m_hwController->setCameraGain(m_targetGainSpinBox->value());
    // if (ret)
    //     m_gainSpinBox->setValue(m_hwController->cameraGain());
}
void ControlGroupBox::exposureTimeSpinBoxValueChanged()
{
    m_hwController->setExposureTime(m_targetExposureTimeSpinBox->value());
    //     bool ret = m_hwController->setExposureTime(m_targetExposureTimeSpinBox->value());
    // if (ret)
    //     m_exposureTimeSpinBox->setValue(m_hwController->exposureTime());
}
void ControlGroupBox::pushButtonClickedManualGrab(bool)
{
    int gain = m_gainSpinBox->value();
    int expTime = m_exposureTimeSpinBox->value();
    int ledPower = (int)m_currentLedPowerDoubleSpinBox->value(); // FIXME: double or int?

    emit doManualGrab(gain, expTime, ledPower);
}
void ControlGroupBox::pushButtonClickedManualLensFitting(bool)
{
    emit doManualLensFitting(m_bevelsComboBox->currentIndex());
}
void ControlGroupBox::pushButtonClickedProfileDetection(bool)
{
    emit doDetectProfile();
}
// void ControlGroupBox::pushButtonClickedAcquireImage(bool)
// {
//     m_imageAcquisitionRequest = true;

//     // Force the stop of the camera
//     if (m_cameraIsRunning)
//         m_hwController->stopCamera();
// }
void ControlGroupBox::pushButtonClickedStartCalibration(bool)
{
    m_startCalibrationButton->setEnabled(false);
    if (m_modality == CALIBRATION_4)
        emit doStartGridCalibration();
    else if (m_modality == CALIBRATION_9)
        emit doStartAccurateCalibration(m_deltaNSpinBox->value(), m_hSpinBox->value());
}
void ControlGroupBox::checkBoxStartStopLedClicked(bool checked)
{
    if (checked)
    {
        double targetPower = m_targetLedPowerDoubleSpinBox->value();
        bool ret = m_hwController->setLedPower(targetPower);
        if (!ret)
        {
            m_checkBoxStartStopLed->blockSignals(true);
            m_checkBoxStartStopLed->setChecked(false);
            m_checkBoxStartStopLed->blockSignals(false);
        }
    } else
    {
        double currentPower = m_currentLedPowerDoubleSpinBox->value();
        bool ret = m_hwController->setLedPower(0);
        if (!ret)
        {
            m_checkBoxStartStopLed->blockSignals(true);
            m_checkBoxStartStopLed->setChecked(true);
            m_checkBoxStartStopLed->blockSignals(false);
        }
        // FIXME
        else
        {
            if (currentPower > m_pCwThreshold && currentPower <= m_pLatchThreshold)
            {
                m_checkBoxStartStopLed->setEnabled(false);
                m_countdownLabel.setVisible(true);
                // m_countdownLabel.setText("00:30.000");
            }
        }
    }
}
void ControlGroupBox::ledPowerDoubleSpinBloxValueChanged()
{
    double currentPower = m_currentLedPowerDoubleSpinBox->value();
    double targetPower = m_targetLedPowerDoubleSpinBox->value();
    QJsonObject led = m_hwBoard["led"].toObject();

    bool unlocked = false;

    if (currentPower <= m_pCwThreshold)
        unlocked = (targetPower <= m_pCwThreshold);
    else if (currentPower <= m_pLatchThreshold)
        unlocked = (targetPower <= m_pLatchThreshold);

    cout << "curr: " << currentPower << " target: " << targetPower << " " << boolalpha << unlocked << endl;
    if (m_checkBoxStartStopLed->isChecked() && unlocked)
    {
        cout << "setLedPower " << targetPower << endl;
        m_hwController->setLedPower(targetPower);
    }
}
void ControlGroupBox::enableStartCalibrationButton()
{
    m_startCalibrationButton->setEnabled(true);
}
void ControlGroupBox::updateCountdown()
{
    if (m_countdownTimer > 1)
    {
        m_countdownTimer--;
        int min = m_countdownTimer / 60;
        int sec = m_countdownTimer % 60;
        QString s = QStringLiteral("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
        m_countdownLabel.setText(s);
    } else
        m_clock->stop();
}
