#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QMainWindow>
#include <QString>
#include <QThread>
#include <memory>

#include "CalibrationWizard.h"
#include "ControlGroupBox.h"
#include "FramesController.h"
#include "GrabbingGroupBox.h"
#include "HttProcessor.h"
#include "HwController.h"
#include "ImageViewerController.h"
#include "LiveAcquisition.h"

using namespace Nidek::Libraries::HTT;

namespace Ui {
class MainWindow;
}

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QThread m_httProcessorThread;
    QThread m_hwControllerThread;
    QThread m_framesControllerThread;

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

signals:
    void doGetReferenceSquare(double width, double height);
    void doGetReferenceSquareForSlit(double width, double height);

private slots:
    void updateStatusBar(QString str);
    void calibrationDone(bool calibrated);
    void enableFramesController(bool enabled);

    // void frameAvailable(QSharedPointer<HwData> data);
    void modalityTabWidgetCurrentChanged(int index);
    void mgmtWizardSlots(bool doConnect);
    void mgmtLiveSlots(bool doConnect);

private:
    Ui::MainWindow* ui;
    ImageViewController* m_viewerController;
    HttProcessor* m_httProcessor;
    CalibrationWizard* m_wizard;
    ControlGroupBox* m_controlsGroupBox;
    LiveAcquisiton* m_live;
    GrabbingGroupBox* m_grabbingGroupBox;
    shared_ptr<FramesController> m_framesController;
    shared_ptr<HwController> m_hwController;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __MAIN_WINDOW_H__
