#include "ImageViewerController.h"
#include "HttUtils.h"
#include "MeasuresStateMachine.h"

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QGraphicsItem>
#include <QLayout>
#include <math.h>
#include <sstream> // std::stringstream

using namespace Nidek::Solution::HTT::UserApplication;
using namespace std;

ImageViewController::ImageViewController(ImageViewer* viewer, QObject* parent)
    : m_viewer(viewer),
      m_checkBoxBackground(new QCheckBox("Background")),
      m_checkBoxCentralCross(new QCheckBox("Cross")),
      m_checkBoxReferenceSquare(new QCheckBox("Reference Square")),
      m_checkBoxReferenceSquareForSlit(new QCheckBox("Reference Square")),
      m_checkBoxGridLines(new QCheckBox("Lines")),
      m_checkBoxGridPoints(new QCheckBox("Points")),
      m_checkBoxSlicing(new QCheckBox("Slicing")),
      m_checkBoxObjBackground(new QCheckBox("Background")),
      m_checkBoxObjGridPoints(new QCheckBox("Points")),
      m_checkBoxObjMeasures(new QCheckBox("Enable Measures")),
      m_checkBoxObjFoVSquare(new QCheckBox("FoV")),
      m_checkBoxProfile(new QCheckBox("Profile")),
      m_checkBoxMaximaPoints(new QCheckBox("Maxima Points")),
      m_checkBoxBevel(new QCheckBox("Bevel")),
      m_checkBoxModelPoints(new QCheckBox("Model Points")),
      m_checkBoxDistancePoints(new QCheckBox("Bottom Shoulder")),
      m_checkBoxObjProfile(new QCheckBox("Profile")),
      m_checkBoxObjMaximaPoints(new QCheckBox("Maxima Points")),
      m_checkBoxObjBevel(new QCheckBox("Bevel")),
      m_checkBoxObjModelPoints(new QCheckBox("Model Points")),
      m_checkBoxObjDistancePoints(new QCheckBox("Bottom Shoulder")),
      m_textEditMeasures(new QTextEdit())
{
    // Tab widget used to visualize the layers in the image plane or in the object plane
    m_layersTabWidget = new QTabWidget();
    ((QVBoxLayout*)parent)->addWidget(m_layersTabWidget);

    m_imgPlaneTab = new QWidget();
    m_objPlaneTab = new QWidget();
    m_layersTabWidget->addTab(m_imgPlaneTab, "Image Plane");
    m_layersTabWidget->addTab(m_objPlaneTab, "Object Plane");

    // Layers layout
    // FIXME: // QHBoxLayout* imagePlaneLayout = new QHBoxLayout();
    QGridLayout* imagePlaneLayout = new QGridLayout();
    imagePlaneLayout->addWidget(m_checkBoxBackground, 0, 0);
    imagePlaneLayout->addWidget(m_checkBoxCentralCross, 0, 1);
    imagePlaneLayout->addWidget(m_checkBoxReferenceSquare, 0, 2);        // FIXME: check position
    imagePlaneLayout->addWidget(m_checkBoxReferenceSquareForSlit, 0, 2); // FIXME: check position
    imagePlaneLayout->addWidget(m_checkBoxSlicing, 0, 3);
    imagePlaneLayout->addWidget(m_checkBoxGridLines, 0, 2);
    imagePlaneLayout->addWidget(m_checkBoxGridPoints, 0, 3);
    imagePlaneLayout->addWidget(m_checkBoxProfile, 1, 0);
    imagePlaneLayout->addWidget(m_checkBoxMaximaPoints, 1, 1);
    imagePlaneLayout->addWidget(m_checkBoxBevel, 1, 2);
    imagePlaneLayout->addWidget(m_checkBoxModelPoints, 1, 3);
    imagePlaneLayout->addWidget(m_checkBoxDistancePoints, 1, 4);
    m_imgPlaneTab->setLayout(imagePlaneLayout);

    QGridLayout* objectPlaneLayout = new QGridLayout();
    objectPlaneLayout->addWidget(m_checkBoxObjBackground, 0, 0);
    objectPlaneLayout->addWidget(m_checkBoxObjGridPoints, 0, 1);
    objectPlaneLayout->addWidget(m_checkBoxObjFoVSquare, 0, 2);
    objectPlaneLayout->addWidget(m_checkBoxObjMeasures, 0, 3);
    objectPlaneLayout->addWidget(m_textEditMeasures, 0, 4);
    objectPlaneLayout->addWidget(m_checkBoxObjProfile, 1, 0);
    objectPlaneLayout->addWidget(m_checkBoxObjMaximaPoints, 1, 1);
    objectPlaneLayout->addWidget(m_checkBoxObjBevel, 1, 2);
    objectPlaneLayout->addWidget(m_checkBoxObjModelPoints, 1, 3);
    objectPlaneLayout->addWidget(m_checkBoxObjDistancePoints, 1, 4);
    m_objPlaneTab->setLayout(objectPlaneLayout);

    QJsonObject root = HttUtils::loadJsonFile("config/settings.json");
    QJsonObject image = root["image"].toObject();
    createCentralCross(image["width"].toInt(), image["height"].toInt());

    // create and start state machine
    initMeasuresStateMachine();
    initSlicingStateMachine();

    connect(m_layersTabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabWidgetPlaneCurrentChanged(int)));
    connect(m_checkBoxBackground, SIGNAL(stateChanged(int)), this, SLOT(checkBoxBackgroundStateChanged(int)));
    connect(m_checkBoxCentralCross, SIGNAL(stateChanged(int)), this, SLOT(checkBoxCrossStateChanged(int)));
    connect(m_checkBoxReferenceSquare, SIGNAL(stateChanged(int)), this, SLOT(checkBoxReferenceSquareStateChanged(int)));
    connect(m_checkBoxReferenceSquareForSlit, SIGNAL(stateChanged(int)), this,
            SLOT(checkBoxReferenceSquareForSlitStateChanged(int)));
    connect(m_checkBoxGridLines, SIGNAL(stateChanged(int)), this, SLOT(checkBoxGridLinesStateChanged(int)));
    connect(m_checkBoxGridPoints, SIGNAL(stateChanged(int)), this, SLOT(checkBoxGridPointsStateChanged(int)));
    connect(m_checkBoxSlicing, SIGNAL(stateChanged(int)), this, SLOT(checkBoxSlicingStateChanged(int)));
    connect(m_checkBoxObjBackground, SIGNAL(stateChanged(int)), this, SLOT(checkBoxObjBackgroundStateChanged(int)));
    connect(m_checkBoxObjGridPoints, SIGNAL(stateChanged(int)), this, SLOT(checkBoxObjGridPointsStateChanged(int)));
    connect(m_checkBoxObjMeasures, SIGNAL(stateChanged(int)), this, SLOT(checkBoxObjMeasuresStateChanged(int)));
    connect(m_checkBoxObjFoVSquare, SIGNAL(stateChanged(int)), this, SLOT(checkBoxObjFoVSquareChanged(int)));
    connect(m_checkBoxProfile, SIGNAL(stateChanged(int)), this, SLOT(checkBoxProfileChanged(int)));
    connect(m_checkBoxMaximaPoints, SIGNAL(stateChanged(int)), this, SLOT(checkBoxMaximaPointsChanged(int)));
    connect(m_checkBoxBevel, SIGNAL(stateChanged(int)), this, SLOT(checkBoxBevelChanged(int)));
    connect(m_checkBoxModelPoints, SIGNAL(stateChanged(int)), this, SLOT(checkBoxModelPointsChanged(int)));
    connect(m_checkBoxDistancePoints, SIGNAL(stateChanged(int)), this, SLOT(checkBoxDistancePointsChanged(int)));
    connect(m_checkBoxObjProfile, SIGNAL(stateChanged(int)), this, SLOT(checkBoxObjProfileChanged(int)));
    connect(m_checkBoxObjMaximaPoints, SIGNAL(stateChanged(int)), this, SLOT(checkBoxObjMaximaPointsChanged(int)));
    connect(m_checkBoxObjBevel, SIGNAL(stateChanged(int)), this, SLOT(checkBoxObjBevelChanged(int)));
    connect(m_checkBoxObjModelPoints, SIGNAL(stateChanged(int)), this, SLOT(checkBoxObjModelPointsChanged(int)));
    connect(m_checkBoxObjDistancePoints, SIGNAL(stateChanged(int)), this, SLOT(checkBoxObjDistancePointsChanged(int)));
    connect(m_viewer, SIGNAL(mousePressEvent(QPointF)), this, SLOT(mousePressEvent(QPointF)));
}

ImageViewController::~ImageViewController()
{
}

Modality ImageViewController::getModality()
{
    return m_modality;
}

void ImageViewController::reset()
{
    qDebug() << "RESET";

    // Hides all layers (ImagePlane, ObjectPlane and Chart)
    m_viewer->imagePlaneLayer()->setVisible(false);
    m_viewer->objectPlaneLayer()->setVisible(false);
    m_viewer->chartLayer()->setVisible(false);

    // Disable both plane tabs and hide layerTabwidget
    m_layersTabWidget->setTabEnabled(m_layersTabWidget->indexOf(m_imgPlaneTab), false);
    m_layersTabWidget->setTabEnabled(m_layersTabWidget->indexOf(m_objPlaneTab), false);
    m_layersTabWidget->setVisible(false);

    // Removes the tick from each checkBox and hides them
    resetAllCheckBoxs();

    // Clean and hide textEditMeasures
    m_textEditMeasures->setText("");
    m_textEditMeasures->setVisible(false);

    // Reset IMAGE layer
    m_viewer->resetZoom();
    // FIXME
    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::IMAGE, new QGraphicsPixmapItem());
    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::SLICING, new QGraphicsItemGroup()); // FIXME
    // m_viewer->imagePlaneLayer()->updateImage(QSharedPointer<QImage>(new QImage()));
}

void ImageViewController::resetAllCheckBoxs()
{
    m_checkBoxBackground->setChecked(false);
    m_checkBoxCentralCross->setChecked(false);
    m_checkBoxReferenceSquare->setChecked(false);
    m_checkBoxReferenceSquareForSlit->setChecked(false);
    m_checkBoxGridLines->setChecked(false);
    m_checkBoxGridPoints->setChecked(false);
    m_checkBoxSlicing->setChecked(false);
    m_checkBoxObjBackground->setChecked(false);
    m_checkBoxObjGridPoints->setChecked(false);
    m_checkBoxObjMeasures->setChecked(false);
    m_checkBoxObjFoVSquare->setChecked(false);
    m_checkBoxProfile->setChecked(false);
    m_checkBoxMaximaPoints->setChecked(false);
    m_checkBoxBevel->setChecked(false);
    m_checkBoxModelPoints->setChecked(false);
    m_checkBoxDistancePoints->setChecked(false);
    m_checkBoxObjProfile->setChecked(false);
    m_checkBoxObjMaximaPoints->setChecked(false);
    m_checkBoxObjBevel->setChecked(false);
    m_checkBoxObjModelPoints->setChecked(false);
    m_checkBoxObjDistancePoints->setChecked(false);

    m_checkBoxBackground->setVisible(false);
    m_checkBoxCentralCross->setVisible(false);
    m_checkBoxReferenceSquare->setVisible(false);
    m_checkBoxReferenceSquareForSlit->setVisible(false);
    m_checkBoxGridLines->setVisible(false);
    m_checkBoxGridPoints->setVisible(false);
    m_checkBoxSlicing->setVisible(false);
    m_checkBoxObjBackground->setVisible(false);
    m_checkBoxObjGridPoints->setVisible(false);
    m_checkBoxObjMeasures->setVisible(false);
    m_checkBoxObjFoVSquare->setVisible(false);
    m_checkBoxProfile->setVisible(false);
    m_checkBoxMaximaPoints->setVisible(false);
    m_checkBoxBevel->setVisible(false);
    m_checkBoxModelPoints->setVisible(false);
    m_checkBoxDistancePoints->setVisible(false);
    m_checkBoxObjProfile->setVisible(false);
    m_checkBoxObjMaximaPoints->setVisible(false);
    m_checkBoxObjBevel->setVisible(false);
    m_checkBoxObjModelPoints->setVisible(false);
    m_checkBoxObjDistancePoints->setVisible(false);

    m_checkBoxBackground->setEnabled(false);
    m_checkBoxCentralCross->setEnabled(false);
    m_checkBoxReferenceSquare->setEnabled(false);
    m_checkBoxReferenceSquareForSlit->setEnabled(false);
    m_checkBoxGridLines->setEnabled(false);
    m_checkBoxGridPoints->setEnabled(false);
    m_checkBoxSlicing->setEnabled(false);
    m_checkBoxObjBackground->setEnabled(false);
    m_checkBoxObjGridPoints->setEnabled(false);
    m_checkBoxObjMeasures->setEnabled(false);
    m_checkBoxObjFoVSquare->setEnabled(false);
    m_checkBoxProfile->setEnabled(false);
    m_checkBoxMaximaPoints->setEnabled(false);
    m_checkBoxBevel->setEnabled(false);
    m_checkBoxModelPoints->setEnabled(false);
    m_checkBoxDistancePoints->setEnabled(false);
    m_checkBoxObjProfile->setEnabled(false);
    m_checkBoxObjMaximaPoints->setEnabled(false);
    m_checkBoxObjBevel->setEnabled(false);
    m_checkBoxObjModelPoints->setEnabled(false);
    m_checkBoxObjDistancePoints->setEnabled(false);
}

void ImageViewController::initMeasuresStateMachine()
{
    m_measuresStateMachine = new QStateMachine();
    MeasuresState* s1 = new MeasuresState();
    MeasuresState* s2 = new MeasuresState();
    MeasuresState* s3 = new MeasuresState();

    s1->assignProperty(this, "text", "Waiting first point");
    s2->assignProperty(this, "text", "Waiting second point");
    s3->assignProperty(this, "text", "Reset");

    MeasuresTransition* t1 = new MeasuresTransition("T");
    t1->setTargetState(s2);
    s1->addTransition(t1);

    MeasuresTransition* t2 = new MeasuresTransition("T");
    t2->setTargetState(s3);
    s2->addTransition(t2);

    MeasuresTransition* t3 = new MeasuresTransition("T");
    t3->setTargetState(s1);
    s3->addTransition(t3);

    m_measuresStateMachine->addState(s1);
    m_measuresStateMachine->addState(s2);
    m_measuresStateMachine->addState(s3);
    m_measuresStateMachine->setInitialState(s1);

    QObject::connect(s1, &QStateMachine::entered, this, &ImageViewController::resetStateMachine);
    QObject::connect(s2, &QStateMachine::entered, this, &ImageViewController::saveFirstPoint);
    QObject::connect(s3, &QStateMachine::entered, this, &ImageViewController::saveSecondPointAndComputeMeasure);
}

void ImageViewController::saveFirstPoint()
{
    m_textEditMeasures->setText(QString());
    p1 = ((MeasuresState*)(m_measuresStateMachine->configuration().toList()[0]))->point;
    qDebug() << "saveFirstPoint: " << p1;

    QGraphicsRectItem* point = new QGraphicsRectItem(p1.x() - 2, p1.y() - 2, 4, 4);
    point->setBrush(Qt::yellow);
    point->setData(0, "measurePoints");
    m_viewer->objectPlaneLayer()->addItemToGroup(ObjectPlaneLayer::Index::MEASURE_POINTS, point);

    std::stringstream ss;
    ss << "P1: (" << p1.x() << "," << p1.y() << ")\t";
    QString s = m_textEditMeasures->toPlainText();
    s.append(ss.str().c_str());
    m_textEditMeasures->setText(s);
}

void ImageViewController::saveSecondPointAndComputeMeasure()
{
    p2 = ((MeasuresState*)(m_measuresStateMachine->configuration().toList()[0]))->point;
    qDebug() << "saveSecondPointAndComputeMeasure: " << p2;

    QGraphicsRectItem* point = new QGraphicsRectItem(p2.x() - 2, p2.y() - 2, 4, 4);
    point->setBrush(Qt::yellow);
    point->setData(0, "measurePoints");
    m_viewer->objectPlaneLayer()->addItemToGroup(ObjectPlaneLayer::Index::MEASURE_POINTS, point);

    QGraphicsLineItem* line = new QGraphicsLineItem(p1.x(), p1.y(), p2.x(), p2.y());
    line->setData(0, "measurePoints");
    line->setPen(QPen(Qt::yellow, 1, Qt::DashLine));
    m_viewer->objectPlaneLayer()->addItemToGroup(ObjectPlaneLayer::Index::MEASURE_POINTS, line);

    std::stringstream ss;
    ss << "P2: (" << p2.x() << "," << p2.y() << ")\n";
    QString s = m_textEditMeasures->toPlainText();
    s.append(ss.str().c_str());
    m_textEditMeasures->setText(s);

    // Compute distance in pixel
    dPixel = sqrtf(pow((p1.x() - p2.x()), 2) + pow((p1.y() - p2.y()), 2));

    // emit signal and wait responce from HttProcessor
    emit doMeasureOnObjectPlane(p1.x(), p1.y(), p2.x(), p2.y());
}

void ImageViewController::resetStateMachine()
{
    qDebug() << "resetStateMachine()";
    qDebug() << m_viewer->objectPlaneLayer()->childItems();

    m_viewer->objectPlaneLayer()->set(ObjectPlaneLayer::Index::MEASURE_POINTS, new QGraphicsItemGroup());
    qDebug() << m_viewer->objectPlaneLayer()->childItems();

    qDebug() << "3";
    p1 = QPointF();
    p2 = QPointF();
    dPixel = 0;
    dMm = 0;
}

void ImageViewController::initSlicingStateMachine()
{
    m_slicingStateMachine = new QStateMachine();
    MeasuresState* s1 = new MeasuresState();
    MeasuresState* s2 = new MeasuresState();
    MeasuresState* s3 = new MeasuresState();
    MeasuresState* s4 = new MeasuresState();
    MeasuresState* s5 = new MeasuresState();

    s1->assignProperty(this, "text", "Waiting first point");
    s2->assignProperty(this, "text", "Waiting second point");
    s3->assignProperty(this, "text", "Waiting third point");
    s4->assignProperty(this, "text", "Waiting fourth point");
    s5->assignProperty(this, "text", "Reset");

    MeasuresTransition* t1 = new MeasuresTransition("T");
    t1->setTargetState(s2);
    s1->addTransition(t1);

    MeasuresTransition* t2 = new MeasuresTransition("T");
    t2->setTargetState(s3);
    s2->addTransition(t2);

    MeasuresTransition* t3 = new MeasuresTransition("T");
    t3->setTargetState(s4);
    s3->addTransition(t3);

    MeasuresTransition* t4 = new MeasuresTransition("T");
    t4->setTargetState(s5);
    s4->addTransition(t4);

    MeasuresTransition* t5 = new MeasuresTransition("T");
    t5->setTargetState(s1);
    s5->addTransition(t5);

    m_slicingStateMachine->addState(s1);
    m_slicingStateMachine->addState(s2);
    m_slicingStateMachine->addState(s3);
    m_slicingStateMachine->addState(s4);
    m_slicingStateMachine->addState(s5);
    m_slicingStateMachine->setInitialState(s1);

    QObject::connect(s1, &QStateMachine::entered, this, &ImageViewController::slicingResetStateMachine);
    QObject::connect(s2, &QStateMachine::entered, this, &ImageViewController::slicingSaveFirstPoint);
    QObject::connect(s3, &QStateMachine::entered, this, &ImageViewController::slicingSaveSecondPoint);
    QObject::connect(s4, &QStateMachine::entered, this, &ImageViewController::slicingSaveFirstPoint);
    QObject::connect(s5, &QStateMachine::entered, this, &ImageViewController::slicingSaveSecondPoint);
    QObject::connect(m_slicingStateMachine, &QStateMachine::stopped, this,
                     &ImageViewController::slicingResetStateMachine);
}

void ImageViewController::slicingSaveFirstPoint()
{
    // m_textEditMeasures->setText(QString());
    p1 = ((MeasuresState*)(m_slicingStateMachine->configuration().toList()[0]))->point;
    qDebug() << "P1: " << p1;

    QGraphicsRectItem* point = new QGraphicsRectItem(p1.x() - 2, p1.y() - 2, 4, 4);
    point->setBrush(Qt::yellow);
    point->setData(0, "slicingPoints");
    m_viewer->imagePlaneLayer()->addItemToGroup(ImagePlaneLayer::Index::SLICING, point);
}

void ImageViewController::slicingSaveSecondPoint()
{
    p2 = ((MeasuresState*)(m_slicingStateMachine->configuration().toList()[0]))->point;
    qDebug() << "P2: " << p2;

    QGraphicsRectItem* point = new QGraphicsRectItem(p2.x() - 2, p2.y() - 2, 4, 4);
    point->setBrush(Qt::yellow);
    point->setData(0, "slicingPoints");
    m_viewer->imagePlaneLayer()->addItemToGroup(ImagePlaneLayer::Index::SLICING, point);

    QGraphicsLineItem* line = new QGraphicsLineItem(p1.x(), p1.y(), p2.x(), p2.y());
    line->setData(0, "slicingPoints");
    line->setPen(QPen(Qt::yellow, 1, Qt::DashLine));
    m_viewer->imagePlaneLayer()->addItemToGroup(ImagePlaneLayer::Index::SLICING, line);

    emit doComputeContrast(p1.x(), p1.y(), p2.x(), p2.y());
}

void ImageViewController::slicingResetStateMachine()
{
    qDebug() << "resetStateMachine()";
    // qDebug() << m_viewer->imagePlaneLayer()->childItems();

    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::SLICING, new QGraphicsItemGroup());
    // qDebug() << m_viewer->imagePlaneLayer()->childItems();

    // qDebug() << "3";
    p1 = QPointF();
    p2 = QPointF();
    // dPixel = 0;
    // dMm = 0;
}

void ImageViewController::createCentralCross(int width, int height)
{
    // Create central cross in order to visualize the image center
    QGraphicsItemGroup* group = new QGraphicsItemGroup();
    QGraphicsEllipseItem* center = new QGraphicsEllipseItem(width / 2 - 5, height / 2 - 5, 10, 10);
    // A(-13,26), B(13,12), B(13,-26), D(-13,-12)
    QGraphicsLineItem* l1 =
        new QGraphicsLineItem(width / 2 - 13, height / 2 + 26, width / 2 + 13, height / 2 + 12); //  A-B
    QGraphicsLineItem* l2 =
        new QGraphicsLineItem(width / 2 + 13, height / 2 + 12, width / 2 + 13, height / 2 + -26); // B-C
    QGraphicsLineItem* l3 =
        new QGraphicsLineItem(width / 2 + 13, height / 2 + -26, width / 2 - 13, height / 2 - 12); // C-D
    QGraphicsLineItem* l4 =
        new QGraphicsLineItem(width / 2 - 13, height / 2 - 12, width / 2 - 13, height / 2 + 26); // D-A

    QGraphicsLineItem* vLine = new QGraphicsLineItem(width / 2, height / 2 - 50, width / 2, height / 2 - 5);
    QGraphicsLineItem* vLine2 = new QGraphicsLineItem(width / 2, height / 2 + 5, width / 2, height / 2 + 50);
    QGraphicsLineItem* hLine = new QGraphicsLineItem(width / 2 - 50, height / 2, width / 2 - 5, height / 2);
    QGraphicsLineItem* hLine2 = new QGraphicsLineItem(width / 2 + 5, height / 2, width / 2 + 50, height / 2);
    QPen pen(Qt::white);

    center->setPen(pen);
    l1->setPen(pen);
    l2->setPen(pen);
    l3->setPen(pen);
    l4->setPen(pen);

    vLine->setPen(pen);
    hLine->setPen(pen);
    vLine2->setPen(pen);
    hLine2->setPen(pen);
    group->addToGroup(center);
    group->addToGroup(l1);
    group->addToGroup(l2);
    group->addToGroup(l3);
    group->addToGroup(l4);
    group->addToGroup(vLine);
    group->addToGroup(hLine);
    group->addToGroup(vLine2);
    group->addToGroup(hLine2);

    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::CROSS, group);
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::CROSS, false);
}

/* SLOTS */
void ImageViewController::mousePressEvent(QPointF point)
{
    // Propagate the point to the MeasuresEvent
    if (m_measuresStateMachine->isRunning())
    {
        qDebug() << "m_measuresStateMachine active";
        m_measuresStateMachine->postEvent(new MeasuresEvent("T", point));
    } else if (m_slicingStateMachine->isRunning())
    {
        m_slicingStateMachine->postEvent(new MeasuresEvent("T", point));
    }
}

void ImageViewController::measureOfDistanceReady(double result)
{
    dMm = result;
    stringstream ss;
    ss << "D: " << dPixel << " pixel / " << dMm << " mm";
    QString s = m_textEditMeasures->toPlainText();
    s.append(ss.str().c_str());
    m_textEditMeasures->setText(s);
}

void ImageViewController::visualizeFrame(QSharedPointer<QImage> image)
{
    // Enables image plane tab
    m_layersTabWidget->setTabEnabled(m_layersTabWidget->indexOf(m_imgPlaneTab), true);

    m_viewer->imagePlaneLayer()->updateImage(image);

    // FIXME
    // // Update IMAGE layer
    // m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::IMAGE,
    //                                  new QGraphicsPixmapItem(QPixmap::fromImage(*image.data())));
    if (!m_viewer->zoomStep())
    {
        m_viewer->setSceneRect(m_viewer->imagePlaneLayer()->get(ImagePlaneLayer::Index::IMAGE)->boundingRect());
        m_viewer->fitInView(m_viewer->imagePlaneLayer()->get(ImagePlaneLayer::Index::IMAGE), Qt::KeepAspectRatio);
    }
}

void ImageViewController::visualizeCalibrationResults(vector<pair<pair<int, int>, pair<int, int>>> imgLines,
                                                      vector<pair<int, int>> imgPoints,
                                                      QSharedPointer<QImage> objPlaneGrid,
                                                      vector<pair<int, int>> objPoints)
{
    // GRID LINES LAYER [IMG PLANE]
    QPen pen(Qt::green);
    QGraphicsItemGroup* gridLinesGroup = new QGraphicsItemGroup();
    for (auto it = imgLines.begin(); it != imgLines.end(); ++it)
    {
        QGraphicsLineItem* line =
            new QGraphicsLineItem(it->first.first, it->first.second, it->second.first, it->second.second);
        line->setData(0, "line");
        line->setPen(pen);
        gridLinesGroup->addToGroup(line);
    }
    gridLinesGroup->setData(0, "gridLines");
    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::GRID_LINES, gridLinesGroup);
    // Enables and check checkBox
    m_checkBoxGridLines->setEnabled(true);
    m_checkBoxGridLines->setChecked(true);

    // GRID POINTS LAYER [IMG PLANE]
    QGraphicsItemGroup* gridPointsGroup = new QGraphicsItemGroup();
    for (auto it = imgPoints.begin(); it != imgPoints.end(); ++it)
    {
        QGraphicsEllipseItem* point = new QGraphicsEllipseItem(it->first - 2, it->second - 2, 4, 4);
        point->setBrush(Qt::red);
        point->setData(0, "point");
        gridPointsGroup->addToGroup(point);
    }
    gridPointsGroup->setData(0, "gridPoints");
    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::GRID_POINTS, gridPointsGroup);
    // Enables and check checkBox
    m_checkBoxGridPoints->setEnabled(true);
    m_checkBoxGridPoints->setChecked(true);

    // GRID LAYER [OBJ PLANE]
    m_layersTabWidget->setTabEnabled(m_layersTabWidget->indexOf(m_objPlaneTab), true);
    // FIXME
    m_viewer->objectPlaneLayer()->set(ObjectPlaneLayer::Index::IMAGE,
                                      new QGraphicsPixmapItem(QPixmap::fromImage(*objPlaneGrid.data())));
    // m_viewer->objectPlaneLayer()->updateImage(objPlaneGrid);
    // Enables and check checkBox
    m_checkBoxObjBackground->setEnabled(true);
    m_checkBoxObjBackground->setChecked(true);

    // GRID POINTS LAYER [OBJ PLANE]
    QGraphicsItemGroup* objGridPointsGroup = new QGraphicsItemGroup();
    for (auto it = objPoints.begin(); it != objPoints.end(); ++it)
    {
        QGraphicsEllipseItem* point = new QGraphicsEllipseItem(it->first - 2, it->second - 2, 4, 4);
        point->setBrush(Qt::red);
        point->setData(0, "objPoint");
        objGridPointsGroup->addToGroup(point);
    }
    objGridPointsGroup->setData(0, "objGridPoints");
    m_viewer->objectPlaneLayer()->set(ObjectPlaneLayer::Index::GRID_POINTS, objGridPointsGroup);
    // Enables and check checkBox
    m_checkBoxObjGridPoints->setEnabled(true);
    m_checkBoxObjGridPoints->setChecked(true);
    m_checkBoxObjMeasures->setEnabled(true);

    // FIXME - save the calibration grid with the lines detected and intersection points
    QSharedPointer<QPixmap> img = QSharedPointer<QPixmap>(new QPixmap(m_viewer->grab()));
    QDir dir(".");
    if (dir.mkpath("config/images/"))
    {
        QImage image(img->toImage());
        image.save("config/images/grid_opt_ima.png", "PNG");
    }
}

void ImageViewController::visualizeImagingFoV(QSharedPointer<QImage> objPlaneGrid, vector<pair<int, int>> objPoints)
{
    m_layersTabWidget->setTabEnabled(m_layersTabWidget->indexOf(m_imgPlaneTab), false);
    m_layersTabWidget->setTabEnabled(m_layersTabWidget->indexOf(m_objPlaneTab), true);

    m_checkBoxObjBackground->setEnabled(true);
    m_checkBoxObjBackground->setChecked(true);
    // FIXME
    m_viewer->objectPlaneLayer()->set(ObjectPlaneLayer::Index::IMAGE,
                                      new QGraphicsPixmapItem(QPixmap::fromImage(*objPlaneGrid.data())));
    // m_viewer->objectPlaneLayer()->updateImage(objPlaneGrid);

    QGraphicsItemGroup* objGridPointsGroup = new QGraphicsItemGroup();
    for (auto it = objPoints.begin(); it != objPoints.end(); ++it)
    {
        QGraphicsEllipseItem* point = new QGraphicsEllipseItem(it->first - 2, it->second - 2, 4, 4);
        point->setBrush(Qt::red);
        point->setData(0, "objPoint");
        objGridPointsGroup->addToGroup(point);
    }
    objGridPointsGroup->setData(0, "objGridPoints");
    m_checkBoxObjGridPoints->setEnabled(true);
    m_checkBoxObjGridPoints->setChecked(true);
    m_viewer->objectPlaneLayer()->set(ObjectPlaneLayer::Index::GRID_POINTS, objGridPointsGroup);

    QGraphicsItem* item = m_viewer->objectPlaneLayer()->get(ObjectPlaneLayer::Index::IMAGE);
    m_viewer->setSceneRect(item->boundingRect());
    m_viewer->fitInView(item, Qt::KeepAspectRatio);

    auto center = objPoints.back();
    // FIXME: width and height are fixed to 5.0mm
    emit doGetMinimumImagingFoV(center.first, center.second, 5.0, 5.0);

    // FIXME - save the calibration grid with the lines detected and intersection points
    QSharedPointer<QPixmap> img = QSharedPointer<QPixmap>(new QPixmap(m_viewer->grab()));
    QDir dir(".");
    if (dir.mkpath("config/images/"))
    {
        QImage image(img->toImage());
        image.save("config/images/grid_opt_obj.png", "PNG");
    }
}

void ImageViewController::visualizeLensFittingResults(Htt::LensFittingResults res)
{
    // PROFILE LAYER [IMG PLANE]
    QPen pen(Qt::cyan);
    pen.setWidth(0);
    QGraphicsItemGroup* profileGroup = new QGraphicsItemGroup();
    for (int i = 0; i < (int)res.imgProfile.size() - 1; ++i)
    {

        QGraphicsLineItem* line = new QGraphicsLineItem(res.imgProfile[i].first, res.imgProfile[i].second,
                                                        res.imgProfile[i + 1].first, res.imgProfile[i + 1].second);
        line->setData(0, "lineProfile");
        line->setPen(pen);
        profileGroup->addToGroup(line);
    }
    profileGroup->setData(0, "profile");
    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::PROFILE, profileGroup);
    // Enables and check checkBox
    m_checkBoxProfile->setEnabled(true);
    m_checkBoxProfile->setChecked(true);

    // PROFILE MAXIMA POINTS LAYER [IMG PLANE]
    QGraphicsItemGroup* maximaPointsGroup = new QGraphicsItemGroup();
    for (auto it = res.imgMaximaPoints.begin(); it != res.imgMaximaPoints.end(); ++it)
    {
        QGraphicsEllipseItem* point = new QGraphicsEllipseItem(it->first - 2, it->second - 2, 4, 4);
        point->setBrush(Qt::yellow);
        point->setPen(Qt::NoPen);
        point->setData(0, "maximaPoint");
        maximaPointsGroup->addToGroup(point);
    }
    maximaPointsGroup->setData(0, "maximaPoints");
    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::PROFILE_MAXIMA, maximaPointsGroup);
    // Enables and check checkBox
    m_checkBoxMaximaPoints->setEnabled(true);
    m_checkBoxMaximaPoints->setChecked(true);
    m_checkBoxMaximaPoints->setChecked(false);

    // BEVEL LAYER [IMG PLANE]
    pen.setColor(Qt::red);
    QGraphicsItemGroup* bevelGroup = new QGraphicsItemGroup();
    for (int i = 0; i < (int)res.imgBevel.size() - 1; ++i)
    {

        QGraphicsLineItem* line = new QGraphicsLineItem(res.imgBevel[i].first, res.imgBevel[i].second,
                                                        res.imgBevel[i + 1].first, res.imgBevel[i + 1].second);
        line->setData(0, "lineBevel");
        line->setPen(pen);
        bevelGroup->addToGroup(line);
    }
    bevelGroup->setData(0, "bevel");
    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::BEVEL, bevelGroup);
    // Enables and check checkBox
    m_checkBoxBevel->setEnabled(true);
    m_checkBoxBevel->setChecked(true);

    // MODEL POINTS LAYER [IMG PLANE]
    QGraphicsItemGroup* modelPointsGroup = new QGraphicsItemGroup();
    for (auto it = res.imgModelPoints.begin(); it != res.imgModelPoints.end(); ++it)
    {
        QGraphicsEllipseItem* point = new QGraphicsEllipseItem(it->first - 2, it->second - 2, 4, 4);
        point->setBrush(Qt::green);
        point->setPen(Qt::NoPen);
        point->setData(0, "modelPoint");
        modelPointsGroup->addToGroup(point);
    }
    modelPointsGroup->setData(0, "modelPoints");
    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::MODEL_POINTS, modelPointsGroup);
    // Enables and check checkBox
    m_checkBoxModelPoints->setEnabled(true);
    m_checkBoxModelPoints->setChecked(true);

    // DISTANCE POINTS LAYER [IMG PLANE]
    QGraphicsItemGroup* distancePointsGroup = new QGraphicsItemGroup();
    for (auto it = res.imgDistancePoints.begin(); it != res.imgDistancePoints.end(); ++it)
    {
        QGraphicsEllipseItem* point = new QGraphicsEllipseItem(it->first - 2, it->second - 2, 4, 4);
        point->setBrush(Qt::magenta);
        point->setPen(Qt::NoPen);
        point->setData(0, "dPoint");
        distancePointsGroup->addToGroup(point);
    }
    distancePointsGroup->setData(0, "distancePoints");
    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::DISTANCE_POINTS, distancePointsGroup);
    // Enables and check checkBox
    m_checkBoxDistancePoints->setEnabled(true);
    m_checkBoxDistancePoints->setChecked(true);
    m_checkBoxDistancePoints->setChecked(false);

    // IMAGE LAYER [OBJ PLANE]
    if (res.objPlaneImage)
    {
        m_layersTabWidget->setTabEnabled(m_layersTabWidget->indexOf(m_objPlaneTab), true);
        QSharedPointer<QImage> objImage = HttUtils::convertImageToQImage(res.objPlaneImage);
        m_viewer->objectPlaneLayer()->set(ObjectPlaneLayer::Index::IMAGE,
                                          new QGraphicsPixmapItem(QPixmap::fromImage(*objImage.data())));
        // Enables and check checkBox
        m_checkBoxObjBackground->setEnabled(true);
        m_checkBoxObjBackground->setChecked(true);
        m_checkBoxObjMeasures->setEnabled(true);
    }

    // PROFILE LAYER [OBJ PLANE]
    pen.setColor(Qt::cyan);
    pen.setWidth(0);
    QGraphicsItemGroup* objProfileGroup = new QGraphicsItemGroup();
    for (int i = 0; i < (int)res.objProfile.size() - 1; ++i)
    {

        QGraphicsLineItem* line = new QGraphicsLineItem(res.objProfile[i].first, res.objProfile[i].second,
                                                        res.objProfile[i + 1].first, res.objProfile[i + 1].second);
        line->setData(0, "objLineProfile");
        line->setPen(pen);
        objProfileGroup->addToGroup(line);
    }
    objProfileGroup->setData(0, "objProfile");
    m_viewer->objectPlaneLayer()->set(ObjectPlaneLayer::Index::PROFILE, objProfileGroup);
    // Enables and check checkBox
    m_checkBoxObjProfile->setEnabled(true);
    m_checkBoxObjProfile->setChecked(true);

    // PROFILE MAXIMA POINTS LAYER [OBJ PLANE]
    QGraphicsItemGroup* objMaximaPointsGroup = new QGraphicsItemGroup();
    for (auto it = res.objMaximaPoints.begin(); it != res.objMaximaPoints.end(); ++it)
    {
        QGraphicsEllipseItem* point = new QGraphicsEllipseItem(it->first - 2, it->second - 2, 4, 4);
        point->setBrush(Qt::yellow);
        point->setPen(Qt::NoPen);
        point->setData(0, "objMaximaPoint");
        objMaximaPointsGroup->addToGroup(point);
    }
    objMaximaPointsGroup->setData(0, "objMaximaPoints");
    m_viewer->objectPlaneLayer()->set(ObjectPlaneLayer::Index::PROFILE_MAXIMA, objMaximaPointsGroup);
    // Enables and check checkBox
    m_checkBoxObjMaximaPoints->setEnabled(true);
    m_checkBoxObjMaximaPoints->setChecked(true);
    m_checkBoxObjMaximaPoints->setChecked(false);

    // BEVEL LAYER [OBJ PLANE]
    pen.setColor(Qt::red);
    QGraphicsItemGroup* objBevelGroup = new QGraphicsItemGroup();
    for (int i = 0; i < (int)res.objBevel.size() - 1; ++i)
    {

        QGraphicsLineItem* line = new QGraphicsLineItem(res.objBevel[i].first, res.objBevel[i].second,
                                                        res.objBevel[i + 1].first, res.objBevel[i + 1].second);
        line->setData(0, "objLineBevel");
        line->setPen(pen);
        objBevelGroup->addToGroup(line);
    }
    objBevelGroup->setData(0, "objBevel");
    m_viewer->objectPlaneLayer()->set(ObjectPlaneLayer::Index::BEVEL, objBevelGroup);
    // Enables and check checkBox
    m_checkBoxObjBevel->setEnabled(true);
    m_checkBoxObjBevel->setChecked(true);

    // MODEL POINTS LAYER [OBJ PLANE]
    QGraphicsItemGroup* objModelPointsGroup = new QGraphicsItemGroup();
    for (auto it = res.objModelPoints.begin(); it != res.objModelPoints.end(); ++it)
    {
        QGraphicsEllipseItem* point = new QGraphicsEllipseItem(it->first - 2, it->second - 2, 4, 4);
        point->setBrush(Qt::green);
        point->setPen(Qt::NoPen);
        point->setData(0, "objModelPoint");
        objModelPointsGroup->addToGroup(point);
    }
    objModelPointsGroup->setData(0, "objModelPoints");
    m_viewer->objectPlaneLayer()->set(ObjectPlaneLayer::Index::MODEL_POINTS, objModelPointsGroup);
    // Enables and check checkBox
    m_checkBoxObjModelPoints->setEnabled(true);
    m_checkBoxObjModelPoints->setChecked(true);

    // DISTANCE POINTS LAYER [OBJ PLANE]
    QGraphicsItemGroup* objDistancePointsGroup = new QGraphicsItemGroup();
    for (auto it = res.objDistancePoints.begin(); it != res.objDistancePoints.end(); ++it)
    {
        QGraphicsEllipseItem* point = new QGraphicsEllipseItem(it->first - 2, it->second - 2, 4, 4);
        point->setBrush(Qt::magenta);
        point->setPen(Qt::NoPen);
        point->setData(0, "objDistancePoint");
        objDistancePointsGroup->addToGroup(point);
    }
    objDistancePointsGroup->setData(0, "objDistancePoints");
    m_viewer->objectPlaneLayer()->set(ObjectPlaneLayer::Index::DISTANCE_POINTS, objDistancePointsGroup);
    // Enables and check checkBox
    m_checkBoxObjDistancePoints->setEnabled(true);
    m_checkBoxObjDistancePoints->setChecked(true);
    m_checkBoxObjDistancePoints->setChecked(false);
}

void ImageViewController::visualizeProfileDetectionResults(Htt::ProfileDetectionResults res)
{
    // PROFILE LAYER [IMG PLANE]
    QPen pen(Qt::cyan);
    pen.setWidth(0);
    QGraphicsItemGroup* profileGroup = new QGraphicsItemGroup();
    for (int i = 0; i < (int)res.imgProfile.size() - 1; ++i)
    {

        QGraphicsLineItem* line = new QGraphicsLineItem(res.imgProfile[i].first, res.imgProfile[i].second,
                                                        res.imgProfile[i + 1].first, res.imgProfile[i + 1].second);
        line->setData(0, "lineProfile");
        line->setPen(pen);
        profileGroup->addToGroup(line);
    }
    profileGroup->setData(0, "profile");
    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::PROFILE, profileGroup);
    // Enables and check checkBox
    m_checkBoxProfile->setEnabled(true);
    m_checkBoxProfile->setChecked(true);
}

void ImageViewController::visualizeSlicingContrastLines(QPointF p1, QPointF p2, QPointF p3, QPointF p4)
{
    QGraphicsRectItem* point = new QGraphicsRectItem(p1.x() - 2, p1.y() - 2, 4, 4);
    point->setBrush(Qt::yellow);
    point->setData(0, "slicingPoints");
    m_viewer->imagePlaneLayer()->addItemToGroup(ImagePlaneLayer::Index::SLICING, point);

    point = new QGraphicsRectItem(p2.x() - 2, p2.y() - 2, 4, 4);
    point->setBrush(Qt::yellow);
    point->setData(0, "slicingPoints");
    m_viewer->imagePlaneLayer()->addItemToGroup(ImagePlaneLayer::Index::SLICING, point);

    QGraphicsLineItem* line = new QGraphicsLineItem(p1.x(), p1.y(), p2.x(), p2.y());
    line->setData(0, "slicingPoints");
    line->setPen(QPen(Qt::yellow, 1, Qt::DashLine));
    m_viewer->imagePlaneLayer()->addItemToGroup(ImagePlaneLayer::Index::SLICING, line);

    point = new QGraphicsRectItem(p3.x() - 2, p3.y() - 2, 4, 4);
    point->setBrush(Qt::yellow);
    point->setData(0, "slicingPoints");
    m_viewer->imagePlaneLayer()->addItemToGroup(ImagePlaneLayer::Index::SLICING, point);

    point = new QGraphicsRectItem(p4.x() - 2, p4.y() - 2, 4, 4);
    point->setBrush(Qt::yellow);
    point->setData(0, "slicingPoints");
    m_viewer->imagePlaneLayer()->addItemToGroup(ImagePlaneLayer::Index::SLICING, point);

    line = new QGraphicsLineItem(p3.x(), p3.y(), p4.x(), p4.y());
    line->setData(0, "slicingPoints");
    line->setPen(QPen(Qt::yellow, 1, Qt::DashLine));
    m_viewer->imagePlaneLayer()->addItemToGroup(ImagePlaneLayer::Index::SLICING, line);

    // m_checkBoxSlicing->blockSignals(true);
    // m_checkBoxSlicing->setChecked(true);
    // m_checkBoxSlicing->blockSignals(false);

    // emit doComputeContrast(p1.x(), p1.y(), p2.x(), p2.y());
}

void ImageViewController::minimumImagingFoVReady(vector<pair<int, int>> points)
{
    // New layer with FoV square
    QPen pen(Qt::green, 1, Qt::DashLine);
    QGraphicsItemGroup* fovLinesGroup = new QGraphicsItemGroup();

    QGraphicsLineItem* line1 =
        new QGraphicsLineItem(points[0].first, points[0].second, points[1].first, points[1].second);
    QGraphicsLineItem* line2 =
        new QGraphicsLineItem(points[0].first, points[0].second, points[2].first, points[2].second);
    QGraphicsLineItem* line3 =
        new QGraphicsLineItem(points[3].first, points[3].second, points[1].first, points[1].second);
    QGraphicsLineItem* line4 =
        new QGraphicsLineItem(points[3].first, points[3].second, points[2].first, points[2].second);

    line1->setData(0, "objLineFoV");
    line2->setData(0, "objLineFoV");
    line3->setData(0, "objLineFoV");
    line4->setData(0, "objLineFoV");

    line1->setPen(pen);
    line2->setPen(pen);
    line3->setPen(pen);
    line4->setPen(pen);

    fovLinesGroup->addToGroup(line1);
    fovLinesGroup->addToGroup(line2);
    fovLinesGroup->addToGroup(line3);
    fovLinesGroup->addToGroup(line4);

    fovLinesGroup->setData(0, "objGridFoVLines");
    fovLinesGroup->setVisible(true);
    m_checkBoxObjFoVSquare->setChecked(true);
    m_viewer->objectPlaneLayer()->set(ObjectPlaneLayer::Index::FOV_LINES, fovLinesGroup);
}

void ImageViewController::referenceSquareReady(vector<pair<int, int>> points)
{
    // New layer with reference square
    QPen penGreen(Qt::green, 1, Qt::DashLine);
    QPen penRed(Qt::red, 1, Qt::DashLine);
    QGraphicsItemGroup* referenceSquareLinesGroup = new QGraphicsItemGroup();

    // Nominal grid
    QGraphicsLineItem* line1 =
        new QGraphicsLineItem(points[0].first, points[0].second, points[1].first, points[1].second);
    QGraphicsLineItem* line2 =
        new QGraphicsLineItem(points[0].first, points[0].second, points[2].first, points[2].second);
    QGraphicsLineItem* line3 =
        new QGraphicsLineItem(points[3].first, points[3].second, points[1].first, points[1].second);
    QGraphicsLineItem* line4 =
        new QGraphicsLineItem(points[3].first, points[3].second, points[2].first, points[2].second);

    line1->setData(0, "imgLineRefSquare");
    line2->setData(0, "imgLineRefSquare");
    line3->setData(0, "imgLineRefSquare");
    line4->setData(0, "imgLineRefSquare");
    line1->setPen(penGreen);
    line2->setPen(penGreen);
    line3->setPen(penGreen);
    line4->setPen(penGreen);
    referenceSquareLinesGroup->addToGroup(line1);
    referenceSquareLinesGroup->addToGroup(line2);
    referenceSquareLinesGroup->addToGroup(line3);
    referenceSquareLinesGroup->addToGroup(line4);

    // Acceptance box
    // outer red box: (147,551), (431,425), (156,162), (420,-4)
    QGraphicsLineItem* line5 = new QGraphicsLineItem(147, 551, 431, 425); // A-B
    QGraphicsLineItem* line6 = new QGraphicsLineItem(431, 425, 420, -4);  // B-D
    QGraphicsLineItem* line7 = new QGraphicsLineItem(420, -4, 156, 162);  // D-C
    QGraphicsLineItem* line8 = new QGraphicsLineItem(156, 162, 147, 551); // C-A
    line5->setData(0, "imgLineRefSquare");
    line6->setData(0, "imgLineRefSquare");
    line7->setData(0, "imgLineRefSquare");
    line8->setData(0, "imgLineRefSquare");
    line5->setPen(penRed);
    line6->setPen(penRed);
    line7->setPen(penRed);
    line8->setPen(penRed);
    referenceSquareLinesGroup->addToGroup(line5);
    referenceSquareLinesGroup->addToGroup(line6);
    referenceSquareLinesGroup->addToGroup(line7);
    referenceSquareLinesGroup->addToGroup(line8);

    // inner red box: (163,518), (413,410), (170,173), (404,30)
    QGraphicsLineItem* line9 = new QGraphicsLineItem(163, 518, 413, 410);  // A-B
    QGraphicsLineItem* line10 = new QGraphicsLineItem(413, 410, 404, 30);  // B-D
    QGraphicsLineItem* line11 = new QGraphicsLineItem(404, 30, 170, 173);  // D-C
    QGraphicsLineItem* line12 = new QGraphicsLineItem(170, 173, 163, 518); // C-A
    line9->setData(0, "imgLineRefSquare");
    line10->setData(0, "imgLineRefSquare");
    line11->setData(0, "imgLineRefSquare");
    line12->setData(0, "imgLineRefSquare");
    line9->setPen(penRed);
    line10->setPen(penRed);
    line11->setPen(penRed);
    line12->setPen(penRed);
    referenceSquareLinesGroup->addToGroup(line9);
    referenceSquareLinesGroup->addToGroup(line10);
    referenceSquareLinesGroup->addToGroup(line11);
    referenceSquareLinesGroup->addToGroup(line12);

    referenceSquareLinesGroup->setData(0, "imgReferenceSquareLines");
    referenceSquareLinesGroup->setVisible(true);
    m_checkBoxObjFoVSquare->setChecked(true);
    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::REFERENCE_SQUARE, referenceSquareLinesGroup);
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::REFERENCE_SQUARE, false);
}

void ImageViewController::referenceSquareForSlitReady(vector<pair<int, int>> points)
{
    // New layer with reference square
    QPen pen(Qt::yellow, 1, Qt::DashLine);
    QGraphicsItemGroup* referenceSquareLinesGroup = new QGraphicsItemGroup();

    // Nominal grid
    QGraphicsLineItem* line1 =
        new QGraphicsLineItem(points[0].first, points[0].second, points[1].first, points[1].second);
    QGraphicsLineItem* line2 =
        new QGraphicsLineItem(points[0].first, points[0].second, points[2].first, points[2].second);
    QGraphicsLineItem* line3 =
        new QGraphicsLineItem(points[3].first, points[3].second, points[1].first, points[1].second);
    QGraphicsLineItem* line4 =
        new QGraphicsLineItem(points[3].first, points[3].second, points[2].first, points[2].second);

    line1->setData(0, "imgLineRefSquareForSlit");
    line2->setData(0, "imgLineRefSquareForSlit");
    line3->setData(0, "imgLineRefSquareForSlit");
    line4->setData(0, "imgLineRefSquareForSlit");
    line1->setPen(pen);
    line2->setPen(pen);
    line3->setPen(pen);
    line4->setPen(pen);
    referenceSquareLinesGroup->addToGroup(line1);
    referenceSquareLinesGroup->addToGroup(line2);
    referenceSquareLinesGroup->addToGroup(line3);
    referenceSquareLinesGroup->addToGroup(line4);

    referenceSquareLinesGroup->setData(0, "imgReferenceSquareForSlitLines");
    referenceSquareLinesGroup->setVisible(true);
    m_checkBoxObjFoVSquare->setChecked(true);
    m_viewer->imagePlaneLayer()->set(ImagePlaneLayer::Index::REFERENCE_SQUARE_FOR_SLIT, referenceSquareLinesGroup);
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::REFERENCE_SQUARE_FOR_SLIT, false);
}

void ImageViewController::setModality(Modality mod)
{
    m_modality = mod;
    qDebug() << "ImageViewController::setModality";

    reset();

    switch (m_modality)
    {
    case CALIBRATION_IDLE:
        qDebug() << "MOD: IDLE";
        m_viewer->imagePlaneLayer()->setVisible(false);
        m_viewer->objectPlaneLayer()->setVisible(false);
        m_viewer->chartLayer()->setVisible(false);
        break;
    case CALIBRATION_1:
        qDebug() << "MOD: STEP1";
        break;
    case CALIBRATION_2:
        qDebug() << "MOD: STEP2";
        // Enables TabWidget and set imgPlane tab
        m_layersTabWidget->setVisible(true);
        m_layersTabWidget->setCurrentIndex(m_layersTabWidget->indexOf(m_imgPlaneTab));
        // Set enabled
        m_checkBoxBackground->setEnabled(true);
        m_checkBoxCentralCross->setEnabled(true);
        m_checkBoxReferenceSquare->setEnabled(true);
        m_checkBoxSlicing->setEnabled(true);
        // Set visibile
        m_checkBoxBackground->setVisible(true);
        m_checkBoxCentralCross->setVisible(true);
        m_checkBoxReferenceSquare->setVisible(true);
        m_checkBoxSlicing->setVisible(true);
        // Set checked
        m_checkBoxBackground->setChecked(true);
        m_checkBoxCentralCross->setChecked(true);
        m_checkBoxReferenceSquare->setChecked(true);
        m_checkBoxSlicing->setChecked(false);
        break;
    case CALIBRATION_3:
        qDebug() << "MOD: STEP3";
        // Enables TabWidget and set imgPlane tab
        m_layersTabWidget->setVisible(true);
        m_layersTabWidget->setCurrentIndex(m_layersTabWidget->indexOf(m_imgPlaneTab));
        // Set enabled
        m_checkBoxBackground->setEnabled(true);
        m_checkBoxCentralCross->setEnabled(true);
        m_checkBoxReferenceSquare->setEnabled(true);
        // Set visibile
        m_checkBoxBackground->setVisible(true);
        m_checkBoxCentralCross->setVisible(true);
        m_checkBoxReferenceSquare->setVisible(true);
        // Set checked
        m_checkBoxBackground->setChecked(true);
        m_checkBoxCentralCross->setChecked(true);
        m_checkBoxReferenceSquare->setChecked(true);
        break;
    case CALIBRATION_4:
        qDebug() << "MOD: STEP4";
        // Enables TabWidget and set imgPlane tab
        m_layersTabWidget->setVisible(true);
        m_layersTabWidget->setCurrentIndex(m_layersTabWidget->indexOf(m_imgPlaneTab));
        // Set enabled
        m_checkBoxBackground->setEnabled(true);
        m_checkBoxCentralCross->setEnabled(true);
        // Set visibile
        m_checkBoxBackground->setVisible(true);
        m_checkBoxCentralCross->setVisible(true);
        m_checkBoxGridLines->setVisible(true);
        m_checkBoxGridPoints->setVisible(true);
        m_checkBoxObjBackground->setVisible(true);
        m_checkBoxObjGridPoints->setVisible(true);
        m_checkBoxObjMeasures->setVisible(true);
        // Set checked
        m_checkBoxBackground->setChecked(true);
        break;
    case CALIBRATION_5:
        qDebug() << "MOD: STEP5";
        // Enables TabWidget and set objPlane tab
        m_layersTabWidget->setVisible(true);
        m_layersTabWidget->setCurrentIndex(m_layersTabWidget->indexOf(m_objPlaneTab));
        // Set enabled
        m_checkBoxObjBackground->setEnabled(true);
        m_checkBoxObjGridPoints->setEnabled(true);
        m_checkBoxObjFoVSquare->setEnabled(true);
        // Set visibile
        m_checkBoxObjBackground->setVisible(true);
        m_checkBoxObjGridPoints->setVisible(true);
        m_checkBoxObjFoVSquare->setVisible(true);
        // Set checked
        m_checkBoxObjBackground->setChecked(true);
        m_checkBoxObjGridPoints->setChecked(true);
        m_checkBoxObjFoVSquare->setChecked(true);
        break;
    case CALIBRATION_6:
        qDebug() << "MOD: STEP6";
        // Enables TabWidget and set objPlane tab
        m_layersTabWidget->setVisible(true);
        // Set enabled
        m_checkBoxBackground->setEnabled(true);
        m_checkBoxReferenceSquareForSlit->setEnabled(true);
        // Set visibile
        m_checkBoxBackground->setVisible(true);
        m_checkBoxReferenceSquareForSlit->setVisible(true);
        // Set checked
        m_checkBoxBackground->setChecked(true);
        m_checkBoxReferenceSquareForSlit->setChecked(true);
        break;
    case CALIBRATION_7:
        qDebug() << "MOD: STEP7";
        m_viewer->chartLayer()->setVisible(true);
        break;
    case CALIBRATION_8:
        qDebug() << "MOD: STEP8";
        // Enables TabWidget and set objPlane tab
        m_layersTabWidget->setVisible(true);
        // Set enabled
        m_checkBoxBackground->setEnabled(true);
        // Set visibile
        m_checkBoxBackground->setVisible(true);
        // Set checked
        m_checkBoxBackground->setChecked(true);
        break;
    case CALIBRATION_9:
        qDebug() << "MOD: STEP9";
        // Enables TabWidget and set imgPlane tab
        m_layersTabWidget->setVisible(true);
        m_layersTabWidget->setCurrentIndex(m_layersTabWidget->indexOf(m_imgPlaneTab));
        // Set enabled
        m_checkBoxBackground->setEnabled(true);
        // Set visibile
        m_checkBoxBackground->setVisible(true);
        m_checkBoxProfile->setVisible(true);
        m_checkBoxMaximaPoints->setVisible(true);
        m_checkBoxBevel->setVisible(true);
        m_checkBoxModelPoints->setVisible(true);
        m_checkBoxDistancePoints->setVisible(true);
        m_checkBoxObjBackground->setVisible(true);
        m_checkBoxObjMeasures->setVisible(true);
        m_checkBoxObjProfile->setVisible(true);
        m_checkBoxObjMaximaPoints->setVisible(true);
        m_checkBoxObjBevel->setVisible(true);
        m_checkBoxObjModelPoints->setVisible(true);
        m_checkBoxObjDistancePoints->setVisible(true);
        // Set checked
        m_checkBoxBackground->setChecked(true);
        break;
    case CALIBRATION_10:
        qDebug() << "MOD: STEP10";
        // Enables TabWidget and set imgPlane tab
        m_layersTabWidget->setVisible(true);
        m_layersTabWidget->setCurrentIndex(m_layersTabWidget->indexOf(m_imgPlaneTab));
        // Set enabled
        m_checkBoxBackground->setEnabled(true);
        // Set visibile
        m_checkBoxBackground->setVisible(true);
        m_checkBoxProfile->setVisible(true);
        m_checkBoxMaximaPoints->setVisible(true);
        m_checkBoxBevel->setVisible(true);
        m_checkBoxModelPoints->setVisible(true);
        m_checkBoxDistancePoints->setVisible(true);
        m_checkBoxObjBackground->setVisible(true);
        m_checkBoxObjMeasures->setVisible(true);
        m_checkBoxObjProfile->setVisible(true);
        m_checkBoxObjMaximaPoints->setVisible(true);
        m_checkBoxObjBevel->setVisible(true);
        m_checkBoxObjModelPoints->setVisible(true);
        m_checkBoxObjDistancePoints->setVisible(true);
        // Set checked
        m_checkBoxBackground->setChecked(true);
        break;
    case LIVE:
        qDebug() << "MOD: LIVE";
        // Enables TabWidget and set imgPlane tab
        m_layersTabWidget->setVisible(true);
        m_layersTabWidget->setCurrentIndex(m_layersTabWidget->indexOf(m_imgPlaneTab));
        // Set enabled
        m_checkBoxBackground->setEnabled(true);
        m_checkBoxCentralCross->setEnabled(true);
        // Set visibile
        m_checkBoxBackground->setVisible(true);
        m_checkBoxCentralCross->setVisible(true);
        m_checkBoxProfile->setVisible(true);
        m_checkBoxMaximaPoints->setVisible(true);
        m_checkBoxBevel->setVisible(true);
        m_checkBoxModelPoints->setVisible(true);
        m_checkBoxDistancePoints->setVisible(true);
        m_checkBoxObjBackground->setVisible(true);
        m_checkBoxObjProfile->setVisible(true);
        m_checkBoxObjMeasures->setVisible(true);
        m_checkBoxObjProfile->setVisible(true);
        m_checkBoxObjMaximaPoints->setVisible(true);
        m_checkBoxObjBevel->setVisible(true);
        m_checkBoxObjModelPoints->setVisible(true);
        m_checkBoxObjDistancePoints->setVisible(true);
        // Set checked
        m_checkBoxBackground->setChecked(true);
        break;
    default:
        qDebug() << "*** MODALITY NOT SUPPORTED ***";
        break;
    }
}

void ImageViewController::visualizeChart(QChart* chart)
{
    qDebug() << "visualizeChart";
    chart->setData(0, "chart");
    m_viewer->chartLayer()->setChart(chart);

    m_viewer->setSceneRect(chart->boundingRect());
    m_viewer->fitInView(chart, Qt::KeepAspectRatio);
}

void ImageViewController::updateChart(QLineSeries* series)
{
    m_viewer->chartLayer()->updateSeries(series);
}

void ImageViewController::takeScreenshot()
{
    QSharedPointer<QPixmap> img = QSharedPointer<QPixmap>(new QPixmap(m_viewer->grab()));
    emit screenshotReady(img);

    // Get the image plane background
    QGraphicsPixmapItem* item = (QGraphicsPixmapItem*)m_viewer->imagePlaneLayer()->get(ImagePlaneLayer::Index::IMAGE);
    emit frameReady(QSharedPointer<QPixmap>(new QPixmap(item->pixmap())));
}

/* END SLOTS */

void ImageViewController::tabWidgetPlaneCurrentChanged(int index)
{
    qDebug() << "TAB IMAGE/OBJECT PLAN: " << index;

    m_viewer->resetZoom();
    if (index == m_layersTabWidget->indexOf(m_objPlaneTab))
    {
        m_viewer->imagePlaneLayer()->setVisible(false);
        m_viewer->objectPlaneLayer()->setVisible(true);

        QGraphicsItem* item = m_viewer->objectPlaneLayer()->get(ObjectPlaneLayer::Index::IMAGE);
        m_viewer->setSceneRect(item->boundingRect());
        m_viewer->fitInView(item, Qt::KeepAspectRatio);
    } else
    {
        m_viewer->imagePlaneLayer()->setVisible(true);
        m_viewer->objectPlaneLayer()->setVisible(false);

        QGraphicsItem* item = m_viewer->imagePlaneLayer()->get(ImagePlaneLayer::Index::IMAGE);
        m_viewer->setSceneRect(item->boundingRect());
        m_viewer->fitInView(item, Qt::KeepAspectRatio);
    }
}

void ImageViewController::checkBoxBackgroundStateChanged(int state)
{
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::IMAGE, state == Qt::Checked);
}

void ImageViewController::checkBoxCrossStateChanged(int state)
{
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::CROSS, state == Qt::Checked);
}

void ImageViewController::checkBoxReferenceSquareStateChanged(int state)
{
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::REFERENCE_SQUARE, state == Qt::Checked);
}

void ImageViewController::checkBoxReferenceSquareForSlitStateChanged(int state)
{
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::REFERENCE_SQUARE_FOR_SLIT,
                                                state == Qt::Checked);
}

void ImageViewController::checkBoxGridLinesStateChanged(int state)
{
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::GRID_LINES, state == Qt::Checked);
}

void ImageViewController::checkBoxGridPointsStateChanged(int state)
{
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::GRID_POINTS, state == Qt::Checked);
}

void ImageViewController::checkBoxSlicingStateChanged(int state)
{
    m_textEditMeasures->setVisible(state == Qt::Checked);

    if (state == Qt::Checked)
    {
        m_slicingStateMachine->setInitialState(m_slicingStateMachine->initialState());
        m_slicingStateMachine->start();
        emit doRemoveSicingResults();
        qDebug() << ">>>>>>>>>>>> START";
    } else
    {
        m_slicingStateMachine->stop();
        qDebug() << ">>>>>>>>>>>> STOP";
    }
}

void ImageViewController::checkBoxObjBackgroundStateChanged(int state)
{
    m_viewer->objectPlaneLayer()->setItemVisible(ObjectPlaneLayer::Index::IMAGE, state == Qt::Checked);
}

void ImageViewController::checkBoxObjGridPointsStateChanged(int state)
{
    m_viewer->objectPlaneLayer()->setItemVisible(ObjectPlaneLayer::Index::GRID_POINTS, state == Qt::Checked);
}

void ImageViewController::checkBoxObjMeasuresStateChanged(int state)
{
    m_textEditMeasures->setVisible(state == Qt::Checked);

    if (state == Qt::Checked)
    {
        m_measuresStateMachine->setInitialState(m_measuresStateMachine->initialState());
        m_measuresStateMachine->start();
        qDebug() << ">>>>>>>>>>>> START";
    } else
    {
        m_measuresStateMachine->stop();
        qDebug() << ">>>>>>>>>>>> STOP";
    }
}

void ImageViewController::checkBoxObjFoVSquareChanged(int state)
{
    m_viewer->objectPlaneLayer()->setItemVisible(ObjectPlaneLayer::Index::FOV_LINES, state == Qt::Checked);
}

void ImageViewController::checkBoxProfileChanged(int state)
{
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::PROFILE, state == Qt::Checked);
}
void ImageViewController::checkBoxMaximaPointsChanged(int state)
{
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::PROFILE_MAXIMA, state == Qt::Checked);
}

void ImageViewController::checkBoxBevelChanged(int state)
{
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::BEVEL, state == Qt::Checked);
}
void ImageViewController::checkBoxModelPointsChanged(int state)
{
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::MODEL_POINTS, state == Qt::Checked);
}
void ImageViewController::checkBoxDistancePointsChanged(int state)
{
    m_viewer->imagePlaneLayer()->setItemVisible(ImagePlaneLayer::Index::DISTANCE_POINTS, state == Qt::Checked);
}
void ImageViewController::checkBoxObjProfileChanged(int state)
{
    m_viewer->objectPlaneLayer()->setItemVisible(ObjectPlaneLayer::Index::PROFILE, state == Qt::Checked);
}
void ImageViewController::checkBoxObjMaximaPointsChanged(int state)
{
    m_viewer->objectPlaneLayer()->setItemVisible(ObjectPlaneLayer::Index::PROFILE_MAXIMA, state == Qt::Checked);
}
void ImageViewController::checkBoxObjBevelChanged(int state)
{
    m_viewer->objectPlaneLayer()->setItemVisible(ObjectPlaneLayer::Index::BEVEL, state == Qt::Checked);
}
void ImageViewController::checkBoxObjModelPointsChanged(int state)
{
    m_viewer->objectPlaneLayer()->setItemVisible(ObjectPlaneLayer::Index::MODEL_POINTS, state == Qt::Checked);
}
void ImageViewController::checkBoxObjDistancePointsChanged(int state)
{
    m_viewer->objectPlaneLayer()->setItemVisible(ObjectPlaneLayer::Index::DISTANCE_POINTS, state == Qt::Checked);
}