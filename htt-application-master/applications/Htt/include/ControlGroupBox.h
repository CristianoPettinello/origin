#ifndef __CONTROL_GROUP_BOX_H__
#define __CONTROL_GROUP_BOX_H__

#include "HwController.h"
#include "ModalityWork.h"
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class ControlGroupBox : public QGroupBox
{
    Q_OBJECT
public:
    explicit ControlGroupBox(shared_ptr<HwController> hwController, QWidget* parent = Q_NULLPTR);
    ~ControlGroupBox();
    float getDeltaN();
    float getH();

private:
    void reset();

signals:
    void doActiveFrameController(bool enabled);
    void doTakeScreenshot();
    void doEnableProgressNextButton(bool);
    void doStartGridCalibration();
    void doStartAccurateCalibration(const double& deltaN, const double& H);
    void doManualGrab(int gain, int exposureTime, int ledPower);
    void doManualLensFitting(int index);
    void doChangeProcessingRealTimeState(bool enabled);
    void doDetectProfile();
    void doResetImageViewerController();
    void doSetViewerModality(Modality mod);

private slots:
    void setModality(Modality m);
    void cameraStarted();
    void cameraStopped();
    void cameraGainChanged();
    void exposureTimeChanged();
    void ledPowerChanged(double power);
    void setWaitingInterval(int value);
    void ledTimeoutTimerExpired();
    void ledWaitingTimerExpired();

    void checkBoxLiveCameraClicked(bool checked);
    void bevelComboBoxCurrentIndexChanged(int index);
    void checkBoxProcessingRealTimeChanged(int state);
    void gainSpinBloxValueChanged();
    void exposureTimeSpinBoxValueChanged();
    void pushButtonClickedManualGrab(bool);
    void pushButtonClickedManualLensFitting(bool);
    void pushButtonClickedProfileDetection(bool);
    // void pushButtonClickedAcquireImage(bool);
    void pushButtonClickedStartCalibration(bool);
    void checkBoxStartStopLedClicked(bool checked);
    void ledPowerDoubleSpinBloxValueChanged();
    void enableStartCalibrationButton();
    void updateCountdown();

private:
    shared_ptr<HwController> m_hwController;
    QJsonObject m_hwBoard;
    Modality m_modality;

    QGridLayout* m_grid;
    QGridLayout* m_cameraGrid;
    QGridLayout* m_ledGrid;
    QGridLayout* m_processingGrid;

    // Widgets
    QGroupBox* m_cameraControlGroupBox;
    QGroupBox* m_ledControlGroupBox;
    QGroupBox* m_processingGroupBox;

    // CAMERA
    QCheckBox* m_checkBoxLiveCamera;
    QComboBox* m_bevelsComboBox;
    QCheckBox* m_checkBoxProcessingRealTime;
    QPushButton* m_pushButtonManualGrab;
    QPushButton* m_pushButtonManualLensFitting;
    QPushButton* m_pushButtonProfileDetection;
    // QPushButton* m_pushButtonAcquireImage;
    QSpinBox* m_gainSpinBox;
    QSpinBox* m_targetGainSpinBox;
    QSpinBox* m_exposureTimeSpinBox;
    QSpinBox* m_targetExposureTimeSpinBox;
    bool m_cameraIsRunning;
    // bool m_imageAcquisitionRequest;

    // LED
    QCheckBox* m_checkBoxStartStopLed;
    QTimer* m_clock;
    int m_countdownTimer;
    QLabel m_countdownLabel;
    QDoubleSpinBox* m_targetLedPowerDoubleSpinBox;
    QDoubleSpinBox* m_currentLedPowerDoubleSpinBox;
    QLabel* m_ledOperationMode;
    QString m_initLedMode;
    int m_pCwThreshold;
    int m_pLatchThreshold;
    int m_pNullThreshold;

    // PROCESSING
    QPushButton* m_startCalibrationButton;
    QLabel* m_deltaNLabel;
    QLabel* m_hLabel;
    QDoubleSpinBox* m_deltaNSpinBox;
    QDoubleSpinBox* m_hSpinBox;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __CONTROL_GROUP_BOX_H__