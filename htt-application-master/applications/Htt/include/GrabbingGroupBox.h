#ifndef __GRABBING_GROUP_BOX_H__
#define __GRABBING_GROUP_BOX_H__

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class GrabbingGroupBox : public QGroupBox
{
    Q_OBJECT
public:
    explicit GrabbingGroupBox(QWidget* parent = Q_NULLPTR);
    ~GrabbingGroupBox();

signals:
    void doManualGrabRequired(QString filepath, QString filename);
    void doDetectProfile(const double& deltaN, const double& H, QString modelType);

private slots:
    void manualGrabRequired(int gain, int exposureTime, int ledPower);
    void detectProfile();

private:
    QGridLayout* m_grid;
    QPushButton* m_savingFolderPathPushButton;
    QLineEdit* m_savingFolderPathLineEdit;
    QComboBox* m_sessionNameComboBox;
    QLineEdit* m_positionLineEdit;
    QLineEdit* m_noteLineEdit;
    // QDoubleSpinBox* m_deltaNSpinBox;
    QComboBox* m_deltaNComboBox;
    QDoubleSpinBox* m_hSpinBox;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __GRABBING_GROUP_BOX_H__