#ifndef __CALIBRATION_REPORT_GENERATOR_H__
#define __CALIBRATION_REPORT_GENERATOR_H__

#include <QObject>
#include <QPainter>
#include <QPdfWriter>

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class CalibrationReportGenerator : public QObject
{

    Q_OBJECT
public:
    explicit CalibrationReportGenerator(QObject* parent = nullptr);
    ~CalibrationReportGenerator();

    bool createReport(QString fileName, const QJsonObject& reportData);

private:
    // typedef enum
    // {
    //     SUCCESS,
    //     FAIL,
    //     WARNING
    // } ResultIcon;

private:
    void newPage(bool isFirstDocumentPage = false);
    void drawHeader(int page);
    void drawSectionTitle(QString title);
    void drawLine(QString text, bool bold = false);
    // void drawLineWithIcon(ResultIcon type, QString text, bool bold = false);

    void drawImage(QImage& image, QString caption = "", float zoom = 1.0f);
    void newVerticalSeparator(int size);
    // QString lineSeparator(int size);

    void getImageDataFromJson(const QJsonObject& reportData, QString path, QImage& outImage);
    QString getValue(const QJsonObject& reportData, QString path, int precision = -1);
    QString getString(const QJsonObject& reportData, QString path);
    QVariantList getList(const QJsonObject& reportData, QString path);
    QString convertMmToUm(QString input);

    void startTwoColumnSection();
    void endTwoColumnSection();
    void nextColumn();

private:
    QPainter m_painter;
    QPdfWriter* m_pdfWriter;

    int m_pageWidth;
    int m_pageHeight;
    int m_pageHBorder;
    int m_pageVBorder;

    int m_pageLeftMargin;
    int m_pageTopMargin;
    int m_pageRightMargin;
    int m_pageBottomMargin;

    int m_contentLeftMargin;
    int m_contentTopMargin;
    int m_contentRightMargin;
    int m_contentBottomMargin;

    int m_columnOffset;

    int m_page;
    int m_line;

    int m_captionFontSize;
    int m_baseFontSize;
    int m_titleFontSize;

    // Two Column sections
    int m_lineBeforeTwoColumns;
    int m_lineEndOfFirstColumn;
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __CALIBRATION_REPORT_GENERATOR_H__
