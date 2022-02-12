#include "CalibrationWizard.h"
#include "CalibrationReportGenerator.h"
#include "HttUtils.h"
#include "Notifications.h"
#include "PeakFinder.h"

#include <QBuffer>

#include <QDebug>
#include <QFileDialog>
#include <QFinalState>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QPushButton>
#include <QState>

#include <map>

using namespace std;

// #define DEBUG

CalibrationWizard::CalibrationWizard(ControlGroupBox* controlsGroupBox, GrabbingGroupBox* grabbingGroupBox,
                                     shared_ptr<HwController> hwController, QObject* parent)
    : m_sm(new QStateMachine(this)),
      m_controlsGroupBox(controlsGroupBox),
      m_grabbingGroupBox(grabbingGroupBox),
      m_validationTableTitle(nullptr),
      m_validationTableResults(nullptr),
      m_calibrationTableTitle(nullptr),
      m_calibrationTableResults(nullptr),
      m_progressBackButtonClicked(false),
      m_httLightOutputFluxTable(nullptr),
      m_httLightOutputFluxChart(nullptr),
      m_hwController(hwController),
      m_resumeTransaction(nullptr),
      m_verticalContrastChart(nullptr),
      m_horizontalContrastChart(nullptr),
      m_verticalConstrastText(nullptr),
      m_horizontalConstrastText(nullptr)
{
    // Vertical Layout used in the calibration tab
    QVBoxLayout* calibrationVerticalLayout = new QVBoxLayout();
    ((QWidget*)parent)->setLayout(calibrationVerticalLayout);

    // Title label
    QLabel* titleLabel = new QLabel();
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setTextFormat(Qt::RichText);
    titleLabel->setText("<b>CALIBRATION WIZARD</b>");
    calibrationVerticalLayout->addWidget(titleLabel);

    m_statusLabel = new QLabel();
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setTextFormat(Qt::RichText);
    calibrationVerticalLayout->addWidget(m_statusLabel);

    // GroupBox with two labels:
    //  1) step index and objective
    //  2) step comment
    m_stepsGroupBox = new QGroupBox();
    QGridLayout* stepsGrid = new QGridLayout;
    m_stepsGroupBox->setLayout(stepsGrid);
    calibrationVerticalLayout->addWidget(m_stepsGroupBox);
    m_stepsGroupBox->setTitle("CALIBRATION PROGRESS");
    m_stepsGroupBox->setVisible(false);
    m_stepsLabel = new QLabel();
    m_stepsLabel->setTextFormat(Qt::RichText);
    stepsGrid->addWidget(m_stepsLabel);

    m_stepsComment = new QLabel();
    m_stepsComment->setTextFormat(Qt::RichText);
    stepsGrid->addWidget(m_stepsComment);

    // GroupBox with a TextBrowser used for visualize the user instructions
    m_instructionGroupBox = new QGroupBox();
    QGridLayout* instructionGrid = new QGridLayout;
    m_instructionGroupBox->setLayout(instructionGrid);
    m_userInstructionTextBrowser = new QTextBrowser();
    m_instructionGroupBox->setTitle("USER INSTRUCTIONS");
    m_instructionGroupBox->setVisible(false);
    m_instructionGroupBox->setStyleSheet("QTextEdit {border: 0px solid black;}");
    instructionGrid->addWidget(m_userInstructionTextBrowser);
    calibrationVerticalLayout->addWidget(m_instructionGroupBox);

    // GroupBox with the grabbing parameters
    calibrationVerticalLayout->addWidget(m_grabbingGroupBox);

    // GroupBox used for visualize the output results
    m_outputGroupBox = new QGroupBox();
    QGridLayout* outputGrid = new QGridLayout;
    m_outputGroupBox->setLayout(outputGrid);
    m_outputResultsTextBrowser = new QTextBrowser();
    m_outputGroupBox->setTitle("RESULTS");
    m_outputGroupBox->setVisible(false);
    m_outputGroupBox->setStyleSheet("QTextEdit {border: 0px solid black;}");
    outputGrid->addWidget(m_outputResultsTextBrowser);
    calibrationVerticalLayout->addWidget(m_outputGroupBox);

    //    // Spacer
    //    QSpacerItem* spacer = new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);
    //    calibrationVerticalLayout->addSpacerItem(spacer);

    // TODO: replace with ButtonGroup
    // Widget with progess buttons: NEXT and BACK
    m_progressButtonsWidget = new QWidget();
    calibrationVerticalLayout->addWidget(m_progressButtonsWidget);
    QHBoxLayout* progessButtonsLayout = new QHBoxLayout();
    m_progressButtonsWidget->setLayout(progessButtonsLayout);
    m_progressNextButton = new QPushButton("Save and proceed");
    m_progressBackButton = new QPushButton("Back");
    progessButtonsLayout->addWidget(m_progressBackButton);
    progessButtonsLayout->addWidget(m_progressNextButton);
    m_progressButtonsWidget->setVisible(false);

    // FIXME: add spacer
    // QSpacerItem* spacer = new QSpacerItem();
    // calibrationVerticalLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding));

    // TODO: maybe we will replace with a ButtonGroup
    // Widget with an horizontal layout with START and RESUME buttons
    m_startingSelectionWidget = new QWidget();
    calibrationVerticalLayout->addWidget(m_startingSelectionWidget);
    QHBoxLayout* startingSelectionLayout = new QHBoxLayout();
    m_startingSelectionWidget->setLayout(startingSelectionLayout);
    m_startWizardButton = new QPushButton("Start Calibration");
    m_resumeWizardButton = new QPushButton("Resume Calibration");
    startingSelectionLayout->addWidget(m_startWizardButton);
    startingSelectionLayout->addWidget(m_resumeWizardButton);

    // Initialize the state machine
    initStateMachine();

    connect(m_startWizardButton, SIGNAL(clicked(bool)), this, SLOT(pushButtonClickedStartWizard(bool)));
    connect(m_resumeWizardButton, SIGNAL(clicked(bool)), this, SLOT(pushButtonClickedResumeWizard(bool)));
    connect(m_progressBackButton, SIGNAL(clicked(bool)), this, SLOT(pushButtonClickedProgressBack(bool)));

    // Start wizard
    start();
}

CalibrationWizard::~CalibrationWizard()
{
    delete m_startWizardButton;
    delete m_resumeWizardButton;
    delete m_startingSelectionWidget;
    delete m_userInstructionTextBrowser;
    delete m_stepsLabel;
    delete m_stepsComment;
    delete m_stepsGroupBox;
    delete m_progressNextButton;
    delete m_progressBackButton;
    delete m_progressButtonsWidget;

    m_sm->stop();
}

void CalibrationWizard::start()
{
    qDebug() << "CalibrationWizard::start()";
    emit updateStatusBar("Ready");

    loadJsonState();
    bool resumeButtonEnabled = m_state.contains("current");
    m_resumeWizardButton->setEnabled(resumeButtonEnabled);
    m_statusLabel->setVisible(true);

    QString text;
    if (m_state.contains("calibrated") && m_state["calibrated"].toBool())
        text.append("<h1 style=\"color : green;\">Device calibrated</h1>");
    else
        text.append("<h1 style=\"color : red;\">Device not calibrated</h1>");

    text.append(Notifications::startCalibrationInstruction);
    m_statusLabel->setText(text);
    m_sm->start();
}
void CalibrationWizard::stop()
{
    qDebug() << "CalibrationWizard::stop()";
    if (m_sm->isRunning())
        m_sm->stop();

    emit doResetImageViewerController();

    m_hwController->stopCamera();

    removeHttLightOutputFluxTable();
    removeCalibrationTableResults();
    removeSlicingResults();
    m_outputResultsTextBrowser->setVisible(true);

    // Remove pointer to gridImage
    if (m_frame)
        m_frame = nullptr;

    m_outputResultsTextBrowser->setText("");
}

void CalibrationWizard::resume()
{
    qDebug() << "CalibrationWizard::resume()";
    // connect(m_sm, SIGNAL(started()), m_resumeWizardButton, SLOT(pushButtonClickedResumeWizard(bool)));

    m_sm->start();
    // m_resumeWizardButton->click();
}

void CalibrationWizard::initStateMachine()
{
    m_idle = new QState();
    m_step1 = new QState();
    m_step2 = new QState();
    m_step3 = new QState();
    m_step4 = new QState();
    m_step5 = new QState();
    m_step6 = new QState();
    m_step7 = new QState();
    m_step8 = new QState();
    m_step9 = new QState();
    m_step10 = new QState();
    m_done = new QFinalState();

    m_idle->assignProperty(m_stepsLabel, "text", "IDLE");

    m_step1->assignProperty(m_stepsLabel, "text", "<b>[1/10]</b> " + Notifications::step1Objective);
    m_step1->assignProperty(m_stepsComment, "text", Notifications::italicString(Notifications::step1Comment));
    m_step1->assignProperty(m_userInstructionTextBrowser, "html", Notifications::userInstructionsStep1);

    m_step2->assignProperty(m_stepsLabel, "text", "<b>[2/10]</b> " + Notifications::step2Objective);
    m_step2->assignProperty(m_stepsComment, "text", Notifications::italicString(Notifications::step2Comment));
    m_step2->assignProperty(m_userInstructionTextBrowser, "html", Notifications::userInstructionsStep2);

    m_step3->assignProperty(m_stepsLabel, "text", "<b>[3/10]</b> " + Notifications::step3Objective);
    m_step3->assignProperty(m_stepsComment, "text", Notifications::italicString(Notifications::step3Comment));
    m_step3->assignProperty(m_userInstructionTextBrowser, "html", Notifications::userInstructionsStep3);

    m_step4->assignProperty(m_stepsLabel, "text", "<b>[4/10]</b> " + Notifications::step4Objective);
    m_step4->assignProperty(m_stepsComment, "text", Notifications::italicString(Notifications::step4Comment));
    m_step4->assignProperty(m_userInstructionTextBrowser, "html", Notifications::userInstructionsStep4);

    m_step5->assignProperty(m_stepsLabel, "text", "<b>[5/10]</b> " + Notifications::step5Objective);
    m_step5->assignProperty(m_stepsComment, "text", Notifications::italicString(Notifications::step5Comment));
    m_step5->assignProperty(m_userInstructionTextBrowser, "html", Notifications::userInstructionsStep5);

    m_step6->assignProperty(m_stepsLabel, "text", "<b>[6/10]</b> " + Notifications::step6Objective);
    m_step6->assignProperty(m_stepsComment, "text", Notifications::italicString(Notifications::step6Comment));
    m_step6->assignProperty(m_userInstructionTextBrowser, "html", Notifications::userInstructionsStep6);

    m_step7->assignProperty(m_stepsLabel, "text", "<b>[7/10]</b> " + Notifications::step7Objective);
    m_step7->assignProperty(m_stepsComment, "text", Notifications::italicString(Notifications::step7Comment));
    m_step7->assignProperty(m_userInstructionTextBrowser, "html", Notifications::userInstructionsStep7);

    m_step8->assignProperty(m_stepsLabel, "text", "<b>[8/10]</b> " + Notifications::step8Objective);
    m_step8->assignProperty(m_stepsComment, "text", Notifications::italicString(Notifications::step8Comment));
    m_step8->assignProperty(m_userInstructionTextBrowser, "html", Notifications::userInstructionsStep8);

    m_step9->assignProperty(m_stepsLabel, "text", "<b>[9/10]</b> " + Notifications::step9Objective);
    m_step9->assignProperty(m_stepsComment, "text", Notifications::italicString(Notifications::step9Comment));
    m_step9->assignProperty(m_userInstructionTextBrowser, "html", Notifications::userInstructionsStep9);

    m_step10->assignProperty(m_stepsLabel, "text", "<b>[10/10]</b> " + Notifications::step10Objective);
    m_step10->assignProperty(m_stepsComment, "text", Notifications::italicString(Notifications::step10Comment));
    m_step10->assignProperty(m_userInstructionTextBrowser, "html", Notifications::userInstructionsStep10);

    m_idle->addTransition(m_startWizardButton, SIGNAL(clicked(bool)), m_step1);

    m_step1->addTransition(this, SIGNAL(progressBackButtonClicked()), m_idle);
    m_step1->addTransition(m_progressNextButton, SIGNAL(clicked(bool)), m_step2);

    m_step2->addTransition(this, SIGNAL(progressBackButtonClicked()), m_step1);
    m_step2->addTransition(m_progressNextButton, SIGNAL(clicked(bool)), m_step3);

    m_step3->addTransition(this, SIGNAL(progressBackButtonClicked()), m_step2);
    m_step3->addTransition(m_progressNextButton, SIGNAL(clicked(bool)), m_step4);

    m_step4->addTransition(this, SIGNAL(progressBackButtonClicked()), m_step3);
    m_step4->addTransition(m_progressNextButton, SIGNAL(clicked(bool)), m_step5);

    m_step5->addTransition(this, SIGNAL(progressBackButtonClicked()), m_step4);
    m_step5->addTransition(m_progressNextButton, SIGNAL(clicked(bool)), m_step6);

    m_step6->addTransition(this, SIGNAL(progressBackButtonClicked()), m_step5);
    m_step6->addTransition(m_progressNextButton, SIGNAL(clicked(bool)), m_step7);

    m_step7->addTransition(this, SIGNAL(progressBackButtonClicked()), m_step6);
    m_step7->addTransition(m_progressNextButton, SIGNAL(clicked(bool)), m_step8);

    m_step8->addTransition(this, SIGNAL(progressBackButtonClicked()), m_step7);
    m_step8->addTransition(m_progressNextButton, SIGNAL(clicked(bool)), m_step9);

    m_step9->addTransition(this, SIGNAL(progressBackButtonClicked()), m_step8);
    m_step9->addTransition(m_progressNextButton, SIGNAL(clicked(bool)), m_step10);

    m_step10->addTransition(this, SIGNAL(progressBackButtonClicked()), m_step9);
    m_step10->addTransition(m_progressNextButton, SIGNAL(clicked(bool)), m_done);

    m_sm->addState(m_idle);
    m_sm->addState(m_step1);
    m_sm->addState(m_step2);
    m_sm->addState(m_step3);
    m_sm->addState(m_step4);
    m_sm->addState(m_step5);
    m_sm->addState(m_step6);
    m_sm->addState(m_step7);
    m_sm->addState(m_step8);
    m_sm->addState(m_step9);
    m_sm->addState(m_step10);
    m_sm->addState(m_done);
    m_sm->setInitialState(m_idle);

    QObject::connect(m_idle, &QStateMachine::entered, this, &CalibrationWizard::enteredIdle);
    QObject::connect(m_idle, &QStateMachine::exited, this, &CalibrationWizard::exitedIdle);
    QObject::connect(m_step1, &QStateMachine::entered, this, &CalibrationWizard::enteredStep1);
    QObject::connect(m_step1, &QStateMachine::exited, this, &CalibrationWizard::exitedStep1);
    QObject::connect(m_step2, &QStateMachine::entered, this, &CalibrationWizard::enteredStep2);
    QObject::connect(m_step2, &QStateMachine::exited, this, &CalibrationWizard::exitedStep2);
    QObject::connect(m_step3, &QStateMachine::entered, this, &CalibrationWizard::enteredStep3);
    QObject::connect(m_step3, &QStateMachine::exited, this, &CalibrationWizard::exitedStep3);
    QObject::connect(m_step4, &QStateMachine::entered, this, &CalibrationWizard::enteredStep4);
    QObject::connect(m_step4, &QStateMachine::exited, this, &CalibrationWizard::exitedStep4);
    QObject::connect(m_step5, &QStateMachine::entered, this, &CalibrationWizard::enteredStep5);
    QObject::connect(m_step5, &QStateMachine::exited, this, &CalibrationWizard::exitedStep5);
    QObject::connect(m_step6, &QStateMachine::entered, this, &CalibrationWizard::enteredStep6);
    QObject::connect(m_step6, &QStateMachine::exited, this, &CalibrationWizard::exitedStep6);
    QObject::connect(m_step7, &QStateMachine::entered, this, &CalibrationWizard::enteredStep7);
    QObject::connect(m_step7, &QStateMachine::exited, this, &CalibrationWizard::exitedStep7);
    QObject::connect(m_step8, &QStateMachine::entered, this, &CalibrationWizard::enteredStep8);
    QObject::connect(m_step8, &QStateMachine::exited, this, &CalibrationWizard::exitedStep8);
    QObject::connect(m_step9, &QStateMachine::entered, this, &CalibrationWizard::enteredStep9);
    QObject::connect(m_step9, &QStateMachine::exited, this, &CalibrationWizard::exitedStep9);
    QObject::connect(m_step10, &QStateMachine::entered, this, &CalibrationWizard::enteredStep10);
    QObject::connect(m_step10, &QStateMachine::exited, this, &CalibrationWizard::exitedStep10);
    QObject::connect(m_done, &QStateMachine::entered, this, &CalibrationWizard::enteredDone);
    QObject::connect(m_sm, &QStateMachine::finished, this, &CalibrationWizard::finishedDone);
}

void CalibrationWizard::loadJsonState()
{
    m_state = HttUtils::loadJsonFile("config/status.json");
}

void CalibrationWizard::saveJsonState()
{
    QFile file("config/status.json");
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
    {
        qDebug() << "File open error";
        return;
    }

    QJsonDocument jsonDoc;
    jsonDoc.setObject(m_state);

    // Write json to the file as text and close the file.
    file.write(jsonDoc.toJson());
    file.close();
}

void CalibrationWizard::enteredIdle()
{
    emit doSetViewerModality(Modality::CALIBRATION_IDLE);
    // m_state["current"] = "idle";
    // saveJsonState();
    m_startingSelectionWidget->setVisible(true);
    m_instructionGroupBox->setVisible(false);
    m_grabbingGroupBox->setVisible(false);
    m_outputGroupBox->setVisible(false);
    m_progressButtonsWidget->setVisible(false);
    m_stepsGroupBox->setVisible(false);

    // if (m_state["calibrated"].toBool())
    //     m_resumeWizardButton->click();
}
void CalibrationWizard::exitedIdle()
{
}

void CalibrationWizard::enteredStep1()
{
    emit doSetViewerModality(Modality::CALIBRATION_1);
    m_state["current"] = "step1";
    m_statusLabel->setVisible(false);
    m_currentStep = {};
    saveJsonState();
    m_progressNextButton->setEnabled(true);
}
void CalibrationWizard::exitedStep1()
{
    if (!m_progressBackButtonClicked)
    {
        m_state["step1"] = m_currentStep;
        saveJsonState();
    } else
    {
        m_statusLabel->setVisible(true);
        m_progressBackButtonClicked = false;
    }
    m_outputResultsTextBrowser->setText(Notifications::emptyString);
}

void CalibrationWizard::enteredStep2()
{
    emit doSetViewerModality(Modality::CALIBRATION_2);
    m_state["current"] = "step2";
    m_currentStep = {};
    saveJsonState();

    if (m_state.contains("step2"))
    {
        m_currentStep = m_state["step2"].toObject();

        if (m_currentStep.contains("frame"))
        {
            // Load frame (image plane)
            QByteArray imgFrameImageArray = QByteArray::fromBase64(m_currentStep["frame"].toString().toUtf8());
            QImage* img = new QImage();
            img->loadFromData(imgFrameImageArray, "PNG");
            m_frame = QSharedPointer<HwData>(new HwData());
            m_frame->image = QSharedPointer<QImage>(img);
            emit doVisualizeFrame(m_frame->image);
        }
        if (m_currentStep.contains("vPoints") && m_currentStep.contains("hPoints"))
        {
            QJsonObject vPoints = m_currentStep["vPoints"].toObject();
            QJsonObject hPoints = m_currentStep["hPoints"].toObject();

            QPointF p1(vPoints["x1"].toDouble(), vPoints["y1"].toDouble());
            QPointF p2(vPoints["x2"].toDouble(), vPoints["y2"].toDouble());
            QPointF p3(hPoints["x1"].toDouble(), hPoints["y1"].toDouble());
            QPointF p4(hPoints["x2"].toDouble(), hPoints["y2"].toDouble());

            computeContrast(p1.x(), p1.y(), p2.x(), p2.y());
            computeContrast(p3.x(), p3.y(), p4.x(), p4.y());

            emit doVisualizeSlicingConstrastLines(p1, p2, p3, p4);
        }

    } else
    {
        m_currentStep = {};
        m_progressNextButton->setEnabled(false);

#ifndef DEBUG
        // Start live acquisition from camera
        m_hwController->startCamera(HwController::GrabbingMode::ContinuousAcquisition);
#else
        // FIXME - only for debug
        QImage* img = new QImage("img/usaf_chart_FM2002_ima1.png");
        m_frame = QSharedPointer<HwData>(new HwData());
        m_frame->image = QSharedPointer<QImage>(img);
        emit doVisualizeFrame(m_frame->image);
        // m_progressNextButton->setEnabled(true);
#endif
    }
}
void CalibrationWizard::exitedStep2()
{
    if (!m_progressBackButtonClicked)
    {
        m_state["step2"] = m_currentStep;

        // Save chartView
        QPixmap vChart(m_verticalContrastChart->size());
        QPainter* paint = new QPainter(&vChart);
        m_verticalContrastChart->render(paint);
        QByteArray arr;
        QBuffer buff(&arr);
        vChart.save(&buff, "PNG");
        m_currentStep.insert("vChart", QString(arr.toBase64()));

        QPixmap hChart(m_horizontalContrastChart->size());
        paint = new QPainter(&hChart);
        m_horizontalContrastChart->render(paint);
        QByteArray arr2;
        QBuffer buff2(&arr2);
        hChart.save(&buff2, "PNG");
        m_currentStep.insert("hChart", QString(arr2.toBase64()));
        // saveJsonState();

        // TODO: save all single layers?
        emit doTakeScreenshot();
    } else
    {
        m_progressBackButtonClicked = false;

        // Stop camera
        m_hwController->stopCamera();
    }

    removeSlicingResults();
    m_outputResultsTextBrowser->setVisible(true);

    // Enable progerssNextButton
    m_progressNextButton->setEnabled(true);
}

void CalibrationWizard::enteredStep3()
{
    emit doSetViewerModality(Modality::CALIBRATION_3);
    m_state["current"] = "step3";
    m_currentStep = {};
    saveJsonState();

    if (m_state.contains("step3"))
    {
        m_currentStep = m_state["step3"].toObject();
        if (m_currentStep.contains("gridImage"))
        {
            // Load frame (image plane)
            QJsonObject gImage = m_currentStep["gridImage"].toObject();
            QByteArray imgFrameImageArray = QByteArray::fromBase64(gImage["data"].toString().toUtf8());

            QImage* img = new QImage();
            img->loadFromData(imgFrameImageArray, "PNG");
            QSharedPointer<QImage> imgFrameImage = QSharedPointer<QImage>(img);
            emit doVisualizeFrame(imgFrameImage);
        }
    } else
    {
        m_currentStep = {};
        m_progressNextButton->setEnabled(false);

#ifndef DEBUG
        // Start live acquisition from camera
        m_hwController->startCamera(HwController::GrabbingMode::ContinuousAcquisition);
#else
        // FIXME - only for debug
        QImage* img = new QImage("img/grid_lineDetectionOK.png");
        QSharedPointer<QImage> imgFrameImage = QSharedPointer<QImage>(img);
        emit doVisualizeFrame(imgFrameImage);
        m_progressNextButton->setEnabled(true);
#endif
    }
}
void CalibrationWizard::exitedStep3()
{
    if (!m_progressBackButtonClicked)
    {
        m_state["step3"] = m_currentStep;

        // TODO: save all single layers?
        emit doTakeScreenshot();
    } else
    {
        m_progressBackButtonClicked = false;

        // Stop camera
        m_hwController->stopCamera();
    }

    // Remove pointer to gridImage
    if (m_gridImage)
        m_gridImage = nullptr;

    m_outputResultsTextBrowser->setText(Notifications::emptyString);

    // Enable progerssNextButton
    m_progressNextButton->setEnabled(true);
}

void CalibrationWizard::enteredStep4()
{
    emit doSetViewerModality(Modality::CALIBRATION_4);

    m_state["current"] = "step4";

    if (m_state.contains("step4"))
    {
        m_currentStep = m_state["step4"].toObject();

        // 1) Load imageViewer configurations
        // 2) Load the table used for visualize the results

        // Load gridImage form step3
        QJsonObject step3 = m_state["step3"].toObject();
        QJsonObject gridImage = step3["gridImage"].toObject();
        QByteArray imgArray = QByteArray::fromBase64(gridImage["data"].toString().toUtf8());
        QImage* img = new QImage();
        img->loadFromData(imgArray, "PNG");
        m_gridImage = QSharedPointer<QImage>(img);
        emit doVisualizeFrame(m_gridImage);
        // **************************************************************************************************

        if (m_currentStep.contains("results"))
        {
            QJsonObject results = m_currentStep["results"].toObject();

            // Load objImage
            QJsonObject objImage = results["objImage"].toObject();
            QByteArray objImageArray = QByteArray::fromBase64(objImage["data"].toString().toUtf8());
            img = new QImage();
            img->loadFromData(objImageArray, "PNG");
            QSharedPointer<QImage> objPlaneGrid = QSharedPointer<QImage>(img);

            // Load imgLines
            vector<pair<pair<int, int>, pair<int, int>>> imgLines;
            QJsonArray imgLinesArray = results["imgPlaneGridLines"].toArray();
            for (int i = 0; i < imgLinesArray.size(); ++i)
            {
                QJsonObject p = imgLinesArray[i].toObject();
                imgLines.push_back(
                    pair<pair<int, int>, pair<int, int>>(pair<int, int>(p["x1"].toInt(), p["y1"].toInt()),
                                                         pair<int, int>(p["x2"].toInt(), p["y2"].toInt())));
            }

            // Load imgPoints
            vector<pair<int, int>> imgPoints;
            QJsonArray imgPointsArray = results["imgPlaneIntersectionPoints"].toArray();
            for (int i = 0; i < imgPointsArray.size(); ++i)
            {
                QJsonObject p = imgPointsArray[i].toObject();
                imgPoints.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
            }

            // Load objPoints
            vector<pair<int, int>> objPoints;
            QJsonArray objPointsArray = results["objPlaneIntersectionPoints"].toArray();
            for (int i = 0; i < objPointsArray.size(); ++i)
            {
                QJsonObject p = objPointsArray[i].toObject();
                objPoints.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
            }

            vector<float> gridErrors;
            QJsonArray gridErrorsArray = results["gridErrors"].toArray();
            for (int i = 0; i < gridErrorsArray.size(); ++i)
            {
                gridErrors.push_back(gridErrorsArray[i].toDouble());
            }

            emit doVisualizeCalibrationResults(imgLines, imgPoints, objPlaneGrid, objPoints);
            emit doValidateGridCalibration(gridErrors);
        }

    } else
    {
        m_currentStep = {};

        // Load gridImage form step3
        QJsonObject gridImage = m_state["step3"].toObject()["gridImage"].toObject();
        QByteArray imgArray = QByteArray::fromBase64(gridImage["data"].toString().toUtf8());
        QImage img = QImage::fromData(imgArray, "PNG");
        m_gridImage = QSharedPointer<QImage>(new QImage(img));

        // Disable progerssNextButton
        m_progressNextButton->setEnabled(false);

        emit doVisualizeFrame(m_gridImage);
    }

    saveJsonState();
}
void CalibrationWizard::exitedStep4()
{
    if (!m_progressBackButtonClicked)
    {
        m_state["step4"] = m_currentStep;
        emit doTakeScreenshot();
        // saveJsonState();
    } else
    {
        m_progressBackButtonClicked = false;
        emit doResetImageViewerController();
    }

    // Hide the calibration TableResults
    removeCalibrationTableResults();
    m_outputResultsTextBrowser->setVisible(true);

    // Remove pointer to gridImage
    if (m_gridImage)
        m_gridImage = nullptr;

    // Enable progerssNextButton
    m_progressNextButton->setEnabled(true);

    emit updateStatusBar("Ready");
}

void CalibrationWizard::enteredStep5()
{
    emit doSetViewerModality(Modality::CALIBRATION_5);
    m_state["current"] = "step5";

    if (m_state.contains("step4"))
    {
        QJsonObject step4 = m_state["step4"].toObject();
        QJsonObject results = step4["results"].toObject();

        // Load objImage
        QJsonObject objImage = results["objImage"].toObject();
        QByteArray objImageArray = QByteArray::fromBase64(objImage["data"].toString().toUtf8());
        QImage* img = new QImage();
        img->loadFromData(objImageArray, "PNG");
        QSharedPointer<QImage> objPlaneGrid = QSharedPointer<QImage>(img);

        // Load objPoints
        vector<pair<int, int>> objPoints;
        QJsonArray objPointsArray = results["objPlaneIntersectionPoints"].toArray();
        for (int i = 0; i < objPointsArray.size(); ++i)
        {
            QJsonObject p = objPointsArray[i].toObject();
            objPoints.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
        }

        emit doVisualizeImagingFoV(objPlaneGrid, objPoints);
    }

    m_currentStep = {};
    saveJsonState();
}
void CalibrationWizard::exitedStep5()
{
    if (!m_progressBackButtonClicked)
    {
        if (!m_state.contains("step5"))
            emit doTakeScreenshot();
    } else
    {
        m_progressBackButtonClicked = false;
    }

    // Remove pointer to gridImage
    if (m_gridImage)
        m_gridImage = nullptr;
}
void CalibrationWizard::enteredStep6()
{
    emit doSetViewerModality(Modality::CALIBRATION_6);
    m_state["current"] = "step6";
    m_currentStep = {};
    saveJsonState();

    if (m_state.contains("step6"))
    {
        m_currentStep = m_state["step6"].toObject();
        if (m_currentStep.contains("frame"))
        {
            // Load frame (image plane)
            QByteArray imgFrameImageArray = QByteArray::fromBase64(m_currentStep["frame"].toString().toUtf8());
            QImage* img = new QImage();
            img->loadFromData(imgFrameImageArray, "PNG");
            QSharedPointer<QImage> imgFrameImage = QSharedPointer<QImage>(img);
            emit doVisualizeFrame(imgFrameImage);
        }
    } else
    {
        m_currentStep = {};
        m_progressNextButton->setEnabled(false);

#ifndef DEBUG
        // Start live acquisition from camera
        m_hwController->startCamera(HwController::GrabbingMode::ContinuousAcquisition);
#else
        // FIXME - only for debug
        QImage* img = new QImage("img/slit_FM2002_ima2.png");
        QSharedPointer<QImage> imgFrameImage = QSharedPointer<QImage>(img);
        emit doVisualizeFrame(imgFrameImage);
        m_progressNextButton->setEnabled(true);
#endif
    }
}
void CalibrationWizard::exitedStep6()
{
    if (!m_progressBackButtonClicked)
    {
        m_state["step6"] = m_currentStep;
        emit doTakeScreenshot();
    } else
    {
        m_progressBackButtonClicked = false;

        // Stop camera
        m_hwController->stopCamera();
    }
    m_outputResultsTextBrowser->setText(Notifications::emptyString);

    // Enable progerssNextButton
    m_progressNextButton->setEnabled(true);
}
void CalibrationWizard::enteredStep7()
{
    emit doSetViewerModality(Modality::CALIBRATION_7);
    m_state["current"] = "step7";

    if (m_state.contains("step7"))
        m_currentStep = m_state["step7"].toObject();
    else
    {
        m_currentStep = {};

        // disable next button
        m_progressNextButton->setEnabled(false);
    }
    saveJsonState();

    // FIXME: only for test!!!
    // Enable controlGroupBox in the viewerController and the button for simulate the insertion
    // m_ctrlSimulateButton->setVisible(true);

    visualizeHttLightOutputFluxTable();
}
void CalibrationWizard::exitedStep7()
{
    if (!m_progressBackButtonClicked)
    {
        emit doTakeScreenshot();
    } else
    {
        m_progressBackButtonClicked = false;
    }

    // Hide the Htt Illumination path Power Table
    if (m_httLightOutputFluxTable)
    {
        m_httLightOutputFluxTable->setVisible(false);
        m_outputResultsTextBrowser->setVisible(true);
    }

    // enable next button
    m_progressNextButton->setEnabled(true);

    // FIXME: only for test!!!
    // m_ctrlSimulateButton->setVisible(false);
}
void CalibrationWizard::enteredStep8()
{
    emit doSetViewerModality(Modality::CALIBRATION_8);
    m_state["current"] = "step8";
    m_currentStep = {};
    saveJsonState();

    if (m_state.contains("step8"))
    {
        m_currentStep = m_state["step8"].toObject();
        if (m_currentStep.contains("frame"))
        {
            // Load frame (image plane)
            QByteArray imgFrameImageArray = QByteArray::fromBase64(m_currentStep["frame"].toString().toUtf8());
            QImage* img = new QImage();
            img->loadFromData(imgFrameImageArray, "PNG");
            QSharedPointer<QImage> imgFrameImage = QSharedPointer<QImage>(img);
            emit doVisualizeFrame(imgFrameImage);
        }
    } else
    {
        m_currentStep = {};
        m_progressNextButton->setEnabled(false);

#ifndef DEBUG
        // Start live acquisition from camera
        m_hwController->startCamera(HwController::GrabbingMode::ContinuousAcquisition);
#else
        // FIXME - only for debug
        QImage* img = new QImage("img/slit_delta=+0.png");
        QSharedPointer<QImage> imgFrameImage = QSharedPointer<QImage>(img);
        emit doVisualizeFrame(imgFrameImage);
#endif
    }
    m_progressNextButton->setEnabled(true);
}
void CalibrationWizard::exitedStep8()
{
    if (!m_progressBackButtonClicked)
    {
        m_state["step8"] = m_currentStep;
        emit doTakeScreenshot();
    } else
    {
        m_progressBackButtonClicked = false;

        // Stop camera
        m_hwController->stopCamera();
    }
    m_outputResultsTextBrowser->setText(Notifications::emptyString);

    // Enable progerssNextButton
    m_progressNextButton->setEnabled(true);
}
void CalibrationWizard::enteredStep9()
{
    emit doSetViewerModality(Modality::CALIBRATION_9);
    m_state["current"] = "step9";

    if (m_state.contains("step9"))
    {
        QJsonObject step9 = m_state["step9"].toObject();
        if (step9.contains("frameImage"))
        {
            QJsonObject frameImage = step9["frameImage"].toObject();
            // Load objImage
            QByteArray imgFrameImageArray = QByteArray::fromBase64(frameImage["data"].toString().toUtf8());
            QImage* img = new QImage();
            img->loadFromData(imgFrameImageArray, "PNG");
            QSharedPointer<QImage> imgFrameImage = QSharedPointer<QImage>(img);

            m_frame = QSharedPointer<HwData>(new HwData());
            m_frame->image = imgFrameImage;
            m_frame->deltaN = frameImage["deltaN"].toDouble();
            m_frame->H = frameImage["H"].toDouble();

            emit doVisualizeFrame(imgFrameImage);
        }

        if (step9.contains("results"))
        {
            QJsonObject results = step9["results"].toObject();
            Htt::LensFittingResults res;

            // Load profile in image plane
            QJsonArray imgProfileArray = results["imgProfile"].toArray();
            for (int i = 0; i < imgProfileArray.size(); ++i)
            {
                QJsonObject p = imgProfileArray[i].toObject();
                res.imgProfile.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
            }
            // Load maxima points in image plane
            QJsonArray imgMaximaPointsArray = results["imgMaximaPoints"].toArray();
            for (int i = 0; i < imgMaximaPointsArray.size(); ++i)
            {
                QJsonObject p = imgMaximaPointsArray[i].toObject();
                res.imgMaximaPoints.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
            }
            // Load bevel in image plane
            QJsonArray imgBevelArray = results["imgBevel"].toArray();
            for (int i = 0; i < imgBevelArray.size(); ++i)
            {
                QJsonObject p = imgBevelArray[i].toObject();
                res.imgBevel.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
            }
            // Load model points in image plane
            QJsonArray imgModelPointsArray = results["imgModelPoints"].toArray();
            for (int i = 0; i < imgModelPointsArray.size(); ++i)
            {
                QJsonObject p = imgModelPointsArray[i].toObject();
                res.imgModelPoints.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
            }
            // Load distance points in image plane
            QJsonArray imgDistancePointsArray = results["imgDistancePoints"].toArray();
            for (int i = 0; i < imgDistancePointsArray.size(); ++i)
            {
                QJsonObject p = imgDistancePointsArray[i].toObject();
                res.imgDistancePoints.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
            }
            // Load objImage
            QJsonObject objImage = results["objImage"].toObject();
            QByteArray objFrameImageArray = QByteArray::fromBase64(objImage["data"].toString().toUtf8());
            QImage* img = new QImage();
            img->loadFromData(objFrameImageArray, "PNG");
            res.objPlaneImage = HttUtils::convertQImageToImage(QSharedPointer<QImage>(img));
            // Load profile in object plane
            QJsonArray objProfileArray = results["objProfile"].toArray();
            for (int i = 0; i < objProfileArray.size(); ++i)
            {
                QJsonObject p = objProfileArray[i].toObject();
                res.objProfile.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
            }
            // Load maxima points in object plane
            QJsonArray objMaximaPointsArray = results["objMaximaPoints"].toArray();
            for (int i = 0; i < objMaximaPointsArray.size(); ++i)
            {
                QJsonObject p = objMaximaPointsArray[i].toObject();
                res.objMaximaPoints.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
            }
            // Load bevel in object plane
            QJsonArray objBevelArray = results["objBevel"].toArray();
            for (int i = 0; i < objBevelArray.size(); ++i)
            {
                QJsonObject p = objBevelArray[i].toObject();
                res.objBevel.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
            }
            // Load model points in object plane
            QJsonArray objModelPointsArray = results["objModelPoints"].toArray();
            for (int i = 0; i < objModelPointsArray.size(); ++i)
            {
                QJsonObject p = objModelPointsArray[i].toObject();
                res.objModelPoints.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
            }
            // Load distance points in object plane
            QJsonArray objDistancePointsArray = results["objDistancePoints"].toArray();
            for (int i = 0; i < objDistancePointsArray.size(); ++i)
            {
                QJsonObject p = objDistancePointsArray[i].toObject();
                res.objDistancePoints.push_back(pair<int, int>(p["x"].toInt(), p["y"].toInt()));
            }
            // Load measures - FIXME
            QJsonObject measures = results["measures"].toObject();
            res.measures.insert(pair<string, float>("B", measures["B"].toDouble()));
            res.measures.insert(pair<string, float>("M", measures["M"].toDouble()));
            res.realMeasures.insert(pair<string, float>("B", measures["realB"].toDouble()));
            res.realMeasures.insert(pair<string, float>("M", measures["realM"].toDouble()));
            res.errors.insert(pair<string, float>("B", measures["errorB"].toDouble()));
            res.errors.insert(pair<string, float>("M", measures["errorM"].toDouble()));

            emit doVisualizeLensFittingResults(res);
            emit doValidateAccurateCalibration(res);
        }

        m_currentStep = step9;
        m_progressNextButton->setEnabled(true);
    } else
    {
        m_currentStep = {};
        m_progressNextButton->setEnabled(false);

#ifndef DEBUG
        // Start live acquisition from camera
        // FIXME
        // m_hwController->setCameraFrameRate(2);
        // m_hwController->setCameraImageMode("M801");
        m_hwController->startCamera(HwController::GrabbingMode::ContinuousAcquisition);
#else
        // FIXME - only for debug
        QImage* img =
            new QImage("img/M804_pos=R250_H=+11.0_deltaN=+00.0_LED=100_Exp=033_G=002_ts=20210511-140918-134.png");
        QSharedPointer<QImage> imgFrameImage = QSharedPointer<QImage>(img);
        m_frame = QSharedPointer<HwData>(new HwData());
        m_frame->image = imgFrameImage;
        m_frame->deltaN = 0.0;
        m_frame->H = 11.0;
        emit doVisualizeFrame(imgFrameImage);
#endif
    }
}
void CalibrationWizard::exitedStep9()
{
    // FIXME: check if necessary
    m_hwController->stopCamera();

    if (!m_progressBackButtonClicked)
    {
        // Save m_gridImage into jsonObject
        QByteArray arr;
        QBuffer buff(&arr);
        m_frame->image->save(&buff, "PNG");

        QJsonObject objImage;
        objImage.insert("data", QString(arr.toBase64()));
        objImage.insert("source", QString("live"));
        objImage.insert("deltaN", m_frame->deltaN);
        objImage.insert("H", m_frame->H);
        m_currentStep.insert("frameImage", objImage);

        m_state["step9"] = m_currentStep;
        // saveJsonState();
        emit doTakeScreenshot();
    } else
    {
        m_progressBackButtonClicked = false;
        emit doResetImageViewerController();
    }

    // Hide the calibration TableResults
    removeCalibrationTableResults();
    m_outputResultsTextBrowser->setVisible(true);

    // Remove pointer to gridImage
    if (m_frame)
        m_frame = nullptr;

    // m_controlsGroupBox->setVisible(false);
    // m_outputResultsTextBrowser->setText("");

    // Enable progerssNextButton
    m_progressNextButton->setEnabled(true);

    emit updateStatusBar("Ready");
}
void CalibrationWizard::enteredStep10()
{
    emit doSetViewerModality(Modality::CALIBRATION_10);
    m_state["current"] = "step10";
    m_currentStep = {};
    saveJsonState();
    m_grabbingGroupBox->setVisible(true);

#ifndef DEBUG
    m_hwController->startCamera(HwController::GrabbingMode::ContinuousAcquisition);
#else
    // FIXME - only for debug
    QImage* img = new QImage("img/M801_pos=R250_H=+11.0_deltaN=+00.0_LED=100_Exp=033_G=002_ts=20210511-133547-555.png");
    QSharedPointer<QImage> imgFrameImage = QSharedPointer<QImage>(img);
    m_frame = QSharedPointer<HwData>(new HwData());
    m_frame->image = imgFrameImage;
    m_frame->deltaN = 0.0;
    m_frame->H = 11.0;
    emit doVisualizeFrame(imgFrameImage);
#endif

    // m_outputResultsTextBrowser->setText(Notifications::stepNotImplemented);
    m_progressNextButton->setText("End procedure and save report");
}
void CalibrationWizard::exitedStep10()
{
    if (!m_progressBackButtonClicked)
    {
        m_state["step10"] = m_currentStep;
        saveJsonState();
    } else
    {
        m_progressBackButtonClicked = false;
        m_hwController->stopCamera();
        m_progressNextButton->setText("Save and proceed");
    }
    m_outputResultsTextBrowser->setText(Notifications::emptyString);
    m_grabbingGroupBox->setVisible(false);
}
void CalibrationWizard::enteredDone()
{
    CalibrationReportGenerator reportGenerator(this);
    QJsonObject reportData = HttUtils::loadJsonFile("config/status.json");
    if (!reportData.isEmpty())
    {
        QString fileName =
            QFileDialog::getSaveFileName(NULL, "Save File", QDir::homePath() + "/httReport.pdf", tr("PDF (*.pdf)"));
        if (!fileName.isEmpty() && !fileName.isNull())
        {
            m_state["calibrated"] = reportGenerator.createReport(fileName, reportData);
            saveJsonState();
        }
    } else
        qDebug() << "config/status.json is empty";
}

void CalibrationWizard::finishedDone()
{
    // TODO: switch to live tab or start m_sm?
    qDebug() << "finishedDone";
    if (m_state["calibrated"].toBool())
        emit doCalibrationDone(m_state["reportSaved"].toBool());
    else
        m_sm->start();
}

void CalibrationWizard::visualizeHttLightOutputFluxTable()
{
    // Hide the textBrower with the outputResults and visualize a custom table
    m_outputResultsTextBrowser->setVisible(false);
    removeHttLightOutputFluxTable();

    // FIXME: Currently "minimum" and "nominal" values are not available for FM-2
    // QLineSeries* minimumSeries = new QLineSeries();
    // minimumSeries->setPointsVisible();
    // minimumSeries->setName("minumum");
    // QLineSeries* nominalSeries = new QLineSeries();
    // nominalSeries->setPointsVisible();
    // nominalSeries->setName("nominal");
    m_fluxSeries = new QLineSeries();
    m_fluxSeries->setPointsVisible();
    m_fluxSeries->setName("flux");

    QVariantList current;
    // QVariantList minimum;
    // QVariantList nominal;
    QVariantList flux;

    bool resumedData = false;

    if (m_currentStep.contains("table"))
    {
        QJsonObject table = m_currentStep["table"].toObject();
        current = table["current"].toArray().toVariantList();
        // minimum = table["minimum"].toArray().toVariantList();
        // nominal = table["nominal"].toArray().toVariantList();
        flux = table["flux"].toArray().toVariantList();
        resumedData = true;
    } else
    {
        QFile file(":/wizard/httLightFlux.csv");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "File open error";
            return;
        }
        file.readLine(); // discard the first line
        while (!file.atEnd())
        {
            auto line = file.readLine().split('\n').first();
            auto list = line.split(',');
            current.append(list[0].toDouble());
            // minimum.append(list[1].toDouble());
            // nominal.append(list[2].toDouble());
        }
    }

    QStringList colNames;
    colNames << "Current (%)"
             << "Flux (mW)";
    // FIXME: Currently not available for FM-2
    //  << "Minimum (mW)"
    //  << "Nominal Ref (mW)";

    m_httLightOutputFluxTable = new QTableWidget(current.size(), colNames.size(), m_outputGroupBox);
    m_httLightOutputFluxTable->setHorizontalHeaderLabels(colNames);

    for (int i = 0; i < current.size(); ++i)
    {
        // current values
        double currValue = current[i].toDouble();
        QTableWidgetItem* item = new QTableWidgetItem(QString::number(currValue));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_httLightOutputFluxTable->setItem(i, 0, item);

        // flux values
        if (flux.count() == 0)
        {
            // Initialize flux values to 0
            m_fluxSeries->append(currValue, 0);
        } else
        {
            double fluxValue = flux[i].toDouble();
            m_fluxSeries->append(currValue, fluxValue);
            m_httLightOutputFluxTable->setItem(i, 1, new QTableWidgetItem(QString::number(fluxValue)));
        }
#if 0
        // minimum values
        double minValue = minimum[i].toDouble();
        item = new QTableWidgetItem(QString::number(minValue));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_httLightOutputFluxTable->setItem(i, 2, item);
        minimumSeries->append(currValue, minValue);

        // nominal values
        double nomValue = nominal[i].toDouble();
        item = new QTableWidgetItem(QString::number(nomValue));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_httLightOutputFluxTable->setItem(i, 3, item);
        nominalSeries->append(currValue, nomValue);
#endif
    }
    m_outputGroupBox->layout()->addWidget(m_httLightOutputFluxTable);

    m_httLightOutputFluxChart = new QChart();
    // m_httLightOutputFluxChart->setTitle("HTT Light Output Flux (mW)");
    QJsonObject root = HttUtils::loadJsonFile("config/settings.json");
    QJsonObject image = root["image"].toObject();
    m_httLightOutputFluxChart->setMinimumSize(image["width"].toInt(), image["height"].toInt());
    // FIXME: Currently not available for FM-2
    // m_httLightOutputFluxChart->addSeries(minimumSeries);
    // m_httLightOutputFluxChart->addSeries(nominalSeries);
    m_httLightOutputFluxChart->addSeries(m_fluxSeries);
    m_httLightOutputFluxChart->createDefaultAxes();
    m_httLightOutputFluxChart->axisX()->setTitleText("LED Eletrical Power (%)");
    m_httLightOutputFluxChart->axisY()->setRange(0.0, 0.05);
    m_httLightOutputFluxChart->axisY()->setTitleText("HTT Light Output Flux (mW)");
    m_httLightOutputFluxChart->legend()->hide();

    if (!resumedData)
    {
        // Store the table columns in json file
        QJsonObject output;
        output.insert("current", QJsonArray::fromVariantList(current));
        // output.insert("minimum", QJsonArray::fromVariantList(minimum));
        // output.insert("nominal", QJsonArray::fromVariantList(nominal));
        m_currentStep = QJsonObject{{"table", output}};
    }

    connect(m_httLightOutputFluxTable, SIGNAL(itemChanged(QTableWidgetItem*)), this,
            SLOT(itemChangedHttLightOutputFluxTable(QTableWidgetItem*)));
    emit doVisualizeChart(m_httLightOutputFluxChart);
}

void CalibrationWizard::removeHttLightOutputFluxTable()
{
    if (m_httLightOutputFluxTable)
    {
        m_outputGroupBox->layout()->removeWidget(m_httLightOutputFluxTable);
        delete m_httLightOutputFluxTable;
        m_httLightOutputFluxTable = nullptr;
    }
}

void CalibrationWizard::removeCalibrationTableResults()
{
    if (m_calibrationTableResults)
    {
        m_outputGroupBox->layout()->removeWidget(m_calibrationTableResults);
        m_outputGroupBox->layout()->removeWidget(m_calibrationTableTitle);
        delete m_calibrationTableResults;
        delete m_calibrationTableTitle;
        m_calibrationTableResults = nullptr;
        m_calibrationTableTitle = nullptr;
    }

    if (m_validationTableResults)
    {
        m_outputGroupBox->layout()->removeWidget(m_validationTableResults);
        m_outputGroupBox->layout()->removeWidget(m_validationTableTitle);
        delete m_validationTableResults;
        delete m_validationTableTitle;
        m_validationTableResults = nullptr;
        m_validationTableTitle = nullptr;
    }
}

/* PRIVATE SLOTS */
void CalibrationWizard::pushButtonClickedStartWizard(bool)
{
    m_startingSelectionWidget->setVisible(false);
    m_instructionGroupBox->setVisible(true);
    m_outputGroupBox->setVisible(true);
    m_progressButtonsWidget->setVisible(true);
    m_stepsGroupBox->setVisible(true);

    // m_state = QJsonObject({{"calibrated", false}, {"reportSaved", false}});
    m_state = QJsonObject({{"calibrated", false}});
    saveJsonState();
}

void CalibrationWizard::pushButtonClickedResumeWizard(bool)
{
    //    m_state["reportSaved"] = false;
    m_statusLabel->setVisible(false);
    m_progressNextButton->setText("Save and proceed");

    QString currStep = m_state["current"].toString();

    m_idle->removeTransition(m_resumeTransaction);

    m_resumeTransaction = new CalibationResumeTransition("ResumeCalibration");
    if (currStep == "step1")
        m_resumeTransaction->setTargetState(m_step1);
    else if (currStep == "step2")
        m_resumeTransaction->setTargetState(m_step2);
    else if (currStep == "step3")
        m_resumeTransaction->setTargetState(m_step3);
    else if (currStep == "step4")
        m_resumeTransaction->setTargetState(m_step4);
    else if (currStep == "step5")
        m_resumeTransaction->setTargetState(m_step5);
    else if (currStep == "step6")
        m_resumeTransaction->setTargetState(m_step6);
    else if (currStep == "step7")
        m_resumeTransaction->setTargetState(m_step7);
    else if (currStep == "step8")
        m_resumeTransaction->setTargetState(m_step8);
    else if (currStep == "step9")
        m_resumeTransaction->setTargetState(m_step9);
    else if (currStep == "step10")
        m_resumeTransaction->setTargetState(m_step10);

    m_idle->addTransition(m_resumeTransaction);
    m_sm->postEvent(new CalibationResumeEvent("ResumeCalibration"));

    m_startingSelectionWidget->setVisible(false);
    m_instructionGroupBox->setVisible(true);
    m_outputGroupBox->setVisible(true);
    m_progressButtonsWidget->setVisible(true);
    m_stepsGroupBox->setVisible(true);
}

void CalibrationWizard::pushButtonClickedProgressBack(bool)
{
    m_progressBackButtonClicked = true;
    emit progressBackButtonClicked();
}

void CalibrationWizard::calibrationGridResultsReady(Htt::CalibrationGridResults result)
{
    if (result.valid)
    {
        QSharedPointer<QImage> qImage = HttUtils::convertImageToQImage(result.objPlaneGrid);

        emit doVisualizeCalibrationResults(result.imgPlaneGridLines, result.imgPlaneIntersectionPoints, qImage,
                                           result.objPlaneIntersectionPoints);
        emit doValidateGridCalibration(result.gridErrors);
        emit updateStatusBar("Calibration successful");

        // Save results in the m_currentState;
        QJsonArray imgLines;
        QJsonArray imgPoints;
        QJsonArray objPoints;
        for (auto it = result.imgPlaneGridLines.begin(); it != result.imgPlaneGridLines.end(); ++it)
        {
            QJsonObject val{
                {"x1", it->first.first}, {"y1", it->first.second}, {"x2", it->second.first}, {"y2", it->second.second}};
            imgLines.append(val);
        }
        for (auto it = result.imgPlaneIntersectionPoints.begin(); it != result.imgPlaneIntersectionPoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            imgPoints.append(val);
        }
        for (auto it = result.objPlaneIntersectionPoints.begin(); it != result.objPlaneIntersectionPoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            objPoints.append(val);
        }

        // Save qImage into jsonObject
        QByteArray arr;
        QBuffer buff(&arr);
        qImage->save(&buff, "PNG");
        QJsonObject objImage{{"data", QString(arr.toBase64())},
                             {"width", qImage->width()},
                             {"height", qImage->height()},
                             {"size", qImage->byteCount()},
                             {"format", qImage->format()}};

        QJsonObject results{{"imgPlaneGridLines", imgLines},
                            {"imgPlaneIntersectionPoints", imgPoints},
                            {"objPlaneIntersectionPoints", objPoints},
                            {"objImage", objImage}};

        m_currentStep["results"] = results;
    } else
    {
        // FIXME
        if (result.returnCode == ReturnCode::CALIBRATION_FAILED_LINE_DETECTION)
            m_outputResultsTextBrowser->setText(Notifications::lineDetectionFailed);
        else if (result.returnCode == ReturnCode::CALIBRATION_FAILED_GRID)
            m_outputResultsTextBrowser->setText(Notifications::wrongGridAcquired);
        else
            m_outputResultsTextBrowser->setText(Notifications::calibrationFailed);
        emit updateStatusBar("Calibration failed");
    }

    emit doEnableStartCalibrationButton();
}

void CalibrationWizard::accurateCalibrationResultsReady(Htt::AccurateCalibrationResults result)
{
    if (result.valid)
    {
        // emit doValidateAccurateCalibration(result.lensFittingResults.measures);
        emit doValidateAccurateCalibration(result.lensValidationResults);
        emit doVisualizeLensFittingResults(result.lensValidationResults);
        emit updateStatusBar("Calibration successful");

        // Save results in the m_currentState;
        QJsonArray imgProfile;
        QJsonArray imgMaximaPoints;
        QJsonArray imgBevel;
        QJsonArray imgModelPoints;
        QJsonArray imgDistancePoints;
        QJsonArray objProfile;
        QJsonArray objMaximaPoints;
        QJsonArray objBevel;
        QJsonArray objModelPoints;
        QJsonArray objDistancePoints;
        for (auto it = result.lensValidationResults.imgProfile.begin();
             it != result.lensValidationResults.imgProfile.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            imgProfile.append(val);
        }
        for (auto it = result.lensValidationResults.imgMaximaPoints.begin();
             it != result.lensValidationResults.imgMaximaPoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            imgMaximaPoints.append(val);
        }
        for (auto it = result.lensValidationResults.imgBevel.begin(); it != result.lensValidationResults.imgBevel.end();
             ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            imgBevel.append(val);
        }
        for (auto it = result.lensValidationResults.imgModelPoints.begin();
             it != result.lensValidationResults.imgModelPoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            imgModelPoints.append(val);
        }
        for (auto it = result.lensValidationResults.imgDistancePoints.begin();
             it != result.lensValidationResults.imgDistancePoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            imgDistancePoints.append(val);
        }
        // Obj
        for (auto it = result.lensValidationResults.objProfile.begin();
             it != result.lensValidationResults.objProfile.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            objProfile.append(val);
        }
        for (auto it = result.lensValidationResults.objMaximaPoints.begin();
             it != result.lensValidationResults.objMaximaPoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            objMaximaPoints.append(val);
        }
        for (auto it = result.lensValidationResults.objBevel.begin(); it != result.lensValidationResults.objBevel.end();
             ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            objBevel.append(val);
        }
        for (auto it = result.lensValidationResults.objModelPoints.begin();
             it != result.lensValidationResults.objModelPoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            objModelPoints.append(val);
        }
        for (auto it = result.lensValidationResults.objDistancePoints.begin();
             it != result.lensValidationResults.objDistancePoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            objDistancePoints.append(val);
        }

        // Save qImage into jsonObject
        QByteArray arr;
        QBuffer buff(&arr);
        QSharedPointer<QImage> objImage = HttUtils::convertImageToQImage(result.lensValidationResults.objPlaneImage);
        objImage->save(&buff, "PNG");
        QJsonObject objImg{{"data", QString(arr.toBase64())},
                           {"width", objImage->width()},
                           {"height", objImage->height()},
                           {"size", objImage->byteCount()},
                           {"format", objImage->format()}};
        QJsonObject measures;
        measures.insert("B", result.lensValidationResults.measures["B"]);
        measures.insert("M", result.lensValidationResults.measures["M"]);
        measures.insert("D", result.lensValidationResults.measures["D"]);
        measures.insert("realB", result.lensValidationResults.realMeasures["B"]);
        measures.insert("realM", result.lensValidationResults.realMeasures["M"]);
        measures.insert("errorB", result.lensValidationResults.errors["B"]);
        measures.insert("errorM", result.lensValidationResults.errors["M"]);

        QJsonObject params{
            {"alpha", result.scheimpflugTransformParams["alpha"]}, {"beta", result.scheimpflugTransformParams["beta"]},
            {"cx0", result.scheimpflugTransformParams["cx0"]},     {"cy0", result.scheimpflugTransformParams["cy0"]},
            {"delta", result.scheimpflugTransformParams["delta"]}, {"p1", result.scheimpflugTransformParams["p1"]},
            {"p2", result.scheimpflugTransformParams["p2"]},       {"phi", result.scheimpflugTransformParams["phi"]},
            {"theta", result.scheimpflugTransformParams["theta"]}, {"tt", result.scheimpflugTransformParams["tt"]},
        };

        QJsonObject results;
        results.insert("imgProfile", imgProfile);
        results.insert("imgMaximaPoints", imgMaximaPoints);
        results.insert("imgBevel", imgBevel);
        results.insert("imgModelPoints", imgModelPoints);
        results.insert("imgDistancePoints", imgDistancePoints);
        results.insert("objImage", objImg);
        results.insert("objProfile", objProfile);
        results.insert("objMaximaPoints", objMaximaPoints);
        results.insert("objBevel", objBevel);
        results.insert("objModelPoints", objModelPoints);
        results.insert("objDistancePoints", objDistancePoints);
        results.insert("measures", measures);
        results.insert("params", params);

        // m_currentStep["results"] = results;
        m_currentStep.insert("results", results);

        // QSharedPointer<QImage> imgFrameImage = QSharedPointer<QImage>(img);
        QSharedPointer<QImage> imgFrameImage = HttUtils::convertImageToQImage(result.lensValidationResults.inputImage);
        m_frame = QSharedPointer<HwData>(new HwData());
        m_frame->image = imgFrameImage;
        // m_frame->deltaN = 0.0;
        // m_frame->H = 11.0;
        // emit doVisualizeFrame(imgFrameImage);

        // Save m_gridImage into jsonObject
        QByteArray arrSrcImage;
        QBuffer buffSrcImage(&arrSrcImage);
        m_frame->image->save(&buffSrcImage, "PNG");

        QJsonObject srcImage;
        srcImage.insert("data", QString(arrSrcImage.toBase64()));
        srcImage.insert("source", QString("live"));
        srcImage.insert("deltaN", m_controlsGroupBox->getDeltaN());
        srcImage.insert("H", m_controlsGroupBox->getH());
        m_currentStep.insert("frameImage", srcImage);

        emit doVisualizeFrame(m_frame->image);

        // Enable progressNextButton
        m_progressNextButton->setEnabled(true);
    } else
    {
        m_outputResultsTextBrowser->setText(Notifications::calibrationFailed);
        emit updateStatusBar("Calibration failed");
    }

    emit doEnableStartCalibrationButton();
}

void CalibrationWizard::calibrationGridValidationReady(QString result, map<string, bool> validation,
                                                       vector<float> gridErrors)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8(), &err);
    // QJsonObject calibrationJson = doc.object();
    m_calibrationJson = doc.object();

    // Hide the textBrower with the outputResults and visualize a custom table
    m_outputResultsTextBrowser->setVisible(false);

    removeCalibrationTableResults();

    QStringList colNames;
    QStringList rowNames;
    colNames << "Computed Values"
             << "Nominal Values"
             << "Min"
             << "Max";
    //  << "Boundary reached";
    rowNames << "alpha"
             << "beta"
             << "cx0"
             << "cy0"
             << "delta"
             << "p1"
             << "p2"
             << "phi"
             << "theta"
             << "tt";

    m_calibrationTableTitle = new QLabel("Scheimpflug parameters");
    m_calibrationTableResults = new QTableWidget(rowNames.size(), colNames.size(), m_outputGroupBox);
    m_calibrationTableResults->setHorizontalHeaderLabels(colNames);
    m_calibrationTableResults->setVerticalHeaderLabels(rowNames);

    QJsonObject st = m_calibrationJson["scheimpflug_transformation"].toObject();
    QJsonObject nst = m_calibrationJson["nominal_scheimpflug_transformation"].toObject();
    QJsonObject nrst = m_calibrationJson["nominal_range_scheimpflug_transformation"].toObject();

    for (int i = 0; i < rowNames.size(); ++i)
    {
        QString s = rowNames[i];
        bool boundaryCheck = validation[s.toStdString()];
        // Computed values
        QTableWidgetItem* item = new QTableWidgetItem(QString::number(st[s].toDouble(), 'g'));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (boundaryCheck)
            item->setBackground(QBrush(QColor(255, 255, 0, 32)));
        else
            item->setBackground(QBrush(QColor(0, 255, 0, 32)));
        m_calibrationTableResults->setItem(i, 0, item);
        // Nominal Values
        item = new QTableWidgetItem(QString::number(nst[s].toDouble()));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (boundaryCheck)
            item->setBackground(QBrush(QColor(255, 255, 0, 32)));
        else
            item->setBackground(QBrush(QColor(0, 255, 0, 32)));
        m_calibrationTableResults->setItem(i, 1, item);
        // min
        item = new QTableWidgetItem(QString::number(nrst[s].toObject()["min"].toDouble()));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (boundaryCheck)
            item->setBackground(QBrush(QColor(255, 255, 0, 32)));
        else
            item->setBackground(QBrush(QColor(0, 255, 0, 32)));
        m_calibrationTableResults->setItem(i, 2, item);
        // max
        item = new QTableWidgetItem(QString::number(nrst[s].toObject()["max"].toDouble()));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (boundaryCheck)
            item->setBackground(QBrush(QColor(255, 255, 0, 32)));
        else
            item->setBackground(QBrush(QColor(0, 255, 0, 32)));
        m_calibrationTableResults->setItem(i, 3, item);
        // // validation
        // item = new QTableWidgetItem(QVariant(validation[s.toStdString()]).toString());
        // item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        // if (validation[s.toStdString()])
        // {
        //     item->setFont(QFont("tahoma", 9, 65));
        //     item->setBackground(QBrush(QColor(255, 0, 0, 128)));
        // } else
        // {
        //     item->setBackground(QBrush(QColor(0, 255, 0, 128)));
        // }
        // m_calibrationTableResults->setItem(i, 4, item);
    }

    QStringList colNamesValidationTable;
    QStringList rowNamesValidationTable;
    colNamesValidationTable << "Error [um]"
                            << "Threshold [um]";
    // << "Validation";
    rowNamesValidationTable << "L1 (L/M)"
                            << "L2 (7/8)"
                            << "L3 (G/H)"
                            << "L4 (12/13)"
                            << "D1 (L1L4/L2L3)"
                            << "D2 (L3L4/L1L2)";

    m_validationTableTitle = new QLabel("Grid reconstruction errors");
    m_validationTableResults =
        new QTableWidget(rowNamesValidationTable.size(), colNamesValidationTable.size(), m_outputGroupBox);
    m_validationTableResults->setHorizontalHeaderLabels(colNamesValidationTable);
    m_validationTableResults->setVerticalHeaderLabels(rowNamesValidationTable);

    for (int i = 0; i < rowNamesValidationTable.size(); ++i)
    {
        float threshold = (i % 2 == 0) ? 16 : 10;
        if (i == 0 || i == 2)
            threshold = 16.0; // Horizontal threshold 16um
        else if (i == 1 || i == 3)
            threshold = 10.0; // vertical threshold 10um
        else
            threshold = 18.868; // Diagonal threshold sqrt(10^2+16^2)
        bool checkThreshold = abs(gridErrors[i]) < threshold;

        // QTableWidgetItem* item = new QTableWidgetItem(QString::number(gridErrors[i], 'f', 3));
        QTableWidgetItem* item = new QTableWidgetItem(QString::number(gridErrors[i], 'g'));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (!checkThreshold)
            item->setBackground(QBrush(QColor(255, 0, 0, 32)));
        else
            item->setBackground(QBrush(QColor(0, 255, 0, 32)));
        m_validationTableResults->setItem(i, 0, item);

        item = new QTableWidgetItem(QString::number(threshold));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (!checkThreshold)
            item->setBackground(QBrush(QColor(255, 0, 0, 32)));
        else
            item->setBackground(QBrush(QColor(0, 255, 0, 32)));
        m_validationTableResults->setItem(i, 1, item);

        // item = new QTableWidgetItem(QVariant(gridErrors[i] < threshold).toString());
        // item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        // m_validationTableResults->setItem(i, 2, item);
    }

    ((QGridLayout*)m_outputGroupBox->layout())->addWidget(m_validationTableTitle, 0, 0);
    ((QGridLayout*)m_outputGroupBox->layout())->addWidget(m_validationTableResults, 1, 0);
    ((QGridLayout*)m_outputGroupBox->layout())->addWidget(m_calibrationTableTitle, 2, 0);
    ((QGridLayout*)m_outputGroupBox->layout())->addWidget(m_calibrationTableResults, 3, 0);
    m_progressNextButton->setEnabled(true);

    // Save results in the m_currentState;
    QJsonObject alpha{{"unit", "deg"},
                      {"nominal", nst["alpha"].toDouble()},
                      {"tolerance", nrst["alpha"].toObject()["max"].toDouble() - nst["alpha"].toDouble()},
                      {"optimized", st["alpha"]},
                      {"delta", st["alpha"].toDouble() - nst["alpha"].toDouble()}};
    QJsonObject beta{{"unit", "deg"},
                     {"nominal", nst["beta"].toDouble()},
                     {"tolerance", nrst["beta"].toObject()["max"].toDouble() - nst["beta"].toDouble()},
                     {"optimized", st["beta"]},
                     {"delta", st["beta"].toDouble() - nst["beta"].toDouble()}};
    QJsonObject cx0{{"unit", "px"},
                    {"nominal", nst["cx0"].toDouble()},
                    {"tolerance", nrst["cx0"].toObject()["max"].toDouble() - nst["cx0"].toDouble()},
                    {"optimized", st["cx0"]},
                    {"delta", st["cx0"].toDouble() - nst["cx0"].toDouble()}};
    QJsonObject cy0{{"unit", "px"},
                    {"nominal", nst["cy0"].toDouble()},
                    {"tolerance", nrst["cy0"].toObject()["max"].toDouble() - nst["cy0"].toDouble()},
                    {"optimized", st["cy0"]},
                    {"delta", st["cy0"].toDouble() - nst["cy0"].toDouble()}};
    QJsonObject delta{{"unit", "deg"},
                      {"nominal", nst["delta"].toDouble()},
                      {"tolerance", nrst["delta"].toObject()["max"].toDouble() - nst["delta"].toDouble()},
                      {"optimized", st["delta"]},
                      {"delta", st["delta"].toDouble() - nst["delta"].toDouble()}};
    QJsonObject p1{{"unit", "mm"},
                   {"nominal", nst["p1"].toDouble()},
                   {"tolerance", nrst["p1"].toObject()["max"].toDouble() - nst["p1"].toDouble()},
                   {"optimized", st["p1"]},
                   {"delta", st["p1"].toDouble() - nst["p1"].toDouble()}};
    QJsonObject p2{{"unit", "mm"},
                   {"nominal", nst["p2"].toDouble()},
                   {"tolerance", nrst["p2"].toObject()["max"].toDouble() - nst["p2"].toDouble()},
                   {"optimized", st["p2"]},
                   {"delta", st["p2"].toDouble() - nst["p2"].toDouble()}};
    QJsonObject phi{{"unit", "deg"},
                    {"nominal", nst["phi"].toDouble()},
                    {"tolerance", nrst["phi"].toObject()["max"].toDouble() - nst["phi"].toDouble()},
                    {"optimized", st["phi"]},
                    {"delta", st["phi"].toDouble() - nst["phi"].toDouble()}};
    QJsonObject theta{{"unit", "deg"},
                      {"nominal", nst["theta"].toDouble()},
                      {"tolerance", nrst["theta"].toObject()["max"].toDouble() - nst["theta"].toDouble()},
                      {"optimized", st["theta"]},
                      {"delta", st["theta"].toDouble() - nst["theta"].toDouble()}};
    QJsonObject tt{{"unit", "mm"},
                   {"nominal", nst["tt"].toDouble()},
                   {"tolerance", nrst["tt"].toObject()["max"].toDouble() - nst["tt"].toDouble()},
                   {"optimized", st["tt"]},
                   {"delta", st["tt"].toDouble() - nst["tt"].toDouble()}};

    QJsonObject params{{"alpha", alpha}, {"beta", beta}, {"cx0", cx0}, {"cy0", cy0},     {"delta", delta},
                       {"p1", p1},       {"p2", p2},     {"phi", phi}, {"theta", theta}, {"tt", tt}};

    QJsonArray gErrors = {gridErrors[0], gridErrors[1], gridErrors[2], gridErrors[3], gridErrors[4], gridErrors[5]};
    QJsonObject results = m_currentStep["results"].toObject();
    results.insert("params", params);
    results.insert("gridErrors", gErrors);
    m_currentStep["results"] = results;
}

void CalibrationWizard::accurateCalibrationValidationReady(QString result, map<string, bool> validation,
                                                           Htt::LensFittingResults lensFittingResult)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8(), &err);
    // QJsonObject calibrationJson = doc.object();
    m_calibrationJson = doc.object();

    // Hide the textBrower with the outputResults and visualize a custom table
    m_outputResultsTextBrowser->setVisible(false);

    removeCalibrationTableResults();

    QStringList colNames;
    QStringList rowNames;
    colNames << "Computed Values"
             << "Nominal Values"
             << "Min"
             << "Max";
    //  << "Boundary reached";
    rowNames << "alpha"
             << "beta"
             << "cx0"
             << "cy0"
             << "delta"
             << "p1"
             << "p2"
             << "phi"
             << "theta"
             << "tt";

    m_calibrationTableTitle = new QLabel("Scheimpflug parameters");
    m_calibrationTableResults = new QTableWidget(rowNames.size(), colNames.size(), m_outputGroupBox);
    m_calibrationTableResults->setHorizontalHeaderLabels(colNames);
    m_calibrationTableResults->setVerticalHeaderLabels(rowNames);

    QJsonObject st = m_calibrationJson["scheimpflug_transformation"].toObject();
    QJsonObject nst = m_calibrationJson["nominal_scheimpflug_transformation"].toObject();
    QJsonObject nrst = m_calibrationJson["nominal_range_scheimpflug_transformation"].toObject();

    for (int i = 0; i < rowNames.size(); ++i)
    {
        QString s = rowNames[i];
        bool boundaryCheck = validation[s.toStdString()];
        // Computed values
        QTableWidgetItem* item = new QTableWidgetItem(QString::number(st[s].toDouble(), 'g'));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (boundaryCheck)
            item->setBackground(QBrush(QColor(255, 255, 0, 32)));
        else
            item->setBackground(QBrush(QColor(0, 255, 0, 32)));
        m_calibrationTableResults->setItem(i, 0, item);
        // Nominal Values
        item = new QTableWidgetItem(QString::number(nst[s].toDouble()));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (boundaryCheck)
            item->setBackground(QBrush(QColor(255, 255, 0, 32)));
        else
            item->setBackground(QBrush(QColor(0, 255, 0, 32)));
        m_calibrationTableResults->setItem(i, 1, item);
        // min
        item = new QTableWidgetItem(QString::number(nrst[s].toObject()["min"].toDouble()));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (boundaryCheck)
            item->setBackground(QBrush(QColor(255, 255, 0, 32)));
        else
            item->setBackground(QBrush(QColor(0, 255, 0, 32)));
        m_calibrationTableResults->setItem(i, 2, item);
        // max
        item = new QTableWidgetItem(QString::number(nrst[s].toObject()["max"].toDouble()));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (boundaryCheck)
            item->setBackground(QBrush(QColor(255, 255, 0, 32)));
        else
            item->setBackground(QBrush(QColor(0, 255, 0, 32)));
        m_calibrationTableResults->setItem(i, 3, item);
    }

    QStringList colNamesValidationTable;
    QStringList rowNamesValidationTable;
    colNamesValidationTable << "Computed Values [mm]"
                            << "Reference Values [mm]"
                            << "Errors [um]";

    rowNamesValidationTable << "B"
                            << "M";
    // << "angle1"
    // << "angle2";

    m_validationTableTitle = new QLabel("Bevel recostruction errors");
    m_validationTableResults =
        new QTableWidget(rowNamesValidationTable.size(), colNamesValidationTable.size(), m_outputGroupBox);
    m_validationTableResults->setHorizontalHeaderLabels(colNamesValidationTable);
    m_validationTableResults->setVerticalHeaderLabels(rowNamesValidationTable);

#if 0
    QJsonObject stc = m_calibrationJson["scheimpflug_transformation_calibration"].toObject();
    QJsonObject currentFrame = stc["frames"].toObject()[stc["current"].toString()].toObject();
    QTableWidgetItem* item;
    for (int i = 0; i < rowNamesValidationTable.size(); ++i)
    {
        float m = measures[rowNamesValidationTable[i].toStdString()];
        item = new QTableWidgetItem(QString::number(m, 'g'));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_validationTableResults->setItem(i, 0, item);

        float realM = currentFrame[rowNamesValidationTable[i]].toDouble();
        item = new QTableWidgetItem(QString::number(realM));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_validationTableResults->setItem(i, 1, item);

        float err = (realM - m);
        if (i < 2)
            item = new QTableWidgetItem(QString::number(err * 1000) + " [um]");
        else
            item = new QTableWidgetItem(QString::number(err) + " [deg]");
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_validationTableResults->setItem(i, 2, item);
    }
#endif
    QTableWidgetItem* item;
    for (int i = 0; i < rowNamesValidationTable.size(); ++i)
    {
        string key = rowNamesValidationTable[i].toStdString();
        float value = lensFittingResult.measures[key];
        item = new QTableWidgetItem(QString::number(value, 'g'));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_validationTableResults->setItem(i, 0, item);

        // key = "real" + rowNamesValidationTable[i].toStdString();
        float realValue = lensFittingResult.realMeasures[key];
        item = new QTableWidgetItem(QString::number(realValue, 'g'));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_validationTableResults->setItem(i, 1, item);

        // key = "error" + rowNamesValidationTable[i].toStdString();
        float error = lensFittingResult.errors[key];
        item = new QTableWidgetItem(QString::number(error, 'g'));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_validationTableResults->setItem(i, 2, item);
    }

    m_outputGroupBox->layout()->addWidget(m_validationTableTitle);
    m_outputGroupBox->layout()->addWidget(m_validationTableResults);
    m_outputGroupBox->layout()->addWidget(m_calibrationTableTitle);
    m_outputGroupBox->layout()->addWidget(m_calibrationTableResults);
    m_progressNextButton->setEnabled(true);

    // TODO
}

void CalibrationWizard::screenshotReady(QSharedPointer<QPixmap> img)
{
    // Save QPixmap img to jsonObject
    QByteArray arr;
    QBuffer buff(&arr);
    img->save(&buff, "PNG");
    m_currentStep.insert("screenshot", QString(arr.toBase64()));

    QString step = m_state["current"].toString();
    m_state[step] = m_currentStep;
    saveJsonState();
}

void CalibrationWizard::frameReady(QSharedPointer<QPixmap> img)
{
    QString step = m_state["current"].toString();
    if (step == "step2")
    {
        // Save QPixmap img to jsonObject
        QByteArray arr;
        QBuffer buff(&arr);
        img->save(&buff, "PNG");
        m_currentStep.insert("frame", QString(arr.toBase64()));

        m_state[step] = m_currentStep;
        saveJsonState();

        // and into file
        QDir dir(".");
        if (dir.mkpath("config/images/"))
        {
            QImage image(img->toImage().convertToFormat(QImage::Format_Grayscale8));
            image.save("config/images/usaf_chart.png", "PNG");
        } else
            qDebug("config/images/ creation failed");
    } else if (step == "step3")
    {
        // m_outputResultsTextBrowser->setText(Notifications::gridAcquired);

        m_gridImage = QSharedPointer<QImage>(new QImage(img->toImage()));
        m_progressNextButton->setEnabled(true);

        // Save m_gridImage into jsonObject
        QByteArray arr;
        QBuffer buff(&arr);
        m_gridImage->save(&buff, "PNG");

        QJsonObject objImage;
        objImage.insert("data", QString(arr.toBase64()));
        objImage.insert("source", QString("live"));
        m_currentStep.insert("gridImage", objImage);

        m_state[step] = m_currentStep;
        saveJsonState();

        // and into file
        QDir dir(".");
        if (dir.mkpath("config/images/"))
        {
            QImage image(img->toImage().convertToFormat(QImage::Format_Grayscale8));
            image.save("config/images/grid.png", "PNG");
        } else
            qDebug("config/images/ creation failed");
    } else if (step == "step6")
    {
        // Save QPixmap img to jsonObject
        QByteArray arr;
        QBuffer buff(&arr);
        img->save(&buff, "PNG");
        m_currentStep.insert("frame", QString(arr.toBase64()));

        m_state[step] = m_currentStep;
        saveJsonState();

        // and into file
        QDir dir(".");
        if (dir.mkpath("config/images/"))
        {
            QImage image(img->toImage().convertToFormat(QImage::Format_Grayscale8));
            image.save("config/images/slit.png", "PNG");
        } else
            qDebug("config/images/ creation failed");
    } else if (step == "step8")
    {
        // Save QPixmap img to jsonObject
        QByteArray arr;
        QBuffer buff(&arr);
        img->save(&buff, "PNG");
        m_currentStep.insert("frame", QString(arr.toBase64()));

        m_state[step] = m_currentStep;
        saveJsonState();

        // and into file
        QDir dir(".");
        if (dir.mkpath("config/images/"))
        {
            QImage image(img->toImage().convertToFormat(QImage::Format_Grayscale8));
            image.save("config/images/slit-centering.png", "PNG");
        } else
            qDebug("config/images/ creation failed");
    } else if (step == "step9")
    {
        // Save QPixmap img to jsonObject
        QByteArray arr;
        QBuffer buff(&arr);
        img->save(&buff, "PNG");
        m_currentStep.insert("frame", QString(arr.toBase64()));

        m_state[step] = m_currentStep;
        saveJsonState();

        // and into file
        QDir dir(".");
        if (dir.mkpath("config/images/"))
        {
            QImage image(img->toImage().convertToFormat(QImage::Format_Grayscale8));
            image.save("config/images/M804_H=+11.0_deltaN=+00.0.png", "PNG");
        } else
            qDebug("config/images/ creation failed");
    }
}

void CalibrationWizard::itemChangedHttLightOutputFluxTable(QTableWidgetItem* item)
{
    // Extract xValue from first column "Current %" and yValue from the item changed
    double xValue = m_httLightOutputFluxTable->item(item->row(), 0)->text().toDouble();
    double yValue = item->text().toDouble();

    QPointF p(xValue, yValue);
    m_fluxSeries->replace(item->row(), p);

    // When all the points have been replaced, then we enable the next button
    bool enableButton = true;
    auto v = m_fluxSeries->pointsVector();
    for (auto it = v.begin(); it != v.end(); it++)
    {
        if (it->y() == 0)
        {
            enableButton = false;
            break;
        }
    }

    if (enableButton)
    {
        QVariantList flux;
        for (auto it = v.begin(); it != v.end(); ++it)
        {
            flux.append(it->y());
        }
        QJsonObject table = m_currentStep["table"].toObject();
        table.insert("flux", QJsonArray::fromVariantList(flux));
        m_currentStep = QJsonObject{{"table", table}};
        m_progressNextButton->setEnabled(true);
    }

    emit doUpdateChart(m_fluxSeries);
}

void CalibrationWizard::frameAvailable(QSharedPointer<HwData> data)
{
    m_frame = data;
    emit doVisualizeFrame(data->image);
}

void CalibrationWizard::lensFittingResultReady(Htt::LensFittingResults res)
{
    // TODO
    if (res.valid)
    {
        emit doVisualizeLensFittingResults(res);
        emit updateStatusBar("Lens fitting successful");

        // Save results in the m_currentState;
        QJsonArray imgProfile;
        QJsonArray imgMaximaPoints;
        QJsonArray imgBevel;
        QJsonArray imgModelPoints;
        QJsonArray imgDistancePoints;
        QJsonArray objProfile;
        QJsonArray objMaximaPoints;
        QJsonArray objBevel;
        QJsonArray objModelPoints;
        QJsonArray objDistancePoints;
        for (auto it = res.imgProfile.begin(); it != res.imgProfile.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            imgProfile.append(val);
        }
        for (auto it = res.imgMaximaPoints.begin(); it != res.imgMaximaPoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            imgMaximaPoints.append(val);
        }
        for (auto it = res.imgBevel.begin(); it != res.imgBevel.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            imgBevel.append(val);
        }
        for (auto it = res.imgModelPoints.begin(); it != res.imgModelPoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            imgModelPoints.append(val);
        }
        for (auto it = res.imgDistancePoints.begin(); it != res.imgDistancePoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            imgDistancePoints.append(val);
        }
        // Obj
        for (auto it = res.objProfile.begin(); it != res.objProfile.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            objProfile.append(val);
        }
        for (auto it = res.objMaximaPoints.begin(); it != res.objMaximaPoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            objMaximaPoints.append(val);
        }
        for (auto it = res.objBevel.begin(); it != res.objBevel.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            objBevel.append(val);
        }
        for (auto it = res.objModelPoints.begin(); it != res.objModelPoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            objModelPoints.append(val);
        }
        for (auto it = res.objDistancePoints.begin(); it != res.objDistancePoints.end(); ++it)
        {
            QJsonObject val{{"x", it->first}, {"y", it->second}};
            objDistancePoints.append(val);
        }

        // Save qImage into jsonObject
        QByteArray arr;
        QBuffer buff(&arr);
        QSharedPointer<QImage> objImage = HttUtils::convertImageToQImage(res.objPlaneImage);
        objImage->save(&buff, "PNG");
        QJsonObject objImg{{"data", QString(arr.toBase64())},
                           {"width", objImage->width()},
                           {"height", objImage->height()},
                           {"size", objImage->byteCount()},
                           {"format", objImage->format()}};
        QJsonObject measures;
        measures.insert("B", res.measures["B"]);
        measures.insert("M", res.measures["M"]);
        measures.insert("D", res.measures["D"]);
        measures.insert("realB", res.realMeasures["B"]);
        measures.insert("realM", res.realMeasures["M"]);
        measures.insert("errorB", res.errors["B"]);
        measures.insert("errorM", res.errors["M"]);

        QString textResult = "<b>Bevel measures:</b><br>";
        textResult += "<br>B: " + QString::number(res.measures["B"], 'f', 3) + " mm (real " +
                      QString::number(res.realMeasures["B"], 'f', 3) +
                      " mm) error: " + QString::number(res.errors["B"]) + " um";
        textResult += "<br>M: " + QString::number(res.measures["M"], 'f', 3) + " mm (real " +
                      QString::number(res.realMeasures["M"], 'f', 3) +
                      " mm) error: " + QString::number(res.errors["M"]) + " um";
        // textResult += "<br>D: " + QString::number(res.measures["D"], 'f', 3) + " mm";
        m_outputResultsTextBrowser->setText(textResult);

        QJsonObject results;
        results.insert("imgProfile", imgProfile);
        results.insert("imgMaximaPoints", imgMaximaPoints);
        results.insert("imgBevel", imgBevel);
        results.insert("imgModelPoints", imgModelPoints);
        results.insert("imgDistancePoints", imgDistancePoints);
        results.insert("objImage", objImg);
        results.insert("objProfile", objProfile);
        results.insert("objMaximaPoints", objMaximaPoints);
        results.insert("objBevel", objBevel);
        results.insert("objModelPoints", objModelPoints);
        results.insert("objDistancePoints", objDistancePoints);
        results.insert("measures", measures);
        results.insert("textResults", textResult);

        m_currentStep["results"] = results;

        // Enable progressNextButton
        m_progressNextButton->setEnabled(true);
    } else
    {
        // FIXME
        emit updateStatusBar("Lens fitting failed");
    }
}

void CalibrationWizard::profileDetectionResultReady(Htt::ProfileDetectionResults res)
{
    if (res.valid)
    {
        emit doVisualizeProfileDetectionResults(res);
        emit updateStatusBar("Profile detection successful");

    } else
    {
        emit updateStatusBar("Profile detection failed");
    }
}

void CalibrationWizard::enableProgressNextButton(bool value)
{
    m_progressNextButton->setEnabled(value);
}

void CalibrationWizard::startGridCalibrationRequired()
{
    // FIXME: backup status.json file and re-calibrate the system
    // if (m_state.contains("step4") && m_state["step4"].toObject().contains("results"))
    // {
    //     m_state["step4"].toObject().remove("results");
    //     m_currentStep.remove("results");
    //     saveJsonState();
    // }
    if (m_currentStep.contains("results"))
    {
        m_currentStep.remove("results");
        m_state["step4"] = m_currentStep;
        saveJsonState();
    }
    m_outputResultsTextBrowser->setText(Notifications::emptyString);

    // Hide the calibration TableResults
    removeCalibrationTableResults();
    m_outputResultsTextBrowser->setVisible(true);

    emit doResetImageViewerController();
    emit doSetViewerModality(Modality::CALIBRATION_4);
    emit doVisualizeFrame(m_gridImage);

    // disable next button
    m_progressNextButton->setEnabled(false);

    emit updateStatusBar("Waiting...");

    emit doStartGridCalibration(m_gridImage);
}

void CalibrationWizard::startAccurateCalibrationRequired(const double& deltaN, const double& H)
{
    if (m_currentStep.contains("results"))
    {
        m_currentStep.remove("results");
        m_state["step9"] = m_currentStep;
        saveJsonState();
    }
    m_outputResultsTextBrowser->setText(Notifications::emptyString);

    // Hide the calibration TableResults
    removeCalibrationTableResults();
    m_outputResultsTextBrowser->setVisible(true);

    emit doResetImageViewerController();
    emit doSetViewerModality(Modality::CALIBRATION_9);
    emit doVisualizeFrame(m_frame->image);

    // disable next button
    m_progressNextButton->setEnabled(false);

    emit updateStatusBar("Waiting...");

    // Accurate calibration with M804 bevel, deltaN=+00.0 and H=+11.0
    emit doStartAccurateCalibration(m_frame->image, deltaN, H);
}

void CalibrationWizard::manualGrabRequired(QString filepath, QString filename)
{
    if (m_frame)
    {
        QDir dir(filepath);
        if (!dir.exists())
            dir.mkpath(".");

        QString date = HttUtils::now();
        m_frame->image->save(filepath + "/" + filename + "_ts=" + date + ".png");

        // if (m_bevelImage)
        //     m_bevelImage->save(filepath + "/" + filename + "_proc_ts=" + date + ".png");
        // if (m_objFrameImage)
        //     m_objFrameImage->save(filepath + "/" + filename + "_obj_ts=" + date + ".png");
        // if (m_objBevelImage)
        //     m_objBevelImage->save(filepath + "/" + filename + "_proc_obj_ts=" + date + ".png");
    }
}

void CalibrationWizard::detectProfileRequired(const double& deltaN, const double& H, QString modelType)
{
    if (m_frame)
    {
        ModelType type = ModelType::Undefined;
        if (modelType == "MiniBevel")
            type = ModelType::MiniBevelModelType;
        else if (modelType == "TBevel")
            type = ModelType::TBevelModelType;
        else
            type = ModelType::CustomBevelModelType;
        emit doDetectProfile(m_frame->image, deltaN, H, type);
    }
}

void CalibrationWizard::computeContrast(int x1, int y1, int x2, int y2)
{
    float deltaX = (float)(x2 - x1);
    float deltaY = (float)(y2 - y1);

    shared_ptr<Image<uint8_t>> img = HttUtils::convertQImageToImage(m_frame->image, QImage::Format_Grayscale8);
    uint8_t* ptr = img->getData();
    uint16_t stride = img->getStride();
    vector<float> pixelVector;

    if (fabs(deltaX) > fabs(deltaY))
    {
        // considero retta orrizzonatale
        float m = deltaY / deltaX;
        float q = (float)y1 - m * (float)x1;

        int start = x1 < x2 ? x1 : x2;
        int stop = x1 < x2 ? x2 : x1;

        for (int x = start; x <= stop; ++x)
        {
            // int y = (int)floorf(m * (float)x + q + 0.5);
            int y = m * (float)x + q;
            float v = (float)ptr[x + y * stride] / 255;
            // cout << "x: " << x << " y: " << y << " pixel: " << v << endl;
            pixelVector.push_back(v);
        }
    } else
    {
        // considero retta verticale
        float m = deltaX / deltaY;
        float q = (float)x1 - m * (float)y1;

        int start = y1 < y2 ? y1 : y2;
        int stop = y1 < y2 ? y2 : y1;

        for (int y = start; y <= stop; ++y)
        {
            // int y = (int)floorf(m * (float)x + q + 0.5);
            int x = m * (float)y + q;
            float v = (float)ptr[x + y * stride] / 255;
            // cout << "x: " << x << " y: " << y << " pixel: " << v << endl;
            pixelVector.push_back(v);
        }
    }

    if (pixelVector.size() > 0)
    {

        vector<int> ipmx;
        vector<int> ipmn;
        PeakFinder::findPeaks(pixelVector, ipmx, true, 1);
        PeakFinder::findPeaks(pixelVector, ipmn, false, -1);

        cout << "ipmx" << endl;
        float sum = 0.0;
        float pmx = 0.0;
        float pmn = 0.0;
        for (auto it = ipmx.begin(); it != ipmx.end(); ++it)
        {
            // FIXME - try to fix the crash!
            if (*it >= pixelVector.size())
                sum += pixelVector[pixelVector.size() - 1];
            else
                sum += pixelVector[*it];
            cout << *it << " -> " << pixelVector[*it] << endl;
        }
        if (ipmx.size() == 0)
            return;
        pmx = sum / (float)ipmx.size();

        cout << "ipmm" << endl;
        sum = 0.0;
        for (auto it = ipmn.begin(); it != ipmn.end(); ++it)
        {
            sum += pixelVector[*it];
            cout << *it << " -> " << pixelVector[*it] << endl;
        }
        if (ipmn.size() == 0)
            return;
        pmn = sum / (float)ipmn.size();
        float c = ((float)(pmx - pmn)) / ((float)(pmx + pmn));
        cout << ">>>>>>>>>>>>>> max: " << pmx << " min: " << pmn << " c: " << c << endl;

        QChart* chart = new QChart();

        QLineSeries* pixelValueSeries = new QLineSeries();
        pixelValueSeries->setPointsVisible();

        for (int i = 0; i < pixelVector.size(); ++i)
        {
            pixelValueSeries->append(i, pixelVector[i]);
        }

        chart->addSeries(pixelValueSeries);
        chart->createDefaultAxes();
        chart->axisY()->setRange(0.0, 1.0);
        chart->legend()->hide();

        if (m_verticalContrastChart && m_horizontalContrastChart)
        {
            removeSlicingResults();
        }

        if (m_verticalContrastChart == nullptr)
        {
            chart->setTitle("Vertical Contrast Slice for G3E4");
            m_verticalContrastChart = new QChartView(chart);
            m_verticalContrastChart->setMaximumSize(600, 600);
            ((QGridLayout*)m_outputGroupBox->layout())->addWidget(m_verticalContrastChart, 0, 0);
            m_verticalConstrastText = new QTextBrowser();
            m_verticalConstrastText->append("Vertical slice contrast is <b>" + QString::number(c) + "</b>");
            ((QGridLayout*)m_outputGroupBox->layout())->addWidget(m_verticalConstrastText, 1, 0);

            QJsonObject vPoints;
            vPoints.insert("x1", x1);
            vPoints.insert("y1", y1);
            vPoints.insert("x2", x2);
            vPoints.insert("y2", y2);
            vPoints.insert("contrast", c);
            m_currentStep.insert("vPoints", vPoints);
        } else if (m_horizontalContrastChart == nullptr)
        {
            chart->setTitle("Horizontal Contrast Slice for G4E2");
            m_horizontalContrastChart = new QChartView(chart);
            m_horizontalContrastChart->setMaximumSize(600, 600);
            ((QGridLayout*)m_outputGroupBox->layout())->addWidget(m_horizontalContrastChart, 0, 1);
            m_horizontalConstrastText = new QTextBrowser();
            m_horizontalConstrastText->append("Horizontal slice contrast is <b>" + QString::number(c) + "</b>");
            ((QGridLayout*)m_outputGroupBox->layout())->addWidget(m_horizontalConstrastText, 1, 1);

            QJsonObject hPoints;
            hPoints.insert("x1", x1);
            hPoints.insert("y1", y1);
            hPoints.insert("x2", x2);
            hPoints.insert("y2", y2);
            hPoints.insert("contrast", c);
            m_currentStep.insert("hPoints", hPoints);

            m_progressNextButton->setEnabled(true);
        }
    }
}

void CalibrationWizard::removeSlicingResults()
{
    if (m_verticalContrastChart)
    {
        m_outputGroupBox->layout()->removeWidget(m_verticalContrastChart);
        delete m_verticalContrastChart;
        m_verticalContrastChart = nullptr;
    }
    if (m_horizontalContrastChart)
    {
        m_outputGroupBox->layout()->removeWidget(m_horizontalContrastChart);
        delete m_horizontalContrastChart;
        m_horizontalContrastChart = nullptr;
    }
    if (m_verticalConstrastText)
    {
        m_outputGroupBox->layout()->removeWidget(m_verticalConstrastText);
        delete m_verticalConstrastText;
        m_verticalConstrastText = nullptr;
    }
    if (m_horizontalConstrastText)
    {
        m_outputGroupBox->layout()->removeWidget(m_horizontalConstrastText);
        delete m_horizontalConstrastText;
        m_horizontalConstrastText = nullptr;
    }

    m_progressNextButton->setEnabled(false);
}

// FIXME: only for test!!!
void CalibrationWizard::pushButtonClickedSimulate(bool)
{
    QFile file(":/wizard/httLightFlux.csv");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "File open error";
        return;
    }

    QVariantList current;
    QVariantList minimum;
    QVariantList nominal;
    QVariantList flux;
    int i = 0;
    file.readLine(); // discard the first line
    while (!file.atEnd())
    {
        auto line = file.readLine().split('\n').first();
        auto list = line.split(',');
        double currValue = list[0].toDouble();
        double minValue = list[1].toDouble();
        double nomValue = list[2].toDouble();
        double delta = nomValue - minValue;
        double fluxValue = minValue + delta * 0.5;
        flux.append(fluxValue);
        m_fluxSeries->replace(i, QPointF(currValue, fluxValue));
        m_httLightOutputFluxTable->setItem(i, 1, new QTableWidgetItem(QString::number(fluxValue)));
        ++i;
    }
}
