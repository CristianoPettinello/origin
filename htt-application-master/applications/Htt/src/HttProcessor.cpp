#include "HttProcessor.h"
#include "HttUtils.h"

#include <QDebug>
#include <QDir>

const QString defaultSettingsFile = "config/settings.json";
const QString defaultCalibrationFile = "config/calibration.json";
const QString defaultLogProperties = "config/log4cpp.properties";

using namespace Nidek::Solution::HTT::UserApplication;

HttProcessor::HttProcessor(QObject* /*parent = nullptr*/)
{
    QDir dir(".");

    m_log = Log::getInstance();
    m_log->setProprieties(dir.absoluteFilePath(defaultLogProperties).toStdString());

    m_settings = Settings::getInstance();
    m_settings->loadSettings(dir.absoluteFilePath(defaultSettingsFile).toStdString());
    m_settings->loadCalibration(dir.absoluteFilePath(defaultCalibrationFile).toStdString());

    m_htt = shared_ptr<Htt>(new Htt());
}

/* SLOTS */
void HttProcessor::calibrateGrid(QSharedPointer<QImage> grid)
{
    Htt::CalibrationGridResults result =
        m_htt->calibrateGrid(HttUtils::convertQImageToImage(grid, QImage::Format_Grayscale8));
    m_htt->printReturnCode();

    if (result.valid)
    {
        QDir dir(".");
        m_settings->saveCalibration(dir.absoluteFilePath(defaultCalibrationFile).toStdString());
    }

    emit calibrationGridResultsReady(result);
}

void HttProcessor::calibrateBevel(QSharedPointer<QImage> frame, const double& deltaN, const double& H)
{
    shared_ptr<Image<uint8_t>> inputImage = HttUtils::convertQImageToImage(frame, QImage::Format_Grayscale8);
    Htt::AccurateCalibrationResults result = m_htt->accurateCalibration(inputImage, deltaN, H, "");
    m_htt->printReturnCode();
    if (result.valid)
    {
        QDir dir(".");
        m_settings->saveCalibration(dir.absoluteFilePath(defaultCalibrationFile).toStdString());

        result.lensValidationResults = m_htt->lensValidation(inputImage, deltaN, H, ModelType::MiniBevelModelType);
    }

    emit accurateCalibrationResultsReady(result);
}

void HttProcessor::computeMeasureOfDistance(double x1, double y1, double x2, double y2)
{
    double result = m_htt->measureOnObjectPlane(x1, y1, x2, y2);
    emit measureOfDistanceReady(result);
}

void HttProcessor::validateGridCalibration(vector<float> gridErrors)
{
    QString res = QString::fromStdString(m_htt->getCalibrationParams());
    map<string, bool> m = m_htt->checkCalibratedParams();

    emit calibrationGridValidationReady(res, m, gridErrors);
}

void HttProcessor::validateAccurateCalibration(Htt::LensFittingResults result)
{
    QString res = QString::fromStdString(m_htt->getCalibrationParams()); // FIXME
    map<string, bool> m = m_htt->checkCalibratedParams();
    emit accurateCalibrationValidationReady(res, m, result);
}

void HttProcessor::getMinimumImagingFoV(int x, int y, double width, double height)
{
    vector<pair<int, int>> points = m_htt->getMinimumImagingFoV(x, y, width, height);
    emit minimumImagingFoVReady(points);
}

void HttProcessor::getReferenceSquare(const double& width, const double& height)
{
    vector<pair<int, int>> points = m_htt->getReferenceSquare(width, height);
    emit referenceSquareReady(points);
}

void HttProcessor::getReferenceSquareForSlit(const double& width, const double& height)
{
    vector<pair<int, int>> points = m_htt->getReferenceSquare(width, height);
    emit referenceSquareForSlitReady(points);
}
void HttProcessor::lensFitting(QSharedPointer<QImage> frame, const double& deltaN, const double& H, ModelType modelType)
{
    Htt::LensFittingResults result =
        m_htt->lensFitting(HttUtils::convertQImageToImage(frame, QImage::Format_Grayscale8), deltaN, H, modelType);
    m_htt->printReturnCode();
    emit lensFittingResultReady(result);
    emit processingFinished();
}
void HttProcessor::detectProfile(QSharedPointer<QImage> frame, const double& deltaN, const double& H,
                                 ModelType modelType)
{
    Htt::ProfileDetectionResults result =
        m_htt->profileDetection(HttUtils::convertQImageToImage(frame, QImage::Format_Grayscale8), deltaN, H, modelType);
    emit profileDetectionResultReady(result);
}

/* END SLOTS */
