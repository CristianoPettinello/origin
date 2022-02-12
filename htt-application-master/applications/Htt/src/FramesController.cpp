#include "FramesController.h"
#include <QDebug>

using namespace Nidek::Solution::HTT::UserApplication;

FramesController::FramesController(QObject* /*parent = nullptr*/)
    : m_processingLocked(false),
      m_processingEnabled(false),
      m_queueMaxSize(1)
{
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));

    // Set the interval.
    int msec = 20;
    m_timer.setInterval(msec);
}

FramesController::~FramesController()
{
    m_timer.stop();
}

void FramesController::start()
{
    // Start the timer.
    m_timer.start();
    // qDebug() << "FramesController::start()";
}

void FramesController::stop()
{
    // Stop the timer.
    m_timer.stop();

    m_mutex.lock();
    m_queue.clear();
    m_mutex.unlock();

    m_processingLocked = false;

    // qDebug() << "FramesController::stop()";
}

void FramesController::setMaxQueueSize(int maxSize)
{
    m_queueMaxSize = maxSize;
}

void FramesController::frameAvailable(QSharedPointer<HwData> data)
{
    m_mutex.lock();
    if (m_queue.size() < m_queueMaxSize)
    {
        m_queue.enqueue(data);
        // qDebug() << "Queue size: " << m_queue.size();
    }
    // else
    //     qDebug() << "Drop frame";
    m_mutex.unlock();
}

void FramesController::timerTimeout()
{
    m_mutex.lock();
    if (!m_queue.isEmpty() && !m_processingLocked)
    {
        QSharedPointer<HwData> data = m_queue.dequeue();
        if (m_processingEnabled)
        {
            m_processingLocked = true;
            emit doStartLensFitting(data->image, data->deltaN, data->H, data->modelType);
        } else
        {
            emit doSaveFrame(data);
        }
    }
    // else
    // {
    //     qDebug() << "empty or busy";
    // }
    m_mutex.unlock();
}

void FramesController::processingFinished()
{
    m_mutex.lock();
    m_processingLocked = false;
    m_mutex.unlock();
}

void FramesController::enableRealTimeProcessing(bool enabled)
{
    m_mutex.lock();
    m_processingEnabled = enabled;
    m_mutex.unlock();
}