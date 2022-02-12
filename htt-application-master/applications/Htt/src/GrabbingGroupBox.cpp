#include "GrabbingGroupBox.h"

#include <QDebug>
#include <QLabel>
#include <QList>

using namespace Nidek::Solution::HTT::UserApplication;

GrabbingGroupBox::GrabbingGroupBox(QWidget* parent /* = Q_NULLPTR*/) : m_grid(new QGridLayout())
{
    this->setLayout(m_grid);
    this->setTitle("GRABBING PARAMETERS");

    m_grid->addWidget(new QLabel("Saving folder"), 0, 0, 1, 6);
    m_grid->addWidget(new QLabel("Session name"), 0, 6, 1, 6);

    m_savingFolderPathPushButton = new QPushButton("Browse");
    m_savingFolderPathLineEdit = new QLineEdit("C:/temp");
    m_sessionNameComboBox = new QComboBox();
    QStringList bevelList = (QStringList() << "M801"
                                           << "M802"
                                           << "M803"
                                           << "M804"
                                           << "M805"
                                           << "M806"
                                           << "M807"
                                           << "M808"
                                           << "M809"
                                           << "M811"
                                           << "M810"
                                           << "M811");
    m_sessionNameComboBox->addItems(bevelList);
    m_grid->addWidget(m_savingFolderPathPushButton, 1, 0, 1, 1);
    m_grid->addWidget(m_savingFolderPathLineEdit, 1, 1, 1, 5);
    m_grid->addWidget(m_sessionNameComboBox, 1, 6, 1, 6);

    m_grid->addWidget(new QLabel("Position"), 2, 0, 1, 2);
    m_grid->addWidget(new QLabel("H"), 2, 2, 1, 2);
    m_grid->addWidget(new QLabel("deltaN"), 2, 4, 1, 2);
    m_grid->addWidget(new QLabel("Note"), 2, 6, 1, 6);

    m_positionLineEdit = new QLineEdit("R000");
    QRegExp rx("[RL]\\d\\d\\d");
    m_positionLineEdit->setValidator(new QRegExpValidator(rx, this));
    // m_deltaNSpinBox = new QDoubleSpinBox();
    // m_deltaNSpinBox->setRange(-99.0, 99.0);
    // m_deltaNSpinBox->setDecimals(1);
    m_deltaNComboBox = new QComboBox();
    m_deltaNComboBox->addItem("-12.0", -12.0);
    m_deltaNComboBox->addItem("-09.0", -9.0);
    m_deltaNComboBox->addItem("-06.0", -6.0);
    m_deltaNComboBox->addItem("-03.0", -6.0);
    m_deltaNComboBox->addItem("+00.0", 0.0);
    m_deltaNComboBox->addItem("+03.0", 3.0);
    m_deltaNComboBox->addItem("+06.0", 6.0);
    m_deltaNComboBox->addItem("+09.0", 9.0);
    m_deltaNComboBox->addItem("+12.0", 12.0);
    m_deltaNComboBox->setCurrentIndex(4);

    m_hSpinBox = new QDoubleSpinBox();
    m_hSpinBox->setRange(-20.0, 20.0);
    m_hSpinBox->setDecimals(1);
    m_noteLineEdit = new QLineEdit();

    m_grid->addWidget(m_positionLineEdit, 3, 0, 1, 2);
    m_grid->addWidget(m_hSpinBox, 3, 2, 1, 2);
    // m_grid->addWidget(m_deltaNSpinBox, 3, 4, 1, 2);
    m_grid->addWidget(m_deltaNComboBox, 3, 4, 1, 2);
    m_grid->addWidget(m_noteLineEdit, 3, 6, 1, 6);
}

GrabbingGroupBox::~GrabbingGroupBox()
{
}

void GrabbingGroupBox::manualGrabRequired(int gain, int exposureTime, int ledPower)
{
    // <session_name>_pos=[L/R]XXX_H=[+/-]XX.X_deltaN=[+/-]XX.X_LED=xxx_Exp=xxx_G=xxx_(note=xxx_)ts=AAAAMMDD-hhmmss-xxx.png
    QString filename;
    // if (!m_sessionNameLineEdit->text().isEmpty() || (m_sessionNameLineEdit->text().simplified().remove(' ') != ""))
    // filename.append(m_sessionNameLineEdit->text().simplified().remove(' ') + "_");
    filename.append(m_sessionNameComboBox->currentText() + "_");
    filename.append("pos=" + m_positionLineEdit->text());
    filename.append("_H=");
    filename.append(QString().sprintf("%+05.1f", m_hSpinBox->value()));
    filename.append("_deltaN=");
    // filename.append(QString().sprintf("%+05.1f", m_deltaNSpinBox->value()));
    filename.append(m_deltaNComboBox->currentText());
    filename.append("_LED=");
    filename.append(QString().sprintf("%03d", ledPower));
    filename.append("_Exp=");
    filename.append(QString().sprintf("%03d", exposureTime));
    filename.append("_G=");
    filename.append(QString().sprintf("%03d", gain));
    if (!m_noteLineEdit->text().isEmpty() || (m_noteLineEdit->text().simplified().remove(' ') != ""))
        filename.append("_note=" + m_noteLineEdit->text().simplified().remove(' '));

    emit doManualGrabRequired(m_savingFolderPathLineEdit->text(), filename);
}

void GrabbingGroupBox::detectProfile()
{
    QString bevel = m_sessionNameComboBox->currentText();
    QString modelType;
    if (bevel == "M805")
        modelType = "TBevel";
    else if (bevel == "M808" || bevel == "M809")
        modelType = "CustomBevel";
    else // "M801" "M802" "M803" "M804" "M806" "M807" "M810" "M811"
        modelType = "MiniBevel";
    emit doDetectProfile(m_deltaNComboBox->currentData().toDouble(), m_hSpinBox->value(), modelType);
}
