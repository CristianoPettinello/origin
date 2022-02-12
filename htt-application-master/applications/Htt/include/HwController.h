#ifndef __HW_CONTROLLER_H__
#define __HW_CONTROLLER_H__

#include "HwData.h"
#include <QObject>
#include <memory>

using namespace std;

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class HwController : public QObject
{

    Q_OBJECT

public:
    enum GrabbingMode
    {
        SingleAcquisition,
        ContinuousAcquisition
    };

public:
    // // Inversion of dependency
    // explicit HwController(QObject* parent = nullptr);
    // ~HwController();

    // Return info about the driver.
    virtual QStringList driverInfo() = 0;

    // Get the current frame rate.
    virtual double cameraFrameRate() = 0;

    // Set the frame rate.
    virtual bool setCameraFrameRate(double frameRate) = 0;

    // Start the camera with a given grabbing mode.
    virtual bool startCamera(GrabbingMode mode) = 0;

    // Stop the camera acquistion
    virtual bool stopCamera() = 0;

    // Get the current camera gain value
    virtual int cameraGain() = 0;

    // Set the camera gain
    virtual bool setCameraGain(int gain) = 0;

    // Get the current camera exposure time
    virtual int exposureTime() = 0;

    // Get the current camera exposure time
    virtual bool setExposureTime(int time) = 0;

    // Get the current led power.
    virtual double ledPower() = 0;

    // Set the led power.
    virtual bool setLedPower(double percent) = 0;

signals:
    void cameraStarted();
    void cameraStopped();
    void cameraGainChanged();
    void exposureTimeChanged();
    void frameAvailable(QSharedPointer<HwData> data);
    void ledPowerChanged(double value);
    void setWaitingInterval(int value);
    void ledTimeoutTimerExpired();
    void ledWaitingTimerExpired();
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __HW_CONTROLLER_H__
