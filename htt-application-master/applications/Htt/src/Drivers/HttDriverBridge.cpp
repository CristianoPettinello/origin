#include "Drivers/HttDriverBridge.h"
#include "HttUtils.h"

#include <QCoreApplication>
#include <QDebug>

using namespace Nidek::Solution::HTT::UserApplication;

#define TIMEOUT_INTERVAL 60000 // 5000
#define WAITING_INTERVAL 60000 // 10000

static const string tag = "[HDBR] ";

static string threadId()
{
    return "[" + to_string(GetCurrentThreadId()) + "] ";
}

FramesWorker::FramesWorker(shared_ptr<HttDriver> httDriver)
    : m_log(Log::getInstance()),
      m_driver(httDriver),
      m_abort(true),
      m_timeout(false),
      m_horizontalFlip(false)
{
}

FramesWorker::~FramesWorker()
{
}

void FramesWorker::setHorizontalFlip(bool hFlipping)
{
    m_horizontalFlip = hFlipping;
}

void FramesWorker::start()
{
    m_abort = false;
    m_counter = 0;
    m_failures = 0;
    m_log->debug(tag + threadId() + "FramesWorker::start() - m_abort = false");

    // m_log->debug(tag + threadId() + "priority: " + to_string(QThread::currentThread()->priority()));
    QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
    // m_log->debug(tag + threadId() + "priority: " + to_string(QThread::currentThread()->priority()));

    QTimer::singleShot(30, this, SLOT(getFrame()));
}

void FramesWorker::stop()
{
    m_abort = true;
    m_log->debug(tag + threadId() + "FramesWorker::stop() - m_abort = true");
}

void FramesWorker::getFrame()
{
    HttDriver::HttReturnCode r;
    // int counter = 0;
    // while (!m_abort)
    {
        // qDebug() << "waiting for frame..." << m_abort;

        shared_ptr<Image<uint8_t>> image(new Image<uint8_t>(560, 560, 1));
        // Enable horizontal flipping of the image
        r = m_driver->getFrame(image, m_horizontalFlip);
        m_counter++;
        if (r == HttDriver::HttReturnCode::HTT_USB_PASS)
        {
            if (m_timeout)
            {
                m_interval = m_timer.stop();
                // m_log->debug(tag + threadId() + "STOP timer");
                m_timeout = false;
                m_log->debug(tag + threadId() + "frame success " + to_string(m_interval));
            }

            QSharedPointer<HwData> frame = QSharedPointer<HwData>(new HwData());

            frame->image = HttUtils::convertImageToQImage(image);
            emit frameAvailable(frame);
        } else
        {
            m_failures++;
            m_log->debug(tag + threadId() + "frame fails " + to_string(m_failures) + "/" + to_string(m_counter) +
                         "m_abort: " + to_string(m_abort) + " timeout: " + to_string(m_timeout));

            // Software restart
            if (!m_abort)
            {
                if (!m_timeout)
                {
                    m_timer.start();
                    // m_log->debug(tag + threadId() + "START timer");
                    m_timeout = true;
                }

                m_log->debug(tag + threadId() + "FramesWorker::getFrame() - RESTART");
                m_driver->streamStart(true);
                // continue;
            }
        }
    }

    if (!m_abort)
        QTimer::singleShot(1, this, SLOT(getFrame()));
}

HttDriverBridge::HttDriverBridge(QObject* parent) /*: HwController(parent)*/ : m_log(Log::getInstance())
{
    m_settings = Settings::getInstance();
    m_settings->loadHwBoard("config/hwBoard.json");

    Json::Value hw = m_settings->getHwBoard();
    Json::Value settings = m_settings->getSettings();

    m_pCwThreshold = hw["led"]["Pcw"].asInt();
    m_pLatchThreshold = hw["led"]["Platch"].asInt();
    m_pNullThreshold = hw["led"]["Pnull"].asInt();
    m_tLatch = hw["led"]["Tlatch"].asInt() * 1000; // ms

    int vid = stoul(hw["firmware"]["vid"].asString(), nullptr, 16);
    int pid = stoul(hw["firmware"]["pid"].asString(), nullptr, 16);
    string filename = hw["firmware"]["filename"].asString();
    int pidBoard = stoul(hw["board"]["pid"].asString(), nullptr, 16);
    int endpoint = stoul(hw["board"]["streaming_endpoint"].asString(), nullptr, 16);

    m_driver = shared_ptr<HttDriver>(new HttDriver(vid, pid, pidBoard, endpoint, filename));

    m_timeoutTimer.setSingleShot(true);
    m_waitingTimer.setSingleShot(true);

    m_worker = shared_ptr<FramesWorker>(new FramesWorker(m_driver));
    m_worker->moveToThread(&m_framesWorkerThread); // FIXME: check ownership

    // Management of the image flipping
    bool horizontalFlip = settings["image"]["horizontalFlip"].asBool();
    m_worker->setHorizontalFlip(horizontalFlip);

    connect(&m_timeoutTimer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    connect(&m_waitingTimer, SIGNAL(timeout()), this, SLOT(timerWaiting()));
    connect(&m_framesWorkerThread, &QThread::finished, m_worker.get(), &QObject::deleteLater);
    connect(this, &HttDriverBridge::startFramesWorker, m_worker.get(), &FramesWorker::start);
    connect(this, &HttDriverBridge::stopFramesWorker, m_worker.get(), &FramesWorker::stop);

    connect(m_worker.get(), SIGNAL(frameAvailable(QSharedPointer<HwData>)), this,
            SIGNAL(frameAvailable(QSharedPointer<HwData>)));
}

HttDriverBridge::~HttDriverBridge()
{
    if (m_timeoutTimer.isActive())
        m_timeoutTimer.stop();
    if (m_waitingTimer.isActive())
        m_waitingTimer.stop();

    stopCamera();
}

// Return info about the camera
QStringList HttDriverBridge::driverInfo()
{
    QStringList info;
    info << "HttDriverBridge info";
    return info;
}

// Get the current frame rate.
double HttDriverBridge::cameraFrameRate()
{
    return 0.0;
}

// Set the frame rate.
bool HttDriverBridge::setCameraFrameRate(double frameRate)
{
    // FIXME
    return true;
}

// Start the camera with a given grabbing mode.
bool HttDriverBridge::startCamera(GrabbingMode mode)
{
    m_framesWorkerThread.start();

    HttDriver::HttReturnCode r = m_driver->streamStart();
    bool isStarted = r == HttDriver::HttReturnCode::HTT_USB_PASS;

    if (isStarted)
    {
        // m_log->debug(tag + threadId() + "emit startFramesWorker()");
        emit startFramesWorker();
        // m_log->debug(tag + threadId() + "emit cameraStarted()");
        emit cameraStarted();
    }

    return isStarted;
}

// Stop the camera acquisition
bool HttDriverBridge::stopCamera()
{
    m_log->debug(tag + "HttDriverBridge::stopCamera()");
    HttDriver::HttReturnCode r = m_driver->streamStop();
    bool isStopped = r == HttDriver::HttReturnCode::HTT_USB_PASS;
    if (isStopped)
    {
        // m_framesWorkerThread.quit();
        // m_framesWorkerThread.wait(); // FIXME: with this line the program crashes

        // Force to quit m_framesWorkerThread
        // m_log->debug(tag + threadId() + "m_abort = true");
        // m_worker->m_abort = true;
        emit stopFramesWorker();

        // SI FERMA MA POI NON RIPARTE
        // m_framesWorkerThread.terminate();

        emit cameraStopped();
    }
    return isStopped;
}

// Get the current camera gain value
int HttDriverBridge::cameraGain()
{
    int gain;
    HttDriver::HttReturnCode r = m_driver->gainGet(gain);
    if (r == HttDriver::HttReturnCode::HTT_USB_PASS)
        return gain;
    else
        return 0;
}

// Set the camera gain
bool HttDriverBridge::setCameraGain(int gain)
{
    HttDriver::HttReturnCode r = m_driver->gainSet(gain);
    bool ret = (r == HttDriver::HttReturnCode::HTT_USB_PASS);
    if (ret)
        emit cameraGainChanged();
    return ret;
}

// Get the current camera exposure time
int HttDriverBridge::exposureTime()
{
    int time;
    HttDriver::HttReturnCode r = m_driver->exposuretTimeGet(time);
    if (r == HttDriver::HttReturnCode::HTT_USB_PASS)
        return time;
    else
        return 0;
}

// Get the current camera exposure time
bool HttDriverBridge::setExposureTime(int time)
{
    HttDriver::HttReturnCode r = m_driver->exposuretTimeSet(time);
    bool ret = (r == HttDriver::HttReturnCode::HTT_USB_PASS);
    if (ret)
        emit exposureTimeChanged();
    return ret;
}

// Get the current Led power
double HttDriverBridge::ledPower()
{
    int power;
    HttDriver::HttReturnCode r = m_driver->ledGet(power);
    if (r == HttDriver::HttReturnCode::HTT_USB_PASS)
        return ((double)power / 10.0);
    else
        return 0;
}

// Set the Led power
bool HttDriverBridge::setLedPower(double power)
{
#if 0
    // for a power <=5%, the LED can be kept continuously ON
    // for a power from 5% to 20% (included), the LED can be kept ON only for 60s. Then, after switch off, swith-on
    // will be disabled for 60s, to let the LED cool down. for a power from 20% to 50% (included), the LED can be
    // used only in triggered mode power >50% is forbidden

    if (power > 50)
        return false;

    int p = (int)(power * 10.0);
    HttDriver::HttReturnCode r = m_driver->ledSet(p);
    bool isSetted = r == HttDriver::HttReturnCode::HTT_USB_PASS;
    if (isSetted)
    {
        emit ledPowerChanged(power);

        // FIXME
        // if (power <= 5)
        // {
        // }

        if (!m_timeoutTimer.isActive())
        {
            m_log->debug(tag + "Set Timeout Timer");
            m_timeoutTimer.start(TIMEOUT_INTERVAL);
            m_waitingTimer.start(TIMEOUT_INTERVAL + WAITING_INTERVAL);
        } else
        {
            if (power == 0)
            {
                m_timeoutTimer.stop();
                m_waitingTimer.stop();
                m_waitingTimer.start(WAITING_INTERVAL);
            }
        }
    }
    return isSetted;
#else
    // int p = (int)(power * 10.0);
    // HttDriver::HttReturnCode r = m_driver->ledSet(p);
    // bool isSetted = r == HttDriver::HttReturnCode::HTT_USB_PASS;
    // if (isSetted)
    //     emit ledPowerChanged(power);
    // return isSetted;

    /**
     * for a percent power P <= Pcw, the LED can be kept continuously ON (cw mode).
     * for a percent power Pcw < P <= Platch, the LED is used in latch mode.
     * for a percent power Platch < P <= Pnull, the LED is used in triggered mode.
     * for a percent power P > Pnull, the LED cannot be switched on.
     */

    if (power > m_pNullThreshold)
        return false;

    // FIXME - remove this control when the "triggered" mode will be implemented
    if (power > m_pLatchThreshold)
        return false;

    int p = (int)(power * 10.0);
    HttDriver::HttReturnCode r = m_driver->ledSet(p);
    bool isSetted = r == HttDriver::HttReturnCode::HTT_USB_PASS;
    if (isSetted)
    {
        emit ledPowerChanged(power);

        if (power == 0)
        {
            if (m_timeoutTimer.isActive())
            {
                int tCool = m_tLatch - m_timeoutTimer.remainingTime();
                m_timeoutTimer.stop();
                m_waitingTimer.stop();
                m_log->debug(tag + "Set Tcool timer " + to_string(tCool) + " ms");
                emit setWaitingInterval(tCool);
                m_waitingTimer.start(tCool);
            }
        } else if (power <= m_pCwThreshold)
        {
            // nothing to do
            ;
        } else if (power <= m_pLatchThreshold)
        {
            if (!m_timeoutTimer.isActive())
            {
                m_log->debug(tag + "Set Tlatch timer " + to_string(m_tLatch) + " ms");
                m_timeoutTimer.start(m_tLatch);
                m_waitingTimer.start(2 * m_tLatch);
            }

        } else // power <= m_pNullThreshold
        {
            // TODO: not yet implemented, missing information!
            ;
        }
    }
    return isSetted;

#endif
}

void HttDriverBridge::timerTimeout()
{
    m_log->debug(tag + "Timeout timer expired");
#if 0
    // FIXME: QObject::startTimer: Timers cannot be started from another thread
    m_log->debug(tag + "Set Tcool timer " + to_string(m_tLatch) + " ms");
    m_waitingTimer.start(m_tLatch);
#endif

    HttDriver::HttReturnCode r = m_driver->ledSet(0);
    if (r == HttDriver::HttReturnCode::HTT_USB_PASS)
        emit ledPowerChanged(0);

    emit ledTimeoutTimerExpired();
    emit setWaitingInterval(m_tLatch);
}

void HttDriverBridge::timerWaiting()
{
    m_log->debug(tag + "Waiting Timer expired");
    emit ledWaitingTimerExpired();
}
