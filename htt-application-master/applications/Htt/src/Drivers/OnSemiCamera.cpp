#include "Drivers/OnSemiCamera.h"
#include <QDebug>
#include <QFileInfo>
#include <QImage>
#include <QList>
#include <math.h>

using namespace Nidek::Solution::HTT::UserApplication;

OnSemiCamera::OnSemiCamera(QObject* parent /* = nullptr */)
    : CameraDriver(parent),
      m_frameRate(0),
      m_maxFrameRate(30),
      m_frameCounter(0)
{
    // // Load images in m_data
    // insert("Grid", ":/images/grid", 0.0, 0.0, ModelType::Undefined);
    // insert("M801", ":/images/frame801", 0, 14.7, ModelType::MiniBevelModelType);

    UsbDeviceParameters parameters;
    m_device = new UsbDevice(parameters);

    string str;

#if 0
    // Camera channel
    const unsigned short GBVF_CAMERA_INPUT_PACKET_SIZE = 512U;
    const unsigned short GBVF_CAMERA_INPUT_PACKET_MAXIMUM_SIZE = 512U;
    UsbEndPointDescriptor GBVF_CAMERA_INPUT_ENDPOINT = {true, UsbMode::UsbModeBulk,
                                                        0x83, GBVF_CAMERA_INPUT_PACKET_MAXIMUM_SIZE,
                                                        1u,   UsbEndpointDirection::UsbEndpointDirectionInput};

    const unsigned short GBVF_CAMERA_OUTPUT_PACKET_SIZE = 512U;
    const unsigned short GBVF_CAMERA_OUTPUT_PACKET_MAXIMUM_SIZE = 512U;
    UsbEndPointDescriptor GBVF_CAMERA_OUTPUT_ENDPOINT = {false, UsbMode::UsbModeBulk,
                                                         0x00,  GBVF_CAMERA_OUTPUT_PACKET_MAXIMUM_SIZE,
                                                         1u,    UsbEndpointDirection::UsbEndpointDirectionOutput};

    static const UsbChannel GBVF_CAMERA_CHANNEL(GBVF_CAMERA_INPUT_ENDPOINT, GBVF_CAMERA_INPUT_PACKET_SIZE,
                                                GBVF_CAMERA_OUTPUT_ENDPOINT, GBVF_CAMERA_OUTPUT_PACKET_SIZE);

    channelList.push_back(GBVF_CAMERA_CHANNEL);
#endif

#if 0 // FIXME: test with single input channel
    // Camera channel
    const unsigned short ONSEMI_CAMERA_INPUT_PACKET_SIZE = 1024U;
    const unsigned short ONSEMI_CAMERA_INPUT_PACKET_MAXIMUM_SIZE = 1024U;
    UsbEndPointDescriptor ONSEMI_CAMERA_INPUT_ENDPOINT = {true, UsbMode::UsbModeBulk,
                                                          0x82, ONSEMI_CAMERA_INPUT_PACKET_MAXIMUM_SIZE,
                                                          0u,   UsbEndpointDirection::UsbEndpointDirectionInput};

    static const UsbChannel ONSEMI_CAMERA_CHANNEL(ONSEMI_CAMERA_INPUT_ENDPOINT, ONSEMI_CAMERA_INPUT_PACKET_SIZE);
#endif

    const unsigned short ONSEMI_CAMERA_INPUT_PACKET_SIZE = 64U;
    const unsigned short ONSEMI_CAMERA_INPUT_PACKET_MAXIMUM_SIZE = 1024U;
    UsbEndPointDescriptor ONSEMI_CAMERA_INPUT_ENDPOINT = {true, UsbMode::UsbModeBulk,
                                                          0x82, ONSEMI_CAMERA_INPUT_PACKET_MAXIMUM_SIZE,
                                                          0u,   UsbEndpointDirection::UsbEndpointDirectionInput};

    const unsigned short ONSEMI_CAMERA_OUTPUT_PACKET_SIZE = 1024U;
    const unsigned short ONSEMI_CAMERA_OUTPUT_PACKET_MAXIMUM_SIZE = 1024U;
    UsbEndPointDescriptor ONSEMI_CAMERA_OUTPUT_ENDPOINT = {false, UsbMode::UsbModeBulk,
                                                           0x00,  ONSEMI_CAMERA_OUTPUT_PACKET_MAXIMUM_SIZE,
                                                           0u,    UsbEndpointDirection::UsbEndpointDirectionOutput};

    static const UsbChannel ONSEMI_CAMERA_CHANNEL(ONSEMI_CAMERA_INPUT_ENDPOINT, ONSEMI_CAMERA_INPUT_PACKET_SIZE,
                                                  ONSEMI_CAMERA_OUTPUT_ENDPOINT, ONSEMI_CAMERA_OUTPUT_PACKET_SIZE);

    channelList.push_back(ONSEMI_CAMERA_CHANNEL);
    bool isOpen = m_device->openDevice(0x20FB, 0x100E, channelList, str);

    qDebug() << "[OnSemi] str " << QString::fromStdString(str) << " open: " << QVariant(isOpen).toString();
    qDebug() << "[OnSemi] channelList size " << channelList.size();

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
}

OnSemiCamera::~OnSemiCamera()
{
    m_device->closeDevice();
}

QSharedPointer<QImage> OnSemiCamera::loadImage(QString fileName)
{
    if (!QFileInfo::exists(fileName))
        return nullptr;

    // Read the image
    QImage image(fileName);
    image = image.convertToFormat(QImage::Format_RGB888);

    return QSharedPointer<QImage>(new QImage(image));
}

void OnSemiCamera::timerTimeout()
{
    std::shared_ptr<UsbChannel> ch = make_shared<UsbChannel>(channelList.front());
    // const std::list<UsbChannel>& ch = channelList.front();
    const int size = 64; // 940800;
    unsigned char* data;
    data = (unsigned char*)malloc(size);
    memset(data, 0, size);
    // unsigned char data[size];

    unsigned int dataSize = 0;
    UsbReturnCode ret = m_device->readData(ch, data, size, 2000, dataSize);
    qDebug() << "[OnSemi] datasize: " << dataSize << " ret: " << QString::number(ret);

    free(data);

    return;
    // Slot called when the timer has a timeout

    // Increase the frame counter
    ++m_frameCounter;

    qDebug() << "[OnSemi] frame #" << m_frameCounter << "emitted";

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

double OnSemiCamera::frameRate()
{
    return m_frameRate;
}

bool OnSemiCamera::setFrameRate(double frameRate)
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

bool OnSemiCamera::start(GrabbingMode mode)
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

bool OnSemiCamera::stop()
{
    m_timer.stop();
    return true;
}

QStringList OnSemiCamera::driverInfo()
{
    QStringList info;
    info << "Dummy Camera v1.0";
    return info;
}

void OnSemiCamera::setImageMode(QString mode)
{
    qDebug() << "Dummy camera Mode:" << mode;
    m_frameToEmit = mode;
}

// FIXME
void OnSemiCamera::insert(QString name, QString filename, double deltaN, double H, ModelType modelType)
{
    m_data.insert(name, loadImage(filename));
    m_params.insert(name, QSharedPointer<DataParams>(new DataParams(deltaN, H, modelType)));
}

void OnSemiCamera::remove(QString name)
{
    if (m_data.contains(name))
        m_data.remove(name);
    if (m_params.contains(name))
        m_params.remove(name);
}

void OnSemiCamera::getParams(QString name, double& H, double& deltaN, ModelType& modelType)
{
    H = m_params[name]->H;
    deltaN = m_params[name]->deltaN;
    modelType = m_params[name]->modelType;
}

void OnSemiCamera::setParams(QString name, const double H, const double deltaN)
{
    if (m_params[name])
    {
        m_params[name]->H = H;
        m_params[name]->deltaN = deltaN;
    }
}