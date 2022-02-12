#ifndef __LIVE_ACQUISITION_H__
#define __LIVE_ACQUISITION_H__

#include "Htt.h"
// #include "HwController.h"
#include "ControlGroupBox.h"
#include "ModalityWork.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <QPushButton>
#include <QQueue>
#include <QTableWidget>
#include <QTextBrowser>

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class LiveAcquisiton : public QObject
{
    Q_OBJECT

public:
    struct Stats
    {
        Stats();
        void reset();
        int count;
        int err;
        map<string, float> min;
        map<string, float> max;
    };

public:
    explicit LiveAcquisiton(ControlGroupBox* controlsGroupBox, shared_ptr<HwController> hwController,
                            QObject* parent = nullptr);
    ~LiveAcquisiton();
    void start();
    void stop();
    void reset();
    void clean();

private:
    int findRowIndex(QString key);
    QString modelTypeToString(ModelType type);
    ModelType indexToModelType(int index);

signals:
    void updateStatusBar(QString str);
    void doEnqueueFrame(QSharedPointer<QImage> image);
    void doResetImageViewerController();
    void doSetViewerModality(Modality mod);
    void doVisualizeFrame(QSharedPointer<QImage> image);
    void doVisualizeLensFittingResults(Htt::LensFittingResults res);
    // void doChangeProcessingRealTimeState(bool enabled);
    void doStartLensFitting(QSharedPointer<QImage> image, double deltaN, double H, ModelType modelType);

private slots:
    void lensFittingResultReady(Htt::LensFittingResults res);
    void optimizationMaxRetriesSpinBloxValueChanged(int value);
    void pushButtonClickedSavingFolderPath(bool);
    void spinBoxHValueChanged(double);
    void spinBoxDeltaNValueChanged(double);
    void saveFrame(QSharedPointer<HwData> data);
    void manualGrabRequired(int gain, int exposureTime, int ledPower);
    void manualLensFittingRequired(int index);

private:
    shared_ptr<HwController> m_hwController;

    // Controls sections
    ControlGroupBox* m_controlsGroupBox;

    // Live tab section
    QGroupBox* m_grabbingGroupBox;
    QGroupBox* m_processingGroupBox;
    QGroupBox* m_resultsGroupBox;
    QTextBrowser* m_resultsTextBrowser;
    Stats* m_stats;
    QTableWidget* m_bevelMeasureResultsTable;
    QSpinBox* m_optimizationMaxRetriesSpinBox;
    QLineEdit* m_savingFolderPathLineEdit;
    QLineEdit* m_sessionNameLineEdit;
    QLineEdit* m_positionLineEdit;
    QLineEdit* m_noteLineEdit;
    QDoubleSpinBox* m_deltaNSpinBox;
    QDoubleSpinBox* m_hSpinBox;
    QSharedPointer<QImage> m_frameImage;
    QSharedPointer<QImage> m_bevelImage;
    QSharedPointer<QImage> m_objFrameImage;
    QSharedPointer<QImage> m_objBevelImage;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __LIVE_ACQUISITION_H__