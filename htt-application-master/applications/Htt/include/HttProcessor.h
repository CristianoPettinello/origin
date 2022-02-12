#ifndef __HTT_PROCESSOR_H__
#define __HTT_PROCESSOR_H__

#include <QObject>

#include "Htt.h"

using namespace Nidek::Libraries::HTT;

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class HttProcessor : public QObject
{
    Q_OBJECT
public:
    explicit HttProcessor(QObject* parent = nullptr);

public slots:
    void calibrateGrid(QSharedPointer<QImage> grid);
    void calibrateBevel(QSharedPointer<QImage> frame, const double& deltaN, const double& H);
    void computeMeasureOfDistance(double x1, double y1, double x2, double y2);
    void validateGridCalibration(vector<float> gridErrors);
    void validateAccurateCalibration(Htt::LensFittingResults result);
    void getMinimumImagingFoV(int x, int y, double width, double height);
    void getReferenceSquare(const double& width, const double& height);
    void getReferenceSquareForSlit(const double& width, const double& height);
    void lensFitting(QSharedPointer<QImage> frame, const double& deltaN, const double& H, ModelType modelType);
    void detectProfile(QSharedPointer<QImage> frame, const double& deltaN, const double& H, ModelType modelType);

signals:
    void calibrationGridResultsReady(Htt::CalibrationGridResults result);
    void accurateCalibrationResultsReady(Htt::AccurateCalibrationResults result);
    void measureOfDistanceReady(double result);
    void calibrationGridValidationReady(QString result, map<string, bool> validation, vector<float> gridErrors);
    void accurateCalibrationValidationReady(QString result, map<string, bool> validation,
                                            Htt::LensFittingResults lensFittingResult);
    void minimumImagingFoVReady(vector<pair<int, int>> points);
    void referenceSquareReady(vector<pair<int, int>> points);
    void referenceSquareForSlitReady(vector<pair<int, int>> points);
    void lensFittingResultReady(Htt::LensFittingResults result);
    void profileDetectionResultReady(Htt::ProfileDetectionResults result);
    void processingFinished();

private:
    shared_ptr<Htt> m_htt;
    shared_ptr<Log> m_log;
    shared_ptr<Settings> m_settings;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __HTT_PROCESSOR_H__
