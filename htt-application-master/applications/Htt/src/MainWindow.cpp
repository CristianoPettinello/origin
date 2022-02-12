#include "MainWindow.h"
#include "Globals.h"

// #include "HttDriver.h"
// using namespace Nidek::Libraries::UsbDriver;

using namespace Nidek::Solution::HTT::UserApplication;
using namespace std;

#include "ui_MainWindow.h"
#include <QDebug>
#include <QFile>
#include <QSpacerItem>

#include "Drivers/HttDriverBridge.h"

// Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr)

#define CONNECT(doConnect, ...)                                                                                        \
    if (doConnect)                                                                                                     \
        connect(__VA_ARGS__);                                                                                          \
    else                                                                                                               \
        disconnect(__VA_ARGS__);

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_httProcessor(new HttProcessor()),
      m_hwController(nullptr)
{
    m_httProcessor->moveToThread(&m_httProcessorThread);

    ui->setupUi(this);
    this->setWindowTitle(QString("HTT GUI v") + GUI_VERSION);

    m_hwController = shared_ptr<HwController>(new HttDriverBridge());
    m_hwController->moveToThread(&m_hwControllerThread);

    // Create the frames controller
    m_framesController = shared_ptr<FramesController>(new FramesController());
    m_framesController->moveToThread(&m_framesControllerThread);

    // camera->timerTimeout();

#if 0
    qRegisterMetaType<QSharedPointer<HwData>>("QSharedPointer<HwData>");
    connect(m_hwController.get(), SIGNAL(frameAvailable(QSharedPointer<HwData>)), this,
            SLOT(frameAvailable(QSharedPointer<HwData>)));
#endif
    m_hwControllerThread.start();
    //     m_hwControllerThread.setPriority(QThread::TimeCriticalPriority);

    // StatusBar
    ui->statusBar->showMessage("Ready");

    // HorizontalLayout used for manage the layers of the QGraphicsScene
    QHBoxLayout* viewerLayersLayout = new QHBoxLayout();
    ui->viewerVerticalLayout->addLayout(viewerLayersLayout);

    // Unused label under the imageViewer
    // FIXME:  can we remove it?
    ui->label->setVisible(false);

    // ImageViewerController
    m_viewerController = new ImageViewController(ui->imageViewer, ui->viewerVerticalLayout);

    // GroupBox with the hardware controls
    m_controlsGroupBox = new ControlGroupBox(m_hwController);
    ui->viewerVerticalLayout->addWidget(m_controlsGroupBox);

    // Grabbing groupBox
    m_grabbingGroupBox = new GrabbingGroupBox();

    // Calibration Wizard
    m_wizard = new CalibrationWizard(m_controlsGroupBox, m_grabbingGroupBox, m_hwController, ui->Calibration);

    // Live Acquisition
    m_live = new LiveAcquisiton(m_controlsGroupBox, m_hwController, ui->Live);

    ui->modalityTabWidget->setCurrentIndex(ui->modalityTabWidget->indexOf(ui->Calibration));

    QSpacerItem* spacer = new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);
    ui->viewerVerticalLayout->addSpacerItem(spacer);

    // Register new meta type
    qRegisterMetaType<QSharedPointer<QImage>>("QSharedPointer<QImage>");
    qRegisterMetaType<Htt::CalibrationGridResults>("Htt::CalibrationGridResults");
    qRegisterMetaType<Htt::AccurateCalibrationResults>("Htt::AccurateCalibrationResults");
    qRegisterMetaType<map<string, bool>>("map<string,bool>");
    qRegisterMetaType<vector<pair<double, double>>>("vector<pair<double,double>>");
    qRegisterMetaType<QSharedPointer<QPixmap>>("QSharedPointer<QPixmap>");
    qRegisterMetaType<QSharedPointer<HwData>>("QSharedPointer<HwData>");
    qRegisterMetaType<Htt::LensFittingResults>("Htt::LensFittingResults");
    qRegisterMetaType<Htt::ProfileDetectionResults>("Htt::ProfileDetectionResults");
    qRegisterMetaType<ModelType>("ModelType");
    qRegisterMetaType<vector<pair<int, int>>>("vector<pair<int,int> >");
    qRegisterMetaType<vector<float>>("vector<float>");

    // Connects
    mgmtWizardSlots(true);

    // HttProcessor
    connect(m_httProcessor, SIGNAL(measureOfDistanceReady(double)), m_viewerController,
            SLOT(measureOfDistanceReady(double)));
    connect(m_httProcessor, SIGNAL(minimumImagingFoVReady(vector<pair<int, int>>)), m_viewerController,
            SLOT(minimumImagingFoVReady(vector<pair<int, int>>)));
    connect(m_httProcessor, SIGNAL(referenceSquareReady(vector<pair<int, int>>)), m_viewerController,
            SLOT(referenceSquareReady(vector<pair<int, int>>)));
    connect(m_httProcessor, SIGNAL(referenceSquareForSlitReady(vector<pair<int, int>>)), m_viewerController,
            SLOT(referenceSquareForSlitReady(vector<pair<int, int>>)));

    // ImageViewerController
    connect(m_viewerController, SIGNAL(doMeasureOnObjectPlane(double, double, double, double)), m_httProcessor,
            SLOT(computeMeasureOfDistance(double, double, double, double)));
    connect(m_viewerController, SIGNAL(doGetMinimumImagingFoV(int, int, double, double)), m_httProcessor,
            SLOT(getMinimumImagingFoV(int, int, double, double)));
    connect(m_viewerController, SIGNAL(updateStatusBar(QString)), this, SLOT(updateStatusBar(QString)));

    // Threads
    connect(&m_httProcessorThread, &QThread::finished, m_httProcessor, &HttProcessor::deleteLater);
    connect(&m_framesControllerThread, &QThread::finished, m_framesController.get(), &FramesController::deleteLater);

    // Modality Tab Widget
    connect(ui->modalityTabWidget, SIGNAL(currentChanged(int)), this, SLOT(modalityTabWidgetCurrentChanged(int)));

    // ControlGroupBox
    connect(m_controlsGroupBox, SIGNAL(doActiveFrameController(bool)), this, SLOT(enableFramesController(bool)));
    connect(m_controlsGroupBox, SIGNAL(doTakeScreenshot()), m_viewerController, SLOT(takeScreenshot()));

    // HttDriverBridge
    connect(m_hwController.get(), SIGNAL(cameraStarted()), m_controlsGroupBox, SLOT(cameraStarted()));
    connect(m_hwController.get(), SIGNAL(cameraStopped()), m_controlsGroupBox, SLOT(cameraStopped()));
    connect(m_hwController.get(), SIGNAL(cameraGainChanged()), m_controlsGroupBox, SLOT(cameraGainChanged()));
    connect(m_hwController.get(), SIGNAL(exposureTimeChanged()), m_controlsGroupBox, SLOT(exposureTimeChanged()));
    connect(m_hwController.get(), SIGNAL(ledPowerChanged(double)), m_controlsGroupBox, SLOT(ledPowerChanged(double)));
    connect(m_hwController.get(), SIGNAL(setWaitingInterval(int)), m_controlsGroupBox, SLOT(setWaitingInterval(int)));
    connect(m_hwController.get(), SIGNAL(ledTimeoutTimerExpired()), m_controlsGroupBox, SLOT(ledTimeoutTimerExpired()));
    connect(m_hwController.get(), SIGNAL(ledWaitingTimerExpired()), m_controlsGroupBox, SLOT(ledWaitingTimerExpired()));

    // Mainwindows
    connect(this, SIGNAL(doGetReferenceSquare(double, double)), m_httProcessor,
            SLOT(getReferenceSquare(double, double)));
    connect(this, SIGNAL(doGetReferenceSquareForSlit(double, double)), m_httProcessor,
            SLOT(getReferenceSquareForSlit(double, double)));

    // Start HTT Processor thread
    m_httProcessorThread.start();

    // Start Frames Controller thread
    m_framesControllerThread.start();

#if 0
        // TEST
        CalibrationReportGenerator reportGenerator(this);

        //     QFile file("/home/degiu/testHttReport/status.json");
        QFile file("config/status.json");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QString jsonString = file.readAll();
            file.close();
            reportGenerator.createReport("config/httReport.pdf", jsonString);
        } else
            qDebug() << "File open error";
#endif

    // Generate the reference square used in calibration wizard
    emit doGetReferenceSquare(4.0, 4.0);
    emit doGetReferenceSquareForSlit(5.0, 5.0);
}

MainWindow::~MainWindow()
{
    m_hwControllerThread.quit();
    m_hwControllerThread.wait();

    m_httProcessorThread.quit();
    m_httProcessorThread.wait();

    m_framesControllerThread.quit();
    m_framesControllerThread.wait();

    delete m_httProcessor;
    delete m_viewerController;
    delete m_wizard;
    delete ui;
}

void MainWindow::updateStatusBar(QString str)
{
    ui->statusBar->showMessage(str);
}

void MainWindow::calibrationDone(bool /*calibrated*/)
{
    qDebug() << "calibrationDone";
    //     int liveIndex = ui->modalityTabWidget->indexOf(ui->Live);
    //     int calibIndex = ui->modalityTabWidget->indexOf(ui->Calibration);
    //     //     ui->modalityTabWidget->setTabEnabled(liveIndex, true);
    //     ui->modalityTabWidget->setCurrentIndex(liveIndex);
    //     ui->modalityTabWidget->setCurrentIndex(calibIndex);

    //     if (calibrated)
    //         m_wizard->start();
    //     else
    //         m_wizard->resume();

    m_wizard->start();
}

void MainWindow::enableFramesController(bool enabled)
{
    if (enabled)
        m_framesController->start();
    else
        m_framesController->stop();
}

void MainWindow::modalityTabWidgetCurrentChanged(int index)
{
    qDebug() << "modalityTabWidgetCurrentChanged";
    //     m_live->clean();
    if (index == ui->modalityTabWidget->indexOf(ui->Calibration))
    {
        qDebug() << "CALIB";
        m_live->stop();
        m_framesController->stop();
        mgmtLiveSlots(false);
        mgmtWizardSlots(true);
        m_wizard->start();
    } else
    {
        qDebug() << "LIVE";
        m_wizard->stop();
        mgmtWizardSlots(false);
        mgmtLiveSlots(true);
        m_framesController->start();
        m_live->start();
    }
}

void MainWindow::mgmtWizardSlots(bool doConnect)
{
    // HttProcessor
    CONNECT(doConnect, m_httProcessor, SIGNAL(calibrationGridResultsReady(Htt::CalibrationGridResults)), m_wizard,
            SLOT(calibrationGridResultsReady(Htt::CalibrationGridResults)));
    CONNECT(doConnect, m_httProcessor, SIGNAL(accurateCalibrationResultsReady(Htt::AccurateCalibrationResults)),
            m_wizard, SLOT(accurateCalibrationResultsReady(Htt::AccurateCalibrationResults)));
    CONNECT(doConnect, m_httProcessor,
            SIGNAL(calibrationGridValidationReady(QString, map<string, bool>, vector<float>)), m_wizard,
            SLOT(calibrationGridValidationReady(QString, map<string, bool>, vector<float>)));
    CONNECT(doConnect, m_httProcessor,
            SIGNAL(accurateCalibrationValidationReady(QString, map<string, bool>, Htt::LensFittingResults)), m_wizard,
            SLOT(accurateCalibrationValidationReady(QString, map<string, bool>, Htt::LensFittingResults)));
    CONNECT(doConnect, m_httProcessor, SIGNAL(lensFittingResultReady(Htt::LensFittingResults)), m_wizard,
            SLOT(lensFittingResultReady(Htt::LensFittingResults)));
    CONNECT(doConnect, m_httProcessor, SIGNAL(profileDetectionResultReady(Htt::ProfileDetectionResults)), m_wizard,
            SLOT(profileDetectionResultReady(Htt::ProfileDetectionResults)));
    // ImageViewerController
    CONNECT(doConnect, m_viewerController, SIGNAL(screenshotReady(QSharedPointer<QPixmap>)), m_wizard,
            SLOT(screenshotReady(QSharedPointer<QPixmap>)));
    CONNECT(doConnect, m_viewerController, SIGNAL(frameReady(QSharedPointer<QPixmap>)), m_wizard,
            SLOT(frameReady(QSharedPointer<QPixmap>)));
    CONNECT(doConnect, m_viewerController, SIGNAL(doComputeContrast(int, int, int, int)), m_wizard,
            SLOT(computeContrast(int, int, int, int)));
    CONNECT(doConnect, m_viewerController, SIGNAL(doRemoveSicingResults()), m_wizard, SLOT(removeSlicingResults()));
    // HW controller
    CONNECT(doConnect, m_hwController.get(), SIGNAL(frameAvailable(QSharedPointer<HwData>)), m_framesController.get(),
            SLOT(frameAvailable(QSharedPointer<HwData>)));
    // Calibration Wizard
    CONNECT(doConnect, m_wizard, SIGNAL(doStartGridCalibration(QSharedPointer<QImage>)), m_httProcessor,
            SLOT(calibrateGrid(QSharedPointer<QImage>)));
    CONNECT(doConnect, m_wizard,
            SIGNAL(doStartAccurateCalibration(QSharedPointer<QImage>, const double&, const double&)), m_httProcessor,
            SLOT(calibrateBevel(QSharedPointer<QImage>, const double&, const double&)));
    CONNECT(doConnect, m_wizard,
            SIGNAL(doVisualizeCalibrationResults(vector<pair<pair<int, int>, pair<int, int>>>, vector<pair<int, int>>,
                                                 QSharedPointer<QImage>, vector<pair<int, int>>)),
            m_viewerController,
            SLOT(visualizeCalibrationResults(vector<pair<pair<int, int>, pair<int, int>>>, vector<pair<int, int>>,
                                             QSharedPointer<QImage>, vector<pair<int, int>>)));
    CONNECT(doConnect, m_wizard, SIGNAL(doEnableStartCalibrationButton()), m_controlsGroupBox,
            SLOT(enableStartCalibrationButton()));
    CONNECT(doConnect, m_wizard, SIGNAL(doValidateGridCalibration(vector<float>)), m_httProcessor,
            SLOT(validateGridCalibration(vector<float>)));
    CONNECT(doConnect, m_wizard, SIGNAL(doValidateAccurateCalibration(Htt::LensFittingResults)), m_httProcessor,
            SLOT(validateAccurateCalibration(Htt::LensFittingResults)));
    CONNECT(doConnect, m_wizard, SIGNAL(doSetViewerModality(Modality)), m_viewerController,
            SLOT(setModality(Modality)));
    CONNECT(doConnect, m_wizard, SIGNAL(doSetViewerModality(Modality)), m_controlsGroupBox,
            SLOT(setModality(Modality)));
    CONNECT(doConnect, m_wizard, SIGNAL(doVisualizeImagingFoV(QSharedPointer<QImage>, vector<pair<int, int>>)),
            m_viewerController, SLOT(visualizeImagingFoV(QSharedPointer<QImage>, vector<pair<int, int>>)));
    CONNECT(doConnect, m_wizard, SIGNAL(doTakeScreenshot()), m_viewerController, SLOT(takeScreenshot()));
    CONNECT(doConnect, m_wizard, SIGNAL(doVisualizeChart(QChart*)), m_viewerController, SLOT(visualizeChart(QChart*)));
    CONNECT(doConnect, m_wizard, SIGNAL(doUpdateChart(QLineSeries*)), m_viewerController,
            SLOT(updateChart(QLineSeries*)));
    CONNECT(doConnect, m_wizard, SIGNAL(doVisualizeFrame(QSharedPointer<QImage>)), m_viewerController,
            SLOT(visualizeFrame(QSharedPointer<QImage>)));
    CONNECT(doConnect, m_wizard, SIGNAL(doStartLensFitting(QSharedPointer<QImage>, double, double, ModelType)),
            m_httProcessor, SLOT(lensFitting(QSharedPointer<QImage>, double, double, ModelType)));
    CONNECT(doConnect, m_wizard, SIGNAL(doDetectProfile(QSharedPointer<QImage>, double, double, ModelType)),
            m_httProcessor, SLOT(detectProfile(QSharedPointer<QImage>, double, double, ModelType)));
    CONNECT(doConnect, m_wizard, SIGNAL(doVisualizeLensFittingResults(Htt::LensFittingResults)), m_viewerController,
            SLOT(visualizeLensFittingResults(Htt::LensFittingResults)));
    CONNECT(doConnect, m_wizard, SIGNAL(doVisualizeProfileDetectionResults(Htt::ProfileDetectionResults)),
            m_viewerController, SLOT(visualizeProfileDetectionResults(Htt::ProfileDetectionResults)));
    CONNECT(doConnect, m_wizard, SIGNAL(doCalibrationDone(bool)), this, SLOT(calibrationDone(bool)));
    CONNECT(doConnect, m_wizard, SIGNAL(doResetImageViewerController()), m_viewerController, SLOT(reset()));
    CONNECT(doConnect, m_wizard, SIGNAL(updateStatusBar(QString)), this, SLOT(updateStatusBar(QString)));
    CONNECT(doConnect, m_wizard, SIGNAL(doVisualizeSlicingConstrastLines(QPointF, QPointF, QPointF, QPointF)),
            m_viewerController, SLOT(visualizeSlicingContrastLines(QPointF, QPointF, QPointF, QPointF)));

    // Frames controller
    CONNECT(doConnect, m_framesController.get(), SIGNAL(doSaveFrame(QSharedPointer<HwData>)), m_wizard,
            SLOT(frameAvailable(QSharedPointer<HwData>)));
    // ControlGroupBox
    CONNECT(doConnect, m_controlsGroupBox, SIGNAL(doEnableProgressNextButton(bool)), m_wizard,
            SLOT(enableProgressNextButton(bool)));
    CONNECT(doConnect, m_controlsGroupBox, SIGNAL(doStartGridCalibration()), m_wizard,
            SLOT(startGridCalibrationRequired()));
    CONNECT(doConnect, m_controlsGroupBox, SIGNAL(doStartAccurateCalibration(const double&, const double&)), m_wizard,
            SLOT(startAccurateCalibrationRequired(const double&, const double&)));
    CONNECT(doConnect, m_controlsGroupBox, SIGNAL(doManualGrab(int, int, int)), m_grabbingGroupBox,
            SLOT(manualGrabRequired(int, int, int)));
    CONNECT(doConnect, m_controlsGroupBox, SIGNAL(doDetectProfile()), m_grabbingGroupBox, SLOT(detectProfile()));
    CONNECT(doConnect, m_controlsGroupBox, SIGNAL(doResetImageViewerController()), m_viewerController, SLOT(reset()));
    CONNECT(doConnect, m_controlsGroupBox, SIGNAL(doSetViewerModality(Modality)), m_viewerController,
            SLOT(setModality(Modality)));
    // GrabbingGroupBox
    CONNECT(doConnect, m_grabbingGroupBox, SIGNAL(doManualGrabRequired(QString, QString)), m_wizard,
            SLOT(manualGrabRequired(QString, QString)));
    CONNECT(doConnect, m_grabbingGroupBox, SIGNAL(doDetectProfile(const double&, const double&, QString)), m_wizard,
            SLOT(detectProfileRequired(const double&, const double&, QString)));
}

void MainWindow::mgmtLiveSlots(bool doConnect)
{
    // HttProcessor
    CONNECT(doConnect, m_httProcessor, SIGNAL(lensFittingResultReady(Htt::LensFittingResults)), m_live,
            SLOT(lensFittingResultReady(Htt::LensFittingResults)));
    CONNECT(doConnect, m_httProcessor, SIGNAL(processingFinished()), m_framesController.get(),
            SLOT(processingFinished()));
    // HW controller
    CONNECT(doConnect, m_hwController.get(), SIGNAL(frameAvailable(QSharedPointer<HwData>)), m_framesController.get(),
            SLOT(frameAvailable(QSharedPointer<HwData>)));
    // Live Acquisition
    CONNECT(doConnect, m_live, SIGNAL(doResetImageViewerController()), m_viewerController, SLOT(reset()));
    CONNECT(doConnect, m_live, SIGNAL(doSetViewerModality(Modality)), m_viewerController, SLOT(setModality(Modality)));
    CONNECT(doConnect, m_live, SIGNAL(doSetViewerModality(Modality)), m_controlsGroupBox, SLOT(setModality(Modality)));
    CONNECT(doConnect, m_live, SIGNAL(doVisualizeFrame(QSharedPointer<QImage>)), m_viewerController,
            SLOT(visualizeFrame(QSharedPointer<QImage>)));
    CONNECT(doConnect, m_live, SIGNAL(doVisualizeLensFittingResults(Htt::LensFittingResults)), m_viewerController,
            SLOT(visualizeLensFittingResults(Htt::LensFittingResults)));
    CONNECT(doConnect, m_live, SIGNAL(doStartLensFitting(QSharedPointer<QImage>, double, double, ModelType)),
            m_httProcessor, SLOT(lensFitting(QSharedPointer<QImage>, double, double, ModelType)));
    // Frames controller
    CONNECT(doConnect, m_framesController.get(), SIGNAL(doSaveFrame(QSharedPointer<HwData>)), m_live,
            SLOT(saveFrame(QSharedPointer<HwData>)));
    CONNECT(doConnect, m_framesController.get(),
            SIGNAL(doStartLensFitting(QSharedPointer<QImage>, double, double, ModelType)), m_httProcessor,
            SLOT(lensFitting(QSharedPointer<QImage>, double, double, ModelType)));
    CONNECT(doConnect, m_live, SIGNAL(updateStatusBar(QString)), this, SLOT(updateStatusBar(QString)));
    // ControlGroupBox
    CONNECT(doConnect, m_controlsGroupBox, SIGNAL(doManualGrab(int, int, int)), m_live,
            SLOT(manualGrabRequired(int, int, int))); // FIXME - replace with grabbing group box?
    CONNECT(doConnect, m_controlsGroupBox, SIGNAL(doChangeProcessingRealTimeState(bool)), m_framesController.get(),
            SLOT(enableRealTimeProcessing(bool)));
    CONNECT(doConnect, m_controlsGroupBox, SIGNAL(doManualLensFitting(int)), m_live,
            SLOT(manualLensFittingRequired(int)));
    CONNECT(doConnect, m_controlsGroupBox, SIGNAL(doResetImageViewerController()), m_viewerController, SLOT(reset()));
    CONNECT(doConnect, m_controlsGroupBox, SIGNAL(doSetViewerModality(Modality)), m_viewerController,
            SLOT(setModality(Modality)));
}
