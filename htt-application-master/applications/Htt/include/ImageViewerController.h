#ifndef __IMAGE_VIEWER_CONTROLLER_H__
#define __IMAGE_VIEWER_CONTROLLER_H__

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStateMachine>
#include <QTabWidget>
#include <QTextEdit>

#include "Htt.h"
#include "Image.h"
#include "ImageViewer.h"
#include "ModalityWork.h"
#include <QChart>
#include <QtCharts>
#include <memory>

using namespace std;
using namespace Nidek::Libraries::HTT;

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class ImageViewController : public QObject
{
    Q_OBJECT

public:
    explicit ImageViewController(ImageViewer* viewer, QObject* parent = nullptr);
    ~ImageViewController();
    Modality getModality();

private:
    void resetAllCheckBoxs();
    void initMeasuresStateMachine();
    void saveFirstPoint();
    void saveSecondPointAndComputeMeasure();
    void resetStateMachine();
    void createCentralCross(int width, int height);
    void createReferenceSquare(float width, float height);
    void initSlicingStateMachine();
    void slicingResetStateMachine();
    void slicingSaveFirstPoint();
    void slicingSaveSecondPoint();

signals:
    void doMeasureOnObjectPlane(double x1, double y1, double x2, double y2);
    void doGetMinimumImagingFoV(int x, int y, double width, double height);
    void updateStatusBar(QString str);
    void screenshotReady(QSharedPointer<QPixmap> img);
    void frameReady(QSharedPointer<QPixmap> img);
    void doComputeContrast(int x1, int y1, int x2, int y2);
    void doRemoveSicingResults();

public slots:
    void reset();
    void mousePressEvent(QPointF point);
    void measureOfDistanceReady(double result);
    void visualizeFrame(QSharedPointer<QImage> image);
    void visualizeCalibrationResults(vector<pair<pair<int, int>, pair<int, int>>> imgLines,
                                     vector<pair<int, int>> imgPoints, QSharedPointer<QImage> objPlaneGrid,
                                     vector<pair<int, int>> objPoints);
    void visualizeImagingFoV(QSharedPointer<QImage> objPlaneGrid, vector<pair<int, int>> objPoints);
    void visualizeLensFittingResults(Htt::LensFittingResults res);
    void visualizeProfileDetectionResults(Htt::ProfileDetectionResults res);
    void visualizeSlicingContrastLines(QPointF p1, QPointF p2, QPointF p3, QPointF p4);
    void minimumImagingFoVReady(vector<pair<int, int>> points);
    void referenceSquareReady(vector<pair<int, int>> points);
    void referenceSquareForSlitReady(vector<pair<int, int>> points);
    void visualizeChart(QChart* chart);
    void updateChart(QLineSeries* series);
    // Save a screenshot of the scene and emit an event screenshotReady()
    void takeScreenshot();
    void setModality(Modality m);

private slots:
    void tabWidgetPlaneCurrentChanged(int index);
    void checkBoxBackgroundStateChanged(int state);
    void checkBoxCrossStateChanged(int state);
    void checkBoxReferenceSquareStateChanged(int state);
    void checkBoxReferenceSquareForSlitStateChanged(int state);
    void checkBoxGridLinesStateChanged(int state);
    void checkBoxGridPointsStateChanged(int state);
    void checkBoxSlicingStateChanged(int state);
    void checkBoxObjBackgroundStateChanged(int state);
    void checkBoxObjGridPointsStateChanged(int state);
    void checkBoxObjMeasuresStateChanged(int state);
    void checkBoxObjFoVSquareChanged(int state);
    void checkBoxProfileChanged(int state);
    void checkBoxMaximaPointsChanged(int state);
    void checkBoxBevelChanged(int state);
    void checkBoxModelPointsChanged(int state);
    void checkBoxDistancePointsChanged(int state);
    void checkBoxObjProfileChanged(int state);
    void checkBoxObjMaximaPointsChanged(int state);
    void checkBoxObjBevelChanged(int state);
    void checkBoxObjModelPointsChanged(int state);
    void checkBoxObjDistancePointsChanged(int state);

protected:
private:
    ImageViewer* m_viewer;
    Modality m_modality;

    QTabWidget* m_layersTabWidget;
    QWidget* m_imgPlaneTab;
    QWidget* m_objPlaneTab;
    QCheckBox* m_checkBoxBackground;
    QCheckBox* m_checkBoxCentralCross;
    QCheckBox* m_checkBoxReferenceSquare;
    QCheckBox* m_checkBoxReferenceSquareForSlit;
    QCheckBox* m_checkBoxGridLines;
    QCheckBox* m_checkBoxGridPoints;
    QCheckBox* m_checkBoxSlicing;
    QCheckBox* m_checkBoxObjBackground;
    QCheckBox* m_checkBoxObjGridPoints;
    QCheckBox* m_checkBoxObjMeasures;
    QCheckBox* m_checkBoxObjFoVSquare;
    QCheckBox* m_checkBoxProfile;
    QCheckBox* m_checkBoxMaximaPoints;
    QCheckBox* m_checkBoxBevel;
    QCheckBox* m_checkBoxModelPoints;
    QCheckBox* m_checkBoxDistancePoints;
    QCheckBox* m_checkBoxObjProfile;
    QCheckBox* m_checkBoxObjMaximaPoints;
    QCheckBox* m_checkBoxObjBevel;
    QCheckBox* m_checkBoxObjModelPoints;
    QCheckBox* m_checkBoxObjDistancePoints;

    QTextEdit* m_textEditMeasures;
    QStateMachine* m_measuresStateMachine;
    QStateMachine* m_slicingStateMachine;
    QPointF p1;
    QPointF p2;
    double dPixel;
    double dMm;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __IMAGE_VIEWER_CONTROLLER_H__
