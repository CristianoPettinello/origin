#ifndef __HTT_DRIVER_BRIDGE_H__
#define __HTT_DRIVER_BRIDGE_H__

#include <QObject>
#include <QThread>
#include <QTimer>

#include "HttDriver.h"
#include "HwController.h"
#include "HwData.h"
#include "Log.h"
#include "Settings.h"
#include "Timer.h"

using namespace Nidek::Libraries::UsbDriver;
using namespace Nidek::Libraries::HTT;

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class FramesWorker : public QObject
{
    Q_OBJECT

public:
    // Constructor
    explicit FramesWorker(shared_ptr<HttDriver> httDriver);
    // Destructor
    ~FramesWorker();
    void setHorizontalFlip(bool hFlipping);

public slots:
    void start();
    void stop();
    void getFrame();

signals:
    void frameAvailable(QSharedPointer<HwData> data);

public:
    bool m_abort;

private:
    shared_ptr<Log> m_log;
    shared_ptr<HttDriver> m_driver;
    // bool m_abort;
    Timer m_timer;
    float m_interval;
    bool m_timeout;
    int m_counter;
    int m_failures;
    bool m_horizontalFlip;
};

class HttDriverBridge : public HwController
{
    Q_OBJECT

public:
    explicit HttDriverBridge(QObject* parent = nullptr);
    ~HttDriverBridge();

    // Return info about the driver.
    virtual QStringList driverInfo();

    // Get the current frame rate.
    virtual double cameraFrameRate();

    // Set the frame rate.
    virtual bool setCameraFrameRate(double frameRate);

    // Start the camera with a given grabbing mode.
    virtual bool startCamera(GrabbingMode mode);

    // Stop the camera acquistion
    virtual bool stopCamera();

    // Get the current camera gain value
    virtual int cameraGain();

    // Set the camera gain
    virtual bool setCameraGain(int gain);

    // Get the current camera exposure time
    virtual int exposureTime();

    // Get the current camera exposure time
    virtual bool setExposureTime(int time);

    // Get the current led power.
    virtual double ledPower();

    // Set the led power.
    virtual bool setLedPower(double percent);

private slots:
    void timerTimeout();
    void timerWaiting();

signals:
    void startFramesWorker();
    void stopFramesWorker();

private:
    shared_ptr<HttDriver> m_driver;
    shared_ptr<Settings> m_settings;
    shared_ptr<FramesWorker> m_worker;
    shared_ptr<Log> m_log;
    QTimer m_timeoutTimer;
    QTimer m_waitingTimer;
    QThread m_framesWorkerThread;
    // Thresholds used to differentiate LED usage modes
    int m_pCwThreshold;
    int m_pLatchThreshold;
    int m_pNullThreshold;
    int m_tLatch;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __HTT_DRIVER_BRIDGE_H__
