#ifndef __FRAMES_CONTROLLER_H__
#define __FRAMES_CONTROLLER_H__

#include "Htt.h"
#include "HwData.h"
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QSharedPointer>
#include <QTimer>

using namespace std;
using namespace Nidek::Libraries::HTT;

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

struct Frame
{
    QSharedPointer<QImage> image;
    double deltaN;
    double H;
    ModelType modelType;
};

class FramesController : public QObject
{
    Q_OBJECT

public:
    explicit FramesController(QObject* parent = nullptr);
    ~FramesController();
    void start();
    void stop();
    void setMaxQueueSize(int maxSize);

public slots:
    void frameAvailable(QSharedPointer<HwData> data);

private slots:
    void timerTimeout();
    void processingFinished();
    void enableRealTimeProcessing(bool enabled);

signals:
    void doSaveFrame(QSharedPointer<HwData> data);
    void doStartLensFitting(QSharedPointer<QImage> image, double deltaN, double H, ModelType modelType);

private:
    bool m_processingLocked;
    bool m_processingEnabled;
    int m_queueMaxSize;
    QMutex m_mutex;
    QTimer m_timer;
    QQueue<QSharedPointer<HwData>> m_queue;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __FRAMES_CONTROLLER_H__