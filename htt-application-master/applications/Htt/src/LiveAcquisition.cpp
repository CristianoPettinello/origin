#include "LiveAcquisition.h"
#include "HttUtils.h"
#include "Notifications.h"
#include <QCheckBox>
#include <QDebug>
#include <QFileDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QRegExpValidator>

using namespace Nidek::Solution::HTT::UserApplication;

enum bevelType
{
    MINI_BEVEL,
    CUSTOM_BEVEL,
    T_BEVEL
};

LiveAcquisiton::Stats::Stats() : count(0), err(0){};
void LiveAcquisiton::Stats::reset()
{
    count = 0;
    err = 0;
    min.clear();
    max.clear();
}

LiveAcquisiton::LiveAcquisiton(ControlGroupBox* controlsGroupBox, shared_ptr<HwController> hwController,
                               QObject* parent /* = nullptr*/)
    : m_hwController(hwController),
      m_controlsGroupBox(controlsGroupBox),
      m_grabbingGroupBox(new QGroupBox("GRABBING PARAMETERS")),
      m_processingGroupBox(new QGroupBox("PROCESSING PARAMETERS")),
      m_stats(new Stats()),
      m_bevelMeasureResultsTable(nullptr),
      m_frameImage(nullptr),
      m_bevelImage(nullptr),
      m_objFrameImage(nullptr),
      m_objBevelImage(nullptr)
{
    // Vertical Layout used in the live tab
    QVBoxLayout* liveTabLayout = new QVBoxLayout();
    ((QWidget*)parent)->setLayout(liveTabLayout);

    // m_hwController->cameraInsertImage("miniBevel", ":/images/miniBevel", 14.7, 0.0, ModelType::MiniBevelModelType);
    // m_hwController->cameraInsertImage("customBevel", ":/images/customBevel", 10.0, 0.0,
    //                                   ModelType::CustomBevelModelType);
    // m_hwController->cameraInsertImage("TBevel", ":/images/TBevel", 7.0, 0.0, ModelType::TBevelModelType);

    // Manual Grab
    QGridLayout* grabbingGrid = new QGridLayout();
    m_grabbingGroupBox->setLayout(grabbingGrid);
    liveTabLayout->addWidget(m_grabbingGroupBox);
    QPushButton* savingFolderPathPushButton = new QPushButton("Browse");
    m_savingFolderPathLineEdit = new QLineEdit("C:/temp");
    m_sessionNameLineEdit = new QLineEdit();
    m_positionLineEdit = new QLineEdit("R000");
    QRegExp rx("[RL]\\d\\d\\d");
    m_positionLineEdit->setValidator(new QRegExpValidator(rx, this));
    m_deltaNSpinBox = new QDoubleSpinBox();
    m_deltaNSpinBox->setRange(-99.0, 99.0);
    m_deltaNSpinBox->setDecimals(1);
    m_hSpinBox = new QDoubleSpinBox();
    m_hSpinBox->setRange(-99.0, 99.0);
    m_hSpinBox->setDecimals(1);

    m_noteLineEdit = new QLineEdit();

    grabbingGrid->addWidget(new QLabel("Saving folder"), 0, 0, 1, 6);
    grabbingGrid->addWidget(new QLabel("Session name"), 0, 6, 1, 6);
    grabbingGrid->addWidget(savingFolderPathPushButton, 1, 0, 1, 1);
    grabbingGrid->addWidget(m_savingFolderPathLineEdit, 1, 1, 1, 5);
    grabbingGrid->addWidget(m_sessionNameLineEdit, 1, 6, 1, 6);
    grabbingGrid->addWidget(new QLabel("Position"), 2, 0, 1, 2);
    grabbingGrid->addWidget(new QLabel("H"), 2, 2, 1, 2);
    grabbingGrid->addWidget(new QLabel("deltaN"), 2, 4, 1, 2);
    grabbingGrid->addWidget(new QLabel("Note"), 2, 6, 1, 6);
    grabbingGrid->addWidget(m_positionLineEdit, 3, 0, 1, 2);
    grabbingGrid->addWidget(m_hSpinBox, 3, 2, 1, 2);
    grabbingGrid->addWidget(m_deltaNSpinBox, 3, 4, 1, 2);
    grabbingGrid->addWidget(m_noteLineEdit, 3, 6, 1, 6);
    connect(savingFolderPathPushButton, SIGNAL(clicked(bool)), this, SLOT(pushButtonClickedSavingFolderPath(bool)));
    connect(m_hSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxHValueChanged(double)));
    connect(m_deltaNSpinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxDeltaNValueChanged(double)));

    // Processing section
    QGridLayout* processingGrid = new QGridLayout();
    m_processingGroupBox->setLayout(processingGrid);
    liveTabLayout->addWidget(m_processingGroupBox);

    // Optimization maxRetries label and spinBox
    QLabel* optimizationMaxRetriesLabel = new QLabel("maxRetries");
    m_optimizationMaxRetriesSpinBox = new QSpinBox();
    m_optimizationMaxRetriesSpinBox->setMaximum(5);
    QJsonObject root = HttUtils::loadJsonFile("config/settings.json");
    m_optimizationMaxRetriesSpinBox->setValue(root["optimization"].toObject()["maxRetries"].toInt());
    processingGrid->addWidget(optimizationMaxRetriesLabel);
    processingGrid->addWidget(m_optimizationMaxRetriesSpinBox);
    connect(m_optimizationMaxRetriesSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(optimizationMaxRetriesSpinBloxValueChanged(int)));

    // // FIXME: Initialize H and deltaN
    // double H, deltaN;
    // ModelType modelType;
    // m_hwController->getCameraParams("miniBevel", H, deltaN, modelType);
    // m_hSpinBox->setValue(H);
    // m_deltaNSpinBox->setValue(deltaN);

    // GroupBox used for visualize the output results
    m_resultsGroupBox = new QGroupBox();
    QGridLayout* outputGrid = new QGridLayout;
    m_resultsGroupBox->setLayout(outputGrid);
    m_resultsTextBrowser = new QTextBrowser();
    m_resultsGroupBox->setTitle("RESULTS");
    m_resultsGroupBox->setVisible(true);
    m_resultsGroupBox->setStyleSheet("QTextEdit {border: 0px solid black;}");
    outputGrid->addWidget(m_resultsTextBrowser);
    liveTabLayout->addWidget(m_resultsGroupBox);
}

LiveAcquisiton::~LiveAcquisiton()
{
}

void LiveAcquisiton::start()
{
    reset();

    emit doSetViewerModality(Modality::LIVE);
#if 1
    // Start live acquisition from camera
    m_hwController->startCamera(HwController::GrabbingMode::ContinuousAcquisition);
#else
    // FIXME - simulate live acquisition
    QImage* img =
        new QImage("config/M804_pos=R750_H=+07.0_deltaN=+00.0_LED=040_Exp=033_G=001_ts=06042021_135335_580.png");
    m_frameImage = QSharedPointer<QImage>(img);
    m_deltaNSpinBox->setValue(0.0);
    m_hSpinBox->setValue(7.0);
    emit doVisualizeFrame(m_frameImage);
#endif
}

void LiveAcquisiton::stop()
{
    m_hwController->stopCamera();
}

void LiveAcquisiton::reset()
{
    m_resultsTextBrowser->setText(Notifications::emptyString);
    if (m_bevelMeasureResultsTable)
    {
        m_resultsGroupBox->layout()->removeWidget(m_bevelMeasureResultsTable);
        delete m_bevelMeasureResultsTable;
        m_bevelMeasureResultsTable = nullptr;
    }
    m_stats->reset();
    emit doResetImageViewerController();
    // emit reset frameQueue?
}

int LiveAcquisiton::findRowIndex(QString key)
{
    int i = 0;
    for (; i < m_bevelMeasureResultsTable->rowCount(); ++i)
    {
        if (m_bevelMeasureResultsTable->verticalHeaderItem(i)->text() == key)
            break;
    }
    return i;
}

QString LiveAcquisiton::modelTypeToString(ModelType type)
{
    if (type == MINI_BEVEL)
        return "miniBevel";
    else if (type == CUSTOM_BEVEL)
        return "customBevel";
    else
        return "TBevel";
}

ModelType LiveAcquisiton::indexToModelType(int index)
{
    if (index == MINI_BEVEL)
        return ModelType::MiniBevelModelType;
    else if (index == CUSTOM_BEVEL)
        return ModelType::CustomBevelModelType;
    else
        return ModelType::TBevelModelType;
}

void LiveAcquisiton::lensFittingResultReady(Htt::LensFittingResults res)
{
    m_stats->count++;
    QString textResult = "<b>Bevel measures:</b><br>";

    if (m_bevelMeasureResultsTable == nullptr && res.measures.size() > 0)
    {
        qDebug() << ">>>>>>>>>>>>>>>>>> " << res.measures.size();
        QStringList rowNames;
        QStringList colNames;
        colNames << "current"
                 << "min"
                 << "max";
        for (auto it = res.measures.begin(); it != res.measures.end(); ++it)
        {
            rowNames << QString::fromStdString(it->first);
        }
        m_bevelMeasureResultsTable = new QTableWidget(rowNames.size(), colNames.size(), m_resultsGroupBox);
        m_bevelMeasureResultsTable->setHorizontalHeaderLabels(colNames);
        m_bevelMeasureResultsTable->setVerticalHeaderLabels(rowNames);
        m_resultsGroupBox->layout()->addWidget(m_bevelMeasureResultsTable);
    }

    m_frameImage = HttUtils::convertImageToQImage(res.inputImage);
    emit doVisualizeFrame(m_frameImage);

    if (res.valid)
    {
        m_bevelImage = HttUtils::convertImageToQImage(res.bevelImage);
        m_objFrameImage = HttUtils::convertImageToQImage(res.objPlaneImage);
        m_objBevelImage = HttUtils::convertImageToQImage(res.objBevelImage);
        emit doVisualizeLensFittingResults(res);

        // qDebug() << "TYPE: " << res.modelType;
        // switch (res.modelType)
        // {
        // case ModelType::MiniBevelModelType:
        // case ModelType::TBevelModelType:
        //     textResult += "<br>B: " + QString::number(res.measures["B"], 'f', 3) + " mm";
        //     textResult += "<br>M: " + QString::number(res.measures["M"], 'f', 3) + " mm";
        //     textResult += "<br>D: " + QString::number(res.measures["D"], 'f', 3) + " mm";
        //     break;
        // case ModelType::CustomBevelModelType:
        //     textResult += "<br>D: " + QString::number(res.measures["D"], 'f', 3) + " mm";
        //     textResult += "<br>E: " + QString::number(res.measures["E"], 'f', 3) + " mm";
        //     textResult += "<br>F: " + QString::number(res.measures["F"], 'f', 3) + " mm";
        //     textResult += "<br>G: " + QString::number(res.measures["G"], 'f', 3) + " mm";
        //     break;

        // default:
        //     break;
        // }

        for (auto it = res.measures.begin(); it != res.measures.end(); ++it)
        {
            string key = it->first;
            float val = it->second;

            if (m_stats->min.find(key) == m_stats->min.end())
            {
                m_stats->min.insert(*it);
                m_stats->max.insert(*it);
            } else
            {
                if (val < m_stats->min[key])
                    m_stats->min[key] = val;
                if (val > m_stats->max[key])
                    m_stats->max[key] = val;
            }

            int index = findRowIndex(QString::fromStdString(key));
            m_bevelMeasureResultsTable->setItem(index, 0, new QTableWidgetItem(QString::number(it->second, 'f', 3)));
            m_bevelMeasureResultsTable->setItem(index, 1,
                                                new QTableWidgetItem(QString::number(m_stats->min[key], 'f', 3)));
            m_bevelMeasureResultsTable->setItem(index, 2,
                                                new QTableWidgetItem(QString::number(m_stats->max[key], 'f', 3)));
        }

        // m_resultsTextBrowser->setText(textResult);
        emit updateStatusBar("Lens fitting successful");
    } else
    {
        m_bevelImage = nullptr;
        m_objFrameImage = nullptr;
        m_objBevelImage = nullptr;

        m_stats->err++;
        emit updateStatusBar("Lens fitting failed");
    }

    // // QString textResult = m_resultsTextBrowser->toHtml();
    textResult += "<br><br>Errors: " + QString::number(m_stats->err) + "/" + QString::number(m_stats->count);
    // for (auto it = m_stats->min.begin(); it != m_stats->min.end(); ++it)
    // {
    //     QString key = QString::fromStdString(it->first);
    //     double minVal = (double)it->second;
    //     double maxVal = (double)m_stats->max[key.toStdString()];
    //     textResult += "<br>min" + key + ": " + QString::number(minVal, 'f', 3);
    //     textResult += "     max" + key + ": " + QString::number(maxVal, 'f', 3);
    // }
    m_resultsTextBrowser->setText(textResult);
}

void LiveAcquisiton::optimizationMaxRetriesSpinBloxValueChanged(int value)
{
    QJsonObject root = HttUtils::loadJsonFile("config/settings.json");
    QJsonObject optimization = root["optimization"].toObject();
    optimization["maxRetries"] = value;
    root["optimization"] = optimization;
    HttUtils::saveJsonFile(root, "config/settings.json");
}

void LiveAcquisiton::pushButtonClickedSavingFolderPath(bool)
{
    QString path = QFileDialog::getExistingDirectory(NULL, tr("Open Directory"), QDir::homePath(),
                                                     QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!path.isEmpty() && !path.isNull())
        m_savingFolderPathLineEdit->setText(path);
}

void LiveAcquisiton::spinBoxHValueChanged(double H)
{
    // FIXME: how to inform the library?
    // QString name = modelTypeToString((ModelType)m_bevelsComboBox->currentIndex());
    // m_hwController->setCameraParams(name, H, m_deltaNSpinBox->value());
}

void LiveAcquisiton::spinBoxDeltaNValueChanged(double deltaN)
{
    // FIXME: how to inform the library?
    // QString name = modelTypeToString((ModelType)m_bevelsComboBox->currentIndex());
    // m_hwController->setCameraParams(name, m_hSpinBox->value(), deltaN);
}

void LiveAcquisiton::saveFrame(QSharedPointer<HwData> data)
{
    m_frameImage = data->image;
    emit doVisualizeFrame(m_frameImage);
}

void LiveAcquisiton::manualGrabRequired(int gain, int exposureTime, int ledPower)
{
    if (m_frameImage)
    {
        // i.e. Frame07_pos=L110_H=+07.0_deltaN=+00.0_LED=xxx_Exp=xxx_G=xxx.png
        QString filename;
        if (!m_sessionNameLineEdit->text().isEmpty() || (m_sessionNameLineEdit->text().simplified().remove(' ') != ""))
            filename.append(m_sessionNameLineEdit->text().simplified().remove(' ') + "_");
        filename.append("pos=" + m_positionLineEdit->text());
        filename.append("_H=");
        filename.append(QString().sprintf("%+05.1f", m_hSpinBox->value()));
        filename.append("_deltaN=");
        filename.append(QString().sprintf("%+05.1f", m_deltaNSpinBox->value()));
        filename.append("_LED=");
        filename.append(QString().sprintf("%03d", ledPower)); // FIXME
        filename.append("_Exp=");
        filename.append(QString().sprintf("%03d", exposureTime)); // FIXME
        filename.append("_G=");
        filename.append(QString().sprintf("%03d", gain)); // FIXME
        if (!m_noteLineEdit->text().isEmpty() || (m_noteLineEdit->text().simplified().remove(' ') != ""))
            filename.append("_note=" + m_noteLineEdit->text().simplified().remove(' '));

        // Save image
        qDebug() << m_savingFolderPathLineEdit->text();
        qDebug() << filename;

        // To both check if it exists and create if it doesn't
        QString filepath = m_savingFolderPathLineEdit->text();
        QDir dir(filepath);
        if (!dir.exists())
            dir.mkpath(".");

        QString date = HttUtils::now();
        m_frameImage->save(filepath + "/" + filename + "_ts=" + date + ".png");

        if (m_bevelImage)
            m_bevelImage->save(filepath + "/" + filename + "_proc_ts=" + date + ".png");
        if (m_objFrameImage)
            m_objFrameImage->save(filepath + "/" + filename + "_obj_ts=" + date + ".png");
        if (m_objBevelImage)
            m_objBevelImage->save(filepath + "/" + filename + "_proc_obj_ts=" + date + ".png");
    }
}

void LiveAcquisiton::manualLensFittingRequired(int index)
{
    if (m_frameImage)
    {
        emit doStartLensFitting(m_frameImage, m_deltaNSpinBox->value(), m_hSpinBox->value(), indexToModelType(index));
    }
}
