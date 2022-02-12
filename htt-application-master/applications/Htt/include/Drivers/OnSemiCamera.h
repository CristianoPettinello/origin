#ifndef __ON_SEMI_CAMERA_H__
#define __ON_SEMI_CAMERA_H__

#include <QMap>
#include <QTimer>
#include <memory>

#include "CameraDriver.h"
#include "HwData.h"
#include "Image.h"
#include "ModelType.h"
#include "UsbDevice.h"

using namespace std;
using namespace Nidek::Libraries::HTT;
using namespace Nidek::Libraries::UsbCommunication;

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class OnSemiCamera : public CameraDriver
{
    Q_OBJECT

private:
    enum ImageType
    {
        Grid,
        Bevel,
        CalibratedBevel
    };

    struct DataParams
    {
        DataParams(double deltaN, double H, ModelType modelType) : deltaN(deltaN), H(H), modelType(modelType){};
        double deltaN;
        double H;
        ModelType modelType;
    };

public:
    explicit OnSemiCamera(QObject* parent = nullptr);
    ~OnSemiCamera();

    // Get the current frame rate.
    virtual double frameRate();

    // Set the frame rate.
    virtual bool setFrameRate(double frameRate);

    // Start the camera with a given grabbing mode.
    virtual bool start(GrabbingMode mode);

    // Stop the camera acquistion
    virtual bool stop();

    // Return info about the driver
    virtual QStringList driverInfo();

    // Test function for setting the current image returned by the camera.
    // This is necessary for using the Dummy camera.
    virtual void setImageMode(QString mode);

    // FIXME: Custom function for insert new image usedfor simulate the live acquisition
    virtual void insert(QString name, QString filename, double deltaN, double H, ModelType modelType);
    virtual void remove(QString name);
    virtual void getParams(QString name, double& H, double& deltaN, ModelType& modelType);
    virtual void setParams(QString name, const double H, const double deltaN);

private:
    QSharedPointer<QImage> loadImage(QString fileName);

private slots:
    void timerTimeout();

private:
    double m_frameRate;
    double m_maxFrameRate;
    int m_frameCounter;
    UsbDevice* m_device;

    QString m_frameToEmit;

    QMap<QString, QSharedPointer<QImage>> m_data;
    QMap<QString, QSharedPointer<DataParams>> m_params;
    QTimer m_timer;
    std::list<UsbChannel> channelList;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __ON_SEMI_CAMERA_H__
