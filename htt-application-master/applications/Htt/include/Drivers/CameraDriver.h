#ifndef __CAMERA_DRIVER_H__
#define __CAMERA_DRIVER_H__

#include "HwData.h"
#include <QObject>
#include <memory>

using namespace std;

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class CameraDriver : public QObject
{

    Q_OBJECT

public:
    enum GrabbingMode
    {
        SingleAcquisition,
        ContinuousAcquisition
    };

public:
    explicit CameraDriver(QObject* parent = nullptr);
    ~CameraDriver();

    // Get the current frame rate.
    virtual double frameRate() = 0;

    // Set the frame rate.
    virtual bool setFrameRate(double frameRate) = 0;

    // Start the camera with a given grabbing mode.
    virtual bool start(GrabbingMode mode) = 0;

    // Stop the camera acquistion
    virtual bool stop() = 0;

    // Return info about the camera
    virtual QStringList driverInfo() = 0;

    // Test function for setting the current image returned by the camera.
    // This is necessary for using the Dummy camera.
    virtual void setImageMode(QString mode) = 0;

    // FIXME: Custom function for insert new image usedfor simulate the live acquisition
    virtual void insert(QString name, QString filename, double deltaN, double H, ModelType modelType) = 0;
    virtual void remove(QString name) = 0;
    virtual void getParams(QString name, double& H, double& deltaN, ModelType& modelType) = 0;
    virtual void setParams(QString name, const double H, const double deltaN) = 0;

signals:

    void frameAvailable(QSharedPointer<HwData> data);
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __CAMERA_DRIVER_H__
