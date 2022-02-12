#include "Drivers/DummyCamera.h"
#include <QDebug>
#include <QFileInfo>
#include <QImage>
#include <math.h>

using namespace Nidek::Solution::HTT::UserApplication;

DummyCamera::DummyCamera(QObject* parent /* = nullptr */)
    : CameraDriver(parent),
      m_frameRate(0),
      m_maxFrameRate(30),
      m_frameCounter(0)
{
    // Load images in m_data
    insert("Grid", ":/images/grid", 0.0, 0.0, ModelType::Undefined);
    insert("M801", ":/images/frame801", 0, 14.7, ModelType::MiniBevelModelType);

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
}

DummyCamera::~DummyCamera()
{
}



QSharedPointer<QImage> DummyCamera::loadImage(QString fileName)
{
    if (!QFileInfo::exists(fileName))
        return nullptr;

    // Read the image
    QImage image(fileName);
    image = image.convertToFormat(QImage::Format_RGB888);

    return QSharedPointer<QImage>(new QImage(image));
}

void DummyCamera::timerTimeout()
{
    // Slot called when the timer has a timeout

    // Increase the frame counter
    ++m_frameCounter;

    qDebug() << "DummyCamera frame #" << m_frameCounter << "emitted";

    if (m_data.contains(m_frameToEmit) && m_data[m_frameToEmit] != nullptr)
    {

        QSharedPointer<HwData> data(new HwData());
        data->image = m_data[m_frameToEmit];
        data->frameCounter = m_frameCounter;
        data->exposureTime = 10.0; // No meaning in this number
        data->ledPower = 20.0;     // No meaning in this number
        data->frameRate = m_frameRate;
        data->deltaN = m_params[m_frameToEmit]->deltaN;
        data->H = m_params[m_frameToEmit]->H;
        data->modelType = m_params[m_frameToEmit]->modelType;
        emit frameAvailable(data);
    } else
        qDebug() << "Dummy Camera: frame not available";
}

double DummyCamera::frameRate()
{
    return m_frameRate;
}

bool DummyCamera::setFrameRate(double frameRate)
{
    if (frameRate <= 0.0 || frameRate > m_maxFrameRate)
        return false;

    // Set the frame rate.
    m_frameRate = frameRate;

    // Set the new interval.
    int msec = (int)floor(1000.0 / m_frameRate + 0.5);
    m_timer.setInterval(msec);

    return true;
}

bool DummyCamera::start(GrabbingMode mode)
{
    switch (mode)
    {
    case ContinuousAcquisition:
        // Start the timer.
        m_timer.start();
        break;
    case SingleAcquisition:
        QTimer::singleShot(50, this, SLOT(timerTimeout()));
        break;
    default:
        return false;
    }

    return true;
}

bool DummyCamera::stop()
{
    m_timer.stop();
    return true;
}

QStringList DummyCamera::driverInfo()
{
    QStringList info;
    info << "Dummy Camera v1.0";
    return info;
}

void DummyCamera::setImageMode(QString mode)
{
    qDebug() << "Dummy camera Mode:" << mode;
    m_frameToEmit = mode;
}

void DummyCamera::insert(QString name, QString filename, double deltaN, double H, ModelType modelType)
{
    m_data.insert(name, loadImage(filename));
    m_params.insert(name, QSharedPointer<DataParams>(new DataParams(deltaN, H, modelType)));
}

void DummyCamera::remove(QString name)
{
    if (m_data.contains(name))
        m_data.remove(name);
    if (m_params.contains(name))
        m_params.remove(name);
}

void DummyCamera::getParams(QString name, double& H, double& deltaN, ModelType& modelType)
{
    H = m_params[name]->H;
    deltaN = m_params[name]->deltaN;
    modelType = m_params[name]->modelType;
}

void DummyCamera::setParams(QString name, const double H, const double deltaN)
{
    if (m_params[name])
    {
        m_params[name]->H = H;
        m_params[name]->deltaN = deltaN;
    }
}
