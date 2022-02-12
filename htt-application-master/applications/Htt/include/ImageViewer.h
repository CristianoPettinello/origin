#ifndef __IMAGE_VIEWER_H__
#define __IMAGE_VIEWER_H__

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtCharts>

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class ImageViewerLayers : public QGraphicsItem
{
public:
    explicit ImageViewerLayers(QGraphicsItem* parent = nullptr);
    ~ImageViewerLayers();

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = Q_NULLPTR);
    virtual void set(int index, QGraphicsItem* item);
    virtual QGraphicsItem* get(int index);
    virtual void setItemVisible(int index, bool visible);
    virtual void addItemToGroup(int index, QGraphicsItem* item);

protected:
    QMap<int, QGraphicsItem*> m_map;
};

class ImagePlaneLayer : public ImageViewerLayers
{
public:
    enum Index
    {
        IMAGE,
        CROSS,
        GRID_LINES,
        REFERENCE_SQUARE,
        REFERENCE_SQUARE_FOR_SLIT,
        SLICING,
        GRID_POINTS,
        PROFILE,
        PROFILE_MAXIMA,
        BEVEL,
        MODEL_POINTS,
        DISTANCE_POINTS
    };

public:
    explicit ImagePlaneLayer(QGraphicsItem* parent = nullptr);
    ~ImagePlaneLayer();
    void updateImage(QSharedPointer<QImage> image);

private:
    QPixmap m_pixmap;
};

class ObjectPlaneLayer : public ImageViewerLayers
{
public:
    enum Index
    {
        IMAGE,
        GRID_POINTS,
        FOV_LINES,
        MEASURE_POINTS,
        PROFILE,
        PROFILE_MAXIMA,
        BEVEL,
        MODEL_POINTS,
        DISTANCE_POINTS
    };

public:
    explicit ObjectPlaneLayer(QGraphicsItem* parent = nullptr);
    ~ObjectPlaneLayer();
    void updateImage(QSharedPointer<QImage> image);

private:
    QPixmap m_pixmap;
};

class ChartLayer : public ImageViewerLayers
{
public:
    explicit ChartLayer(QGraphicsItem* parent = nullptr);
    ~ChartLayer();
    virtual void set(int index, QGraphicsItem* item);
    void setChart(QChart* newChart);
    void updateSeries(QLineSeries* series);

private:
    QChart* m_chart;
};

class ImageViewer : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ImageViewer(QWidget* parent = nullptr);
    ~ImageViewer();

    void show(const QPixmap& pixmap);
    void addItem(QGraphicsItem* item);
    void removeItem(QGraphicsItem* item);
    void paintEvent(QPaintEvent* event);
    void clear();

public:
    ImagePlaneLayer* imagePlaneLayer() const;
    ObjectPlaneLayer* objectPlaneLayer() const;
    ChartLayer* chartLayer() const;
    void resetZoom();
    int zoomStep();

signals:
    void mousePressEvent(QPointF point);

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;

public slots:

private:
    QGraphicsScene* m_scene;
    bool m_isMouseLeftButtonPressed = false;
    QPoint m_anchorPoint;
    ImagePlaneLayer* m_imgPlaneLayer;
    ObjectPlaneLayer* m_objPlaneLayer;
    ChartLayer* m_chartLayer;
    int m_zoomStep;
    QGraphicsSimpleTextItem* m_zoomLabel;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __IMAGE_VIEWER_H__
