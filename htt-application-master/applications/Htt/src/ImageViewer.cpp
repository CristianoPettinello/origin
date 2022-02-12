#include "ImageViewer.h"

#include <QChart>
#include <QDebug>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QScrollBar>
#include <QtMath>

using namespace Nidek::Solution::HTT::UserApplication;

ImageViewer::ImageViewer(QWidget* parent) : QGraphicsView(parent), m_zoomStep(0)
{
    // created a QGraphicsScene object, scene
    m_scene = new QGraphicsScene(this);

    // put the scene to graphicsView
    setScene(m_scene);

    // Disable scrollBar
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setBackgroundBrush(Qt::black);
    setStyleSheet("border-width: 0px; border-style: solid");

    m_imgPlaneLayer = new ImagePlaneLayer();
    m_objPlaneLayer = new ObjectPlaneLayer();
    m_chartLayer = new ChartLayer();

    m_zoomLabel = new QGraphicsSimpleTextItem();
    m_zoomLabel->setFlags(QGraphicsItem::ItemIgnoresTransformations);
    m_zoomLabel->setBrush(Qt::yellow);

    addItem(m_imgPlaneLayer);
    addItem(m_objPlaneLayer);
    addItem(m_chartLayer);
    addItem(m_zoomLabel);
}

ImageViewer::~ImageViewer()
{
    if (m_scene)
        delete m_scene;
}

void ImageViewer::show(const QPixmap& pixmap)
{
    m_scene->addPixmap(pixmap);
}

void ImageViewer::addItem(QGraphicsItem* item)
{
    m_scene->addItem(item);
}

void ImageViewer::removeItem(QGraphicsItem* item)
{
    m_scene->removeItem(item);
}

void ImageViewer::paintEvent(QPaintEvent* event)
{
    // Anchor the zoom label to the top-left corner of the Viewer
    QPointF scenePos = mapToScene(0, 0); // map viewport's top-left corner to scene
    m_zoomLabel->setPos(scenePos);

    QGraphicsView::paintEvent(event);
}

void ImageViewer::clear()
{
    m_scene->clear();
}

ImagePlaneLayer* ImageViewer::imagePlaneLayer() const
{
    return m_imgPlaneLayer;
}

ObjectPlaneLayer* ImageViewer::objectPlaneLayer() const
{
    return m_objPlaneLayer;
}

ChartLayer* ImageViewer::chartLayer() const
{
    return m_chartLayer;
}

void ImageViewer::resetZoom()
{
    m_zoomStep = 0;
    m_zoomLabel->setText("");
}

int ImageViewer::zoomStep()
{
    return m_zoomStep;
}

void ImageViewer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_isMouseLeftButtonPressed = true;
        m_anchorPoint = event->pos();

        // Emit the event to the ImageViewerController
        emit mousePressEvent(mapToScene(m_anchorPoint));

        // don't propagate the event
        event->accept();
    }
}

void ImageViewer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_isMouseLeftButtonPressed = false;

        // don't propagate the event
        event->accept();
    }
}

void ImageViewer::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isMouseLeftButtonPressed)
    {
        // Pan the image following the mouse pointer
        QPointF delta = event->pos() - m_anchorPoint;

        auto dx = static_cast<int>(delta.x());
        auto dy = static_cast<int>(delta.y());

        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - dx);
        verticalScrollBar()->setValue(verticalScrollBar()->value() - dy);

        // don't propagate the event
        event->accept();

        m_anchorPoint = event->pos();
    }
}

void ImageViewer::wheelEvent(QWheelEvent* event)
{
    // Do a wheel-based zoom about the cursor position
    int numSteps = event->delta() / 15 / 8;

    if (numSteps == 0)
    {
        event->ignore();
        return;
    }

    // the zoom-out range is limited to the point at which the output image perfectly fits its containing window.
    if (m_zoomStep + numSteps < 0)
    {
        event->ignore();
        return;
    } else
    {
        qreal sc = pow(1.25, numSteps); // I use scale factor 1.25
        scale(sc, sc);
        m_zoomStep += numSteps;

        if (m_zoomStep > 0)
            m_zoomLabel->setText("Zoomed " + QString::number(pow(1.25, m_zoomStep), 'f', 2) + "x");
        else
            m_zoomLabel->setText("");

        // don't propagate the event
        event->accept();
    }

#if 0
    // Do a wheel-based zoom about the cursor position
    double angle = event->angleDelta().y();
    double factor = qPow(1.0015, angle);

    auto targetViewportPos = event->pos();
    auto targetScenePos = mapToScene(targetViewportPos);

    scale(factor, factor);
    centerOn(targetScenePos);
    QPointF deltaViewportPos = targetViewportPos - QPointF(viewport()->width() / 2.0, viewport()->height() / 2.0);
    QPointF viewportCenter = mapFromScene(targetScenePos) - deltaViewportPos;
    centerOn(mapToScene(viewportCenter.toPoint()));

    // don't propagate the event
    event->accept();
#endif
}

ImageViewerLayers::ImageViewerLayers(QGraphicsItem* parent /*= nullptr*/) : QGraphicsItem(parent)
{
}

ImageViewerLayers::~ImageViewerLayers()
{
}

QRectF ImageViewerLayers::boundingRect() const
{
    float minX, maxX, minY, maxY;
    auto items = childItems();
    for (QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); ++it)
    {
        QRectF r = (*it)->boundingRect();
        if (it == items.begin())
        {
            minX = r.x();
            maxX = minX + r.width();
            minY = r.y();
            maxY = minY + r.height();
        } else
        {
            if (r.x() < minX)
                minX = r.x();
            if (r.y() < minY)
                minY = r.y();
            if (r.x() > maxX)
                maxX = r.x();
            if (r.y() > maxY)
                maxY = r.y();
        }
    }
    return QRectF(minX, minY, maxX - minX, maxY - minY);
}

void ImageViewerLayers::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                              QWidget* widget /* = Q_NULLPTR*/)
{
    auto items = childItems();
    for (QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); ++it)
    {
        if ((*it)->isVisible())
            (*it)->paint(painter, option, widget);
    }
}

void ImageViewerLayers::set(int index, QGraphicsItem* item)
{
    if (item != nullptr)
    {
        QGraphicsItem* currentItem = m_map[index];
        QGraphicsScene* scene = currentItem->scene();
        if (scene)
            scene->removeItem(currentItem);

        m_map.remove(index);

        m_map.insert(index, item);
        item->setZValue(index); // TODO: replace with map
        item->setVisible(currentItem->isVisible());
        // Adds this graphics item to the scene of the parent
        item->setParentItem(this);

        if (item != currentItem)
            delete currentItem;
    }
}

void ImageViewerLayers::setItemVisible(int index, bool visible)
{
    m_map[index]->setVisible(visible);
}

void ImageViewerLayers::addItemToGroup(int index, QGraphicsItem* item)
{
    QGraphicsItemGroup* group = qgraphicsitem_cast<QGraphicsItemGroup*>(m_map[index]);
    if (group)
    {
        group->addToGroup(item);
        set(index, group);
    }
}

QGraphicsItem* ImageViewerLayers::get(int index)
{
    return m_map[index];
}

ImagePlaneLayer::ImagePlaneLayer(QGraphicsItem* parent /*= nullptr*/) : ImageViewerLayers(parent)
{
    m_map.insert(IMAGE, new QGraphicsPixmapItem(m_pixmap, this));
    m_map.insert(CROSS, new QGraphicsItemGroup(this));
    m_map.insert(REFERENCE_SQUARE, new QGraphicsItemGroup(this));
    m_map.insert(REFERENCE_SQUARE_FOR_SLIT, new QGraphicsItemGroup(this));
    m_map.insert(SLICING, new QGraphicsItemGroup(this));
    m_map.insert(GRID_LINES, new QGraphicsItemGroup(this));
    m_map.insert(GRID_POINTS, new QGraphicsItemGroup(this));
    m_map.insert(PROFILE, new QGraphicsItemGroup(this));
    m_map.insert(PROFILE_MAXIMA, new QGraphicsItemGroup(this));
    m_map.insert(BEVEL, new QGraphicsItemGroup(this));
    m_map.insert(MODEL_POINTS, new QGraphicsItemGroup(this));
    m_map.insert(DISTANCE_POINTS, new QGraphicsItemGroup(this));
}

ImagePlaneLayer::~ImagePlaneLayer()
{
}

void ImagePlaneLayer::updateImage(QSharedPointer<QImage> image)
{
    m_pixmap.convertFromImage(*image.data());
    ((QGraphicsPixmapItem*)m_map[IMAGE])->setPixmap(m_pixmap);
}

ObjectPlaneLayer::ObjectPlaneLayer(QGraphicsItem* parent /*= nullptr*/) : ImageViewerLayers(parent)
{
    m_map.insert(IMAGE, new QGraphicsPixmapItem(m_pixmap, this));
    m_map.insert(GRID_POINTS, new QGraphicsItemGroup(this));
    m_map.insert(FOV_LINES, new QGraphicsItemGroup(this));
    m_map.insert(MEASURE_POINTS, new QGraphicsItemGroup(this));
    m_map.insert(PROFILE, new QGraphicsItemGroup(this));
    m_map.insert(PROFILE_MAXIMA, new QGraphicsItemGroup(this));
    m_map.insert(BEVEL, new QGraphicsItemGroup(this));
    m_map.insert(MODEL_POINTS, new QGraphicsItemGroup(this));
    m_map.insert(DISTANCE_POINTS, new QGraphicsItemGroup(this));
}

ObjectPlaneLayer::~ObjectPlaneLayer()
{
}

void ObjectPlaneLayer::updateImage(QSharedPointer<QImage> image)
{
    m_pixmap.fromImage(*image.data());
    ((QGraphicsPixmapItem*)m_map[IMAGE])->setPixmap(m_pixmap);
}

ChartLayer::ChartLayer(QGraphicsItem* parent /*= nullptr*/) : ImageViewerLayers(parent), m_chart(new QChart(this))
{
}

ChartLayer::~ChartLayer()
{
}

void ChartLayer::set(int /*index*/, QGraphicsItem* /*item*/)
{
    // Override ImageViewerLayers::update(int index, QGraphicsItem* item)
}

void ChartLayer::setChart(QChart* newChart)
{
    QGraphicsItem* item = m_chart->graphicsItem();
    if (item)
    {
        QGraphicsScene* scene = item->scene();
        if (scene)
            scene->removeItem(item);

        newChart->setParentItem(this);
        newChart->setVisible(m_chart->isVisible());

        if (m_chart)
        {
            delete m_chart;
            m_chart = nullptr;
        }
    }
    m_chart = newChart;
}

void ChartLayer::updateSeries(QLineSeries* series)
{
    m_chart->removeSeries(series);
    m_chart->addSeries(series);
    // FIXME: axes are not updated correctly
    m_chart->createDefaultAxes();
}
