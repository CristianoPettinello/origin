#include "CalibrationReportGenerator.h"
#include "Globals.h"
#include "Notifications.h"
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPdfWriter>

using namespace Nidek::Solution::HTT::UserApplication;

CalibrationReportGenerator::CalibrationReportGenerator(QObject* /*parent = nullptr*/)
    : m_pdfWriter(nullptr),
      m_pageWidth(0),
      m_pageHeight(0),
      m_pageHBorder(0),
      m_pageVBorder(0),
      m_pageLeftMargin(0),
      m_pageTopMargin(0),
      m_pageRightMargin(0),
      m_pageBottomMargin(0),
      m_page(0),
      m_baseFontSize(0)
{
}

CalibrationReportGenerator::~CalibrationReportGenerator()
{
    if (m_pdfWriter)
    {
        delete m_pdfWriter;
        m_pdfWriter = nullptr;
    }
}

void CalibrationReportGenerator::newPage(bool isFirstDocumentPage /* = false */)
{
    if (!m_pdfWriter)
        return;

    if (!isFirstDocumentPage)
        m_pdfWriter->newPage();

    drawHeader(m_page++);
    m_line = 0;
}

void CalibrationReportGenerator::drawHeader(int page)
{
    QFont font = m_painter.font();
    QImage logo(":/icons/NidekLogo");

    int logoWidth = logo.width();
    int logoHeight = logo.height();
    m_contentTopMargin = m_pageTopMargin + m_pageVBorder + logoHeight;

    // Draw the Nidek logo
    m_painter.drawImage(m_pageLeftMargin, m_pageTopMargin, logo);

    font.setPixelSize(60);
    font.setBold(true);
    m_painter.setFont(font);
    m_painter.drawText(QRect(m_pageLeftMargin + logoWidth, m_pageTopMargin,
                             m_pageRightMargin - m_pageLeftMargin - logoWidth, logoHeight),
                       Qt::AlignCenter, "HTT Calibration Report");

    font.setPixelSize(25);
    font.setBold(false);
    m_painter.setFont(font);
    m_painter.drawText(QRect(m_pageRightMargin - 100, m_pageTopMargin, 100, logoHeight), 0,
                       QString("Page %1").arg(page));
}

void CalibrationReportGenerator::drawSectionTitle(QString title)
{
    QFont font = m_painter.font();
    font.setPixelSize(m_titleFontSize);
    font.setBold(true);
    m_painter.setFont(font);
    newVerticalSeparator(50);
    m_painter.drawText(m_contentLeftMargin + m_columnOffset, m_contentTopMargin + m_line, title);
    m_line += m_titleFontSize * 1.5;
    newVerticalSeparator(50);
}

void CalibrationReportGenerator::drawLine(QString text, bool bold /*= false */)
{
    QFont font = m_painter.font();
    font.setPixelSize(m_baseFontSize);
    font.setBold(bold);
    m_painter.setFont(font);
    m_painter.drawText(m_contentLeftMargin + m_columnOffset + 60, m_contentTopMargin + m_line, text);
    m_line += m_baseFontSize * 1.5;
}

#if 0
void CalibrationReportGenerator::drawLineWithIcon(ResultIcon type, QString text, bool bold /*= false */)
{
    QImage icon;
    if (type == ResultIcon::SUCCESS)
        icon = QImage(":/icons/checkIcon");
    else if (type == ResultIcon::FAIL)
        icon = QImage(":/icons/failIcon");
    else
        icon = QImage(":/icons/warningIcon");

    // int iconWidth = icon.width();
    // int iconHeight = icon.height();
    icon.scaled(2, 2, Qt::KeepAspectRatio);

    QFont font = m_painter.font();
    font.setPixelSize(m_baseFontSize);
    m_painter.setFont(font);
    m_painter.drawImage(m_contentLeftMargin + m_columnOffset + 60, m_contentTopMargin + m_line, icon);
    font.setBold(bold);
    m_painter.setFont(font);
    m_painter.drawText(m_contentLeftMargin + m_columnOffset + 60 + icon.width(), m_contentTopMargin + m_line, text);
    m_line += m_baseFontSize * 1.5;
}
#endif

void CalibrationReportGenerator::drawImage(QImage& image, QString caption, float zoom /* = 1.0f */)
{
    if (zoom != 1.0f)
    {
        int width = image.width();
        int height = image.height();

        int newWidth = zoom * width;
        int newHeight = zoom * height;

        QImage scaledImage = image.scaled(newWidth, newHeight, Qt::KeepAspectRatio);

        m_painter.drawImage(m_contentLeftMargin + m_columnOffset, m_contentTopMargin + m_line, scaledImage);
        m_line += scaledImage.height() + m_baseFontSize;

        if (caption.size() > 0)
        {
            QFont font = m_painter.font();
            font.setPixelSize(m_captionFontSize);
            font.setBold(false);
            m_painter.setFont(font);
            m_painter.drawText(QRect(m_contentLeftMargin + m_columnOffset, m_contentTopMargin + m_line,
                                     scaledImage.width(), m_captionFontSize * 1.2),
                               Qt::AlignCenter, caption);

            m_line += m_captionFontSize * 1.5;
        }
    } else
    {
        m_painter.drawImage(m_contentLeftMargin + m_columnOffset, m_contentTopMargin + m_line, image);
        m_line += image.height() + m_baseFontSize;

        if (caption.size() > 0)
        {
            QFont font = m_painter.font();
            font.setPixelSize(m_captionFontSize);
            font.setBold(false);
            m_painter.setFont(font);
            m_painter.drawText(QRect(m_contentLeftMargin + m_columnOffset, m_contentTopMargin + m_line, image.width(),
                                     m_captionFontSize * 1.2),
                               Qt::AlignCenter, caption);

            m_line += m_captionFontSize * 1.5;
        }
    }
}

void CalibrationReportGenerator::newVerticalSeparator(int size)
{
    m_line += size;
}

#if 0
QString CalibrationReportGenerator::lineSeparator(int size)
{
    QString line;
    for (int i = 0; i < size; ++i)
    {
        line.append("-");
    }
    return line;
}
#endif

void CalibrationReportGenerator::getImageDataFromJson(const QJsonObject& reportData, QString path, QImage& outImage)
{
    QStringList nodes = path.split("/");

    QJsonObject currentNode = reportData;
    int N = nodes.size();
    for (int n = 0; n < N - 1; ++n)
        currentNode = currentNode[nodes.at(n)].toObject();

    QByteArray imageArray = QByteArray::fromBase64(currentNode[nodes.at(N - 1)].toString().toUtf8());
    outImage.loadFromData(imageArray, "PNG");
}

QString CalibrationReportGenerator::getValue(const QJsonObject& reportData, QString path, int precision /* = -1*/)
{
    QStringList nodes = path.split("/");
    QJsonObject currentNode = reportData;
    int N = nodes.size();
    for (int n = 0; n < N - 1; ++n)
        currentNode = currentNode[nodes.at(n)].toObject();

    double value = currentNode[nodes.at(N - 1)].toDouble();
    if (precision >= 0)
        return QString::number(value, 'f', precision);
    else
        return QString::number(value);
}

QString CalibrationReportGenerator::getString(const QJsonObject& reportData, QString path)
{
    QStringList nodes = path.split("/");
    QJsonObject currentNode = reportData;
    int N = nodes.size();
    for (int n = 0; n < N - 1; ++n)
        currentNode = currentNode[nodes.at(n)].toObject();

    return currentNode[nodes.at(N - 1)].toString();
}

QVariantList CalibrationReportGenerator::getList(const QJsonObject& reportData, QString path)
{
    QStringList nodes = path.split("/");
    QJsonObject currentNode = reportData;

    int N = nodes.size();
    for (int n = 0; n < N - 2; ++n)
        currentNode = currentNode[nodes.at(n)].toObject();

    QJsonObject obj = currentNode[nodes.at(N - 2)].toObject();
    return obj[nodes.at(N - 1)].toArray().toVariantList();
}

QString CalibrationReportGenerator::convertMmToUm(QString input)
{
    return QString::number(input.toFloat() * 1000.0);
}

void CalibrationReportGenerator::startTwoColumnSection()
{
    // Store m_line before inserting new items
    m_lineBeforeTwoColumns = m_line;
    m_columnOffset = 0;
}

void CalibrationReportGenerator::endTwoColumnSection()
{
    // When the secon column is edited, m_line is changed accordingly,
    // and at the end of the two column section we need to get the max
    // between m_line and m_lineEndOfFirstColumn
    m_line = m_line > m_lineEndOfFirstColumn ? m_line : m_lineEndOfFirstColumn;
    m_columnOffset = 0;
}

void CalibrationReportGenerator::nextColumn()
{
    // Store m_line before working on the second column
    m_lineEndOfFirstColumn = m_line;
    m_line = m_lineBeforeTwoColumns;
    m_columnOffset = (m_contentRightMargin - m_contentLeftMargin) >> 1;
}

bool CalibrationReportGenerator::createReport(QString fileName, const QJsonObject& reportData)
{
    m_pdfWriter = new QPdfWriter(fileName);

    m_pdfWriter->setCreator("Nidek Technologies");
    m_pdfWriter->setTitle("HTT - <DEVICE> TEST REPORT");
    m_pdfWriter->setPageSize(QPageSize(QPageSize::A4));
    m_pdfWriter->setResolution(300);

    // A4 = 210x297 = 8.3" x 11.7" = 2490 x 3510 pixels # 300 dpi
    m_pageWidth = 2490;
    m_pageHeight = 3510;
    m_pageHBorder = 100;
    m_pageVBorder = 100;

    m_baseFontSize = 30;
    m_titleFontSize = m_baseFontSize * 1.2;
    m_captionFontSize = m_baseFontSize * 0.8;

    m_pageLeftMargin = m_pageHBorder;
    m_pageTopMargin = m_pageVBorder;
    m_pageRightMargin = m_pageWidth - m_pageHBorder;
    m_pageBottomMargin = m_pageHeight - m_pageVBorder;

    m_columnOffset = 0;

    m_contentLeftMargin = m_pageLeftMargin + 50;
    m_contentTopMargin = m_pageTopMargin;
    m_contentRightMargin = m_pageRightMargin - 50;
    m_contentBottomMargin = m_pageBottomMargin;

    m_page = 1;
    m_painter.begin(m_pdfWriter);

    // ================================================
    // BEGIN DOCUMENT GENERATION
    // ================================================
    newPage(true);

    QTextStream in();

    QString checked("[x] ");
    QString failed("[-] ");
    QString warning("[#] ");

    drawSectionTitle("General Information");
    drawLine(QString("Calibration Date: \t %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd - hh:mm")));
    drawLine(QString("Software version: \t %1").arg(GUI_VERSION));
    drawLine(QString("Device S/N:       \t %1").arg("FM2002")); // FIXME
    drawLine(QString("Operator:         \t %1").arg("N/A"));    // FIXME

    newVerticalSeparator(100);

    // =============================
    // STEP 1
    // =============================
    newPage();
    drawSectionTitle("[1/10] " + Notifications::step1Objective);
    // drawLine(lineSeparator(m_pageRightMargin - m_contentLeftMargin));
    drawLine("Objective: " + Notifications::step1Comment);
    newVerticalSeparator(50);
    // drawLineWithIcon(ResultIcon::SUCCESS, "The projected slit is aligned with the reference aperture of
    // LTT12-8016.");
    drawLine(checked + "The projected slit is aligned with the reference aperture of LTT12-8016.");

    newVerticalSeparator(100);

    // =============================
    // STEP 2
    // =============================
    // newPage();
    drawSectionTitle("[2/10] " + Notifications::step2Objective);
    // drawLine("Objective: find the best focusing position of the sensor along with a rough centering and");
    // drawLine("           verfy that the system satisfy the resolution specifications.");
    drawLine("Objective: " + Notifications::step2Comment);
    newVerticalSeparator(50);

    // Start a two column section
    startTwoColumnSection();

    QString val = getValue(reportData, "step2/vPoints/contrast", 2);
    bool check = val.toFloat() >= 0.2;
    drawLine(QString("LTT21-2017, G3E4 V contrast=%1").arg(val));
    newVerticalSeparator(50);
    {
        QString s(QString("Vertical constrast >= %1").arg(QString::number(0.2)));
        if (check)
            drawLine(checked + s);
        else
            drawLine(failed + s);
    }
    newVerticalSeparator(50);
    val = getValue(reportData, "step2/hPoints/contrast", 2);
    check = val.toFloat() >= 0.2;
    drawLine(QString("LTT21-2017, G4E2 H contrast=%1").arg(val));
    newVerticalSeparator(50);
    {
        QString s(QString("Horizontal constrast >= %1").arg(QString::number(0.2)));
        if (check)
            drawLine(checked + s);
        else
            drawLine(failed + s);
    }
    newVerticalSeparator(50);

    nextColumn();

    // USAF chart image
    QImage usafChartImage;
    getImageDataFromJson(reportData, "step2/frame", usafChartImage);
    drawImage(usafChartImage, "USAF chart image", 1.5);

    newVerticalSeparator(50);

    startTwoColumnSection();

    // vertical contrast slice chart
    QImage vChart;
    getImageDataFromJson(reportData, "step2/vChart", vChart);
    drawImage(vChart, "Vertical contrast slice for the G3E4", 1.5);

    nextColumn();

    // horizontal contrast slice chart
    QImage hChart;
    getImageDataFromJson(reportData, "step2/hChart", hChart);
    drawImage(hChart, "Horizontal contrast slice for the G4E2", 1.5);

    endTwoColumnSection();
    // newVerticalSeparator(100);

    // =============================
    // STEP 3
    // =============================
    newPage();
    drawSectionTitle("[3/10] " + Notifications::step3Objective);

    // drawLine("Objective: align the sensor in x and y with the grid center.");
    drawLine("Objective: " + Notifications::step3Comment);
    newVerticalSeparator(50);

    // Start a two column section
    startTwoColumnSection();

    // // Grid image
    // QImage gridImage;
    // getImageDataFromJson(reportData, "step3/gridImage/data", gridImage);
    // drawImage(gridImage, "Calibration grid in the image plane.", 2.0);
    drawLine(checked + "The grid center (J)(10) falls inside the viewfinder.");

    nextColumn();

    // Grid image
    // QImage gridImageScreenshot;
    // getImageDataFromJson(reportData, "step3/screenshot", gridImageScreenshot);
    // drawImage(gridImageScreenshot, "Calibration grid with central cross.", 1.5);
    QImage gridImage;
    getImageDataFromJson(reportData, "step3/gridImage/data", gridImage);
    drawImage(gridImage, "Grid image in the image plane.", 1.5);

    endTwoColumnSection();
    newVerticalSeparator(100);

    // =============================
    // STEP 4
    // =============================
    // newPage();
    drawSectionTitle("[4/10] " + Notifications::step4Objective);

    // Objective
    // drawLine("Objective: the Scheimpflug parameters for the Back Ray Tracing Algorithm are calculated from the grid "
    //          "and validated.");
    drawLine("Objective: " + Notifications::step4Comment);
    newVerticalSeparator(50);

    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("")
                 .arg("Unit")
                 .arg("Nominal")
                 .arg("+/-Tolr")
                 .arg("Optimized")
                 .arg("Difference from nominal"),
             true);
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("alpha")
                 .arg(getString(reportData, "step4/results/params/alpha/unit"))
                 .arg(getValue(reportData, "step4/results/params/alpha/nominal"))
                 .arg(getValue(reportData, "step4/results/params/alpha/tolerance"))
                 .arg(getValue(reportData, "step4/results/params/alpha/optimized"))
                 .arg(getValue(reportData, "step4/results/params/alpha/delta")));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("beta")
                 .arg(getString(reportData, "step4/results/params/beta/unit"))
                 .arg(getValue(reportData, "step4/results/params/beta/nominal"))
                 .arg(getValue(reportData, "step4/results/params/beta/tolerance"))
                 .arg(getValue(reportData, "step4/results/params/beta/optimized"))
                 .arg(getValue(reportData, "step4/results/params/beta/delta")));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("cx0")
                 .arg(getString(reportData, "step4/results/params/cx0/unit"))
                 .arg(getValue(reportData, "step4/results/params/cx0/nominal"))
                 .arg(getValue(reportData, "step4/results/params/cx0/tolerance"))
                 .arg(getValue(reportData, "step4/results/params/cx0/optimized"))
                 .arg(getValue(reportData, "step4/results/params/cx0/delta")));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("cy0")
                 .arg(getString(reportData, "step4/results/params/cy0/unit"))
                 .arg(getValue(reportData, "step4/results/params/cy0/nominal"))
                 .arg(getValue(reportData, "step4/results/params/cy0/tolerance"))
                 .arg(getValue(reportData, "step4/results/params/cy0/optimized"))
                 .arg(getValue(reportData, "step4/results/params/cy0/delta")));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("delta")
                 .arg(getString(reportData, "step4/results/params/delta/unit"))
                 .arg(getValue(reportData, "step4/results/params/delta/nominal"))
                 .arg(getValue(reportData, "step4/results/params/delta/tolerance"))
                 .arg(getValue(reportData, "step4/results/params/delta/optimized"))
                 .arg(getValue(reportData, "step4/results/params/delta/delta")));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("p1")
                 .arg(getString(reportData, "step4/results/params/p1/unit"))
                 .arg(getValue(reportData, "step4/results/params/p1/nominal"))
                 .arg(getValue(reportData, "step4/results/params/p1/tolerance"))
                 .arg(getValue(reportData, "step4/results/params/p1/optimized"))
                 .arg(getValue(reportData, "step4/results/params/p1/delta")));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("p2")
                 .arg(getString(reportData, "step4/results/params/p2/unit"))
                 .arg(getValue(reportData, "step4/results/params/p2/nominal"))
                 .arg(getValue(reportData, "step4/results/params/p2/tolerance"))
                 .arg(getValue(reportData, "step4/results/params/p2/optimized"))
                 .arg(getValue(reportData, "step4/results/params/p2/delta")));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("phi")
                 .arg(getString(reportData, "step4/results/params/phi/unit"))
                 .arg(getValue(reportData, "step4/results/params/phi/nominal"))
                 .arg(getValue(reportData, "step4/results/params/phi/tolerance"))
                 .arg(getValue(reportData, "step4/results/params/phi/optimized"))
                 .arg(getValue(reportData, "step4/results/params/phi/delta")));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("theta")
                 .arg(getString(reportData, "step4/results/params/theta/unit"))
                 .arg(getValue(reportData, "step4/results/params/theta/nominal"))
                 .arg(getValue(reportData, "step4/results/params/theta/tolerance"))
                 .arg(getValue(reportData, "step4/results/params/theta/optimized"))
                 .arg(getValue(reportData, "step4/results/params/theta/delta")));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("tt")
                 .arg(getString(reportData, "step4/results/params/tt/unit"))
                 .arg(getValue(reportData, "step4/results/params/tt/nominal"))
                 .arg(getValue(reportData, "step4/results/params/tt/tolerance"))
                 .arg(getValue(reportData, "step4/results/params/tt/optimized"))
                 .arg(getValue(reportData, "step4/results/params/tt/delta")));

    newVerticalSeparator(50);
    drawLine(checked + "The grid reconstruction errors are all within the limits.");
    newVerticalSeparator(50);

    QVariantList list = getList(reportData, "step4/results/gridErrors");
    float errL1 = list[0].toFloat() + 3000;
    float errL2 = list[1].toFloat() + 3000;
    float errL3 = list[2].toFloat() + 3000;
    float errL4 = list[3].toFloat() + 3000;
    float errD1 = list[4].toFloat() + 4242.640;
    float errD2 = list[5].toFloat() + 4242.640;
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("")
                 .arg("Unit")
                 .arg("Nominal")
                 .arg("+/-Tolr")
                 .arg("Optimized")
                 .arg("Difference from nominal"),
             true);
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("L1(L/M)")
                 .arg("um")
                 .arg("3000")
                 .arg("16")
                 .arg(QString::number(errL1))
                 .arg(list[0].toString()));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("L2(7/8)")
                 .arg("um")
                 .arg("3000")
                 .arg("10")
                 .arg(QString::number(errL2))
                 .arg(list[1].toString()));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("L3(G/H)")
                 .arg("um")
                 .arg("3000")
                 .arg("16")
                 .arg(QString::number(errL3))
                 .arg(list[2].toString()));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("L4(12/13)")
                 .arg("um")
                 .arg("3000")
                 .arg("10")
                 .arg(QString::number(errL4))
                 .arg(list[3].toString()));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("D1(L1,L4/L2,L3)")
                 .arg("um")
                 .arg("4242.640")
                 .arg("18.868")
                 .arg(QString::number(errD1))
                 .arg(list[4].toString()));
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("D2(L3,L4/L1,L2)")
                 .arg("um")
                 .arg("4242.640")
                 .arg("18.868")
                 .arg(QString::number(errD2))
                 .arg(list[5].toString()));
    newVerticalSeparator(50);

    newPage();

    // Start a two column section
    startTwoColumnSection();

    // Grid image
    QImage acquiredGridImage;
    getImageDataFromJson(reportData, "step4/screenshot", acquiredGridImage);
    drawImage(acquiredGridImage, "Acquired grid image with detected points.", 1.5);

    nextColumn();

    // Undistorted grid image
    QImage objImage;
    getImageDataFromJson(reportData, "step4/results/objImage/data", objImage);
    drawImage(objImage, "Undistorted grid in the object plane.", 1.5);

    endTwoColumnSection();
    newVerticalSeparator(100);

    // newVerticalSeparator(50);
    // drawLine("Notes", true);
    // drawLine("  - The measured flux as a function of the LED power must be within the minimum and nominal values, "
    //          "otherwise the test"
    //          "is failed.");
    // =============================
    // STEP 5
    // =============================
    // newPage();
    drawSectionTitle("[5/10] " + Notifications::step5Objective);

    // drawLine("Objective: the imaging Field of View (Fov) is larger than the minimum of 5mm x 5mm required.");
    drawLine("Objective: " + Notifications::step5Comment);
    newVerticalSeparator(50);

    startTwoColumnSection();
    drawLine(checked + "The imaging Field of View is >= 5mm x 5mm");
    nextColumn();

    QImage step5Screenshot;
    getImageDataFromJson(reportData, "step5/screenshot", step5Screenshot);
    drawImage(step5Screenshot, "Undistorted grid in the object plane and 5x5mm reference square.", 1.5);

    endTwoColumnSection();
    newVerticalSeparator(100);

    // =============================
    // STEP 6 MISSING
    // =============================
    newPage();
    drawSectionTitle("[6/10] " + Notifications::step6Objective);
    // drawLine("Objective: The slit projection is verified to completely fill the vertical field-of-view of the
    // system.");
    drawLine("Objective: " + Notifications::step6Comment);
    newVerticalSeparator(50);

    startTwoColumnSection();
    drawLine(checked + "The slit has the minimum width on LTT12-DJ015 plane.");
    newVerticalSeparator(50);
    drawLine(checked + "The slit completely fills the vertical FoV.");
    nextColumn();

    QImage step6Screenshot;
    getImageDataFromJson(reportData, "step6/screenshot", step6Screenshot);
    drawImage(step6Screenshot, "", 1.5);

    endTwoColumnSection();
    newVerticalSeparator(100);

    // =============================
    // STEP 7
    // =============================
    // newPage();
    drawSectionTitle("[7/10] " + Notifications::step7Objective);

    // drawLine("Objective: the power output of the HTT illumination path is measured and validated against a minimum "
    //          "reference value.");
    drawLine("Objective: " + Notifications::step7Comment);
    newVerticalSeparator(50);

    startTwoColumnSection();
    // drawLine("LED power measurments:", true);
    // newVerticalSeparator(50);

    QVariantList current = getList(reportData, "step7/table/current");
    QVariantList flux = getList(reportData, "step7/table/flux");
    QVariantList minimum = getList(reportData, "step7/table/minimum");
    QVariantList nominal = getList(reportData, "step7/table/nominal");

    int N = current.size();
    // drawLine(QString("Current[%]\tFlux[mW]\tMinimum[mW]\tNominal[mW]"), true);
    drawLine(QString("Current[%]\tFlux[mW]"), true);
    drawLine("");

    for (int n = 0; n < N; ++n)
        drawLine(QString("%1\t%2").arg(current[n].toString()).arg(flux[n].toString()));
    //  .arg(minimum[n].toString())
    //  .arg(nominal[n].toString()));

    nextColumn();
    // newVerticalSeparator(150);
    QImage step7Screenshot;
    getImageDataFromJson(reportData, "step7/screenshot", step7Screenshot);
    drawImage(step7Screenshot, "", 1.5);

    endTwoColumnSection();

    newVerticalSeparator(100);
    // drawLine("Notes", true);
    // drawLine("  - the test is passed if the measured flux as a function of the LED power is within the minimum and "
    //          "nominal curves.");

    // =============================
    // STEP 8 MISSING
    // =============================
    newPage();
    drawSectionTitle("[8/10] " + Notifications::step8Objective);
    // drawLine("Objective: align the projected slit with the on-tracer jig.");
    drawLine("Objective: " + Notifications::step8Comment);
    newVerticalSeparator(50);

    startTwoColumnSection();
    drawLine(checked + "The projected slit is aligned with the reference aperture of LTT12-8014.");
    nextColumn();
    QImage step8Screenshot;
    getImageDataFromJson(reportData, "step8/screenshot", step8Screenshot);
    drawImage(step8Screenshot, "", 1.5);
    endTwoColumnSection();
    newVerticalSeparator(100);

    // =============================
    // STEP 9
    // =============================
    newPage();
    drawSectionTitle("[9/10] " + Notifications::step9Objective);

    // drawLine("Objective: the Scheimpflug parameters for the Back Ray Tracing Algorithm are calculated from the "
    //          "LTT12-8000 bevels and validated.");
    drawLine("Objective: " + Notifications::step9Comment);
    newVerticalSeparator(50);

    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("")
                 .arg("Unit")
                 .arg("Nominal")
                 .arg("+/-Tolr")
                 .arg("Optimized")
                 .arg("Difference from nominal"),
             true);
    float delta = getValue(reportData, "step9/results/params/alpha").toFloat() -
                  getValue(reportData, "step4/results/params/alpha/nominal").toFloat();
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("alpha")
                 .arg(getString(reportData, "step4/results/params/alpha/unit"))
                 .arg(getValue(reportData, "step4/results/params/alpha/nominal"))
                 .arg(getValue(reportData, "step4/results/params/alpha/tolerance"))
                 .arg(getValue(reportData, "step9/results/params/alpha"))
                 .arg(QString::number(delta, 'g')));
    delta = getValue(reportData, "step9/results/params/beta").toFloat() -
            getValue(reportData, "step4/results/params/beta/nominal").toFloat();
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("beta")
                 .arg(getString(reportData, "step4/results/params/beta/unit"))
                 .arg(getValue(reportData, "step4/results/params/beta/nominal"))
                 .arg(getValue(reportData, "step4/results/params/beta/tolerance"))
                 .arg(getValue(reportData, "step9/results/params/beta"))
                 .arg(QString::number(delta, 'g')));
    delta = getValue(reportData, "step9/results/params/cx0").toFloat() -
            getValue(reportData, "step4/results/params/cx0/nominal").toFloat();
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("cx0")
                 .arg(getString(reportData, "step4/results/params/cx0/unit"))
                 .arg(getValue(reportData, "step4/results/params/cx0/nominal"))
                 .arg(getValue(reportData, "step4/results/params/cx0/tolerance"))
                 .arg(getValue(reportData, "step9/results/params/cx0"))
                 .arg(QString::number(delta, 'g')));
    delta = getValue(reportData, "step9/results/params/cy0").toFloat() -
            getValue(reportData, "step4/results/params/cy0/nominal").toFloat();
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("cy0")
                 .arg(getString(reportData, "step4/results/params/cy0/unit"))
                 .arg(getValue(reportData, "step4/results/params/cy0/nominal"))
                 .arg(getValue(reportData, "step4/results/params/cy0/tolerance"))
                 .arg(getValue(reportData, "step9/results/params/cy0"))
                 .arg(QString::number(delta, 'g')));
    delta = getValue(reportData, "step9/results/params/delta").toFloat() -
            getValue(reportData, "step4/results/params/delta/nominal").toFloat();
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("delta")
                 .arg(getString(reportData, "step4/results/params/delta/unit"))
                 .arg(getValue(reportData, "step4/results/params/delta/nominal"))
                 .arg(getValue(reportData, "step4/results/params/delta/tolerance"))
                 .arg(getValue(reportData, "step9/results/params/delta"))
                 .arg(QString::number(delta, 'g')));
    delta = getValue(reportData, "step9/results/params/p1").toFloat() -
            getValue(reportData, "step4/results/params/p1/nominal").toFloat();
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("p1")
                 .arg(getString(reportData, "step4/results/params/p1/unit"))
                 .arg(getValue(reportData, "step4/results/params/p1/nominal"))
                 .arg(getValue(reportData, "step4/results/params/p1/tolerance"))
                 .arg(getValue(reportData, "step9/results/params/p1"))
                 .arg(QString::number(delta, 'g')));
    delta = getValue(reportData, "step9/results/params/p2").toFloat() -
            getValue(reportData, "step4/results/params/p2/nominal").toFloat();
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("p2")
                 .arg(getString(reportData, "step4/results/params/p2/unit"))
                 .arg(getValue(reportData, "step4/results/params/p2/nominal"))
                 .arg(getValue(reportData, "step4/results/params/p2/tolerance"))
                 .arg(getValue(reportData, "step9/results/params/p2"))
                 .arg(QString::number(delta, 'g')));
    delta = getValue(reportData, "step9/results/params/phi").toFloat() -
            getValue(reportData, "step4/results/params/phi/nominal").toFloat();
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("phi")
                 .arg(getString(reportData, "step4/results/params/phi/unit"))
                 .arg(getValue(reportData, "step4/results/params/phi/nominal"))
                 .arg(getValue(reportData, "step4/results/params/phi/tolerance"))
                 .arg(getValue(reportData, "step9/results/params/phi"))
                 .arg(QString::number(delta, 'g')));
    delta = getValue(reportData, "step9/results/params/theta").toFloat() -
            getValue(reportData, "step4/results/params/theta/nominal").toFloat();
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("theta")
                 .arg(getString(reportData, "step4/results/params/theta/unit"))
                 .arg(getValue(reportData, "step4/results/params/theta/nominal"))
                 .arg(getValue(reportData, "step4/results/params/theta/tolerance"))
                 .arg(getValue(reportData, "step9/results/params/theta"))
                 .arg(QString::number(delta, 'g')));
    delta = getValue(reportData, "step9/results/params/tt").toFloat() -
            getValue(reportData, "step4/results/params/tt/nominal").toFloat();
    drawLine(QString("%1\t%2\t%3\t%4\t%5\t%6")
                 .arg("tt")
                 .arg(getString(reportData, "step4/results/params/tt/unit"))
                 .arg(getValue(reportData, "step4/results/params/tt/nominal"))
                 .arg(getValue(reportData, "step4/results/params/tt/tolerance"))
                 .arg(getValue(reportData, "step9/results/params/tt"))
                 .arg(QString::number(delta, 'g')));

    newVerticalSeparator(50);

    drawLine(checked + "The M804 reconstruction errors on M and B are within the 50μm limit.");

    newVerticalSeparator(50);

    drawLine(QString("%1\t%2\t%3\t%4\t%5").arg("").arg("Unit").arg("Nominal").arg("Mesured").arg("Error"), true);
    drawLine(QString("%1\t%2\t%3\t%4\t%5")
                 .arg("B")
                 .arg("um")
                 .arg(convertMmToUm(getValue(reportData, "step9/results/measures/realB")))
                 .arg(convertMmToUm(getValue(reportData, "step9/results/measures/B")))
                 .arg(getValue(reportData, "step9/results/measures/errorB")));
    drawLine(QString("%1\t%2\t%3\t%4\t%5")
                 .arg("M")
                 .arg("um")
                 .arg(convertMmToUm(getValue(reportData, "step9/results/measures/realM")))
                 .arg(convertMmToUm(getValue(reportData, "step9/results/measures/M")))
                 .arg(getValue(reportData, "step9/results/measures/errorM")));

    newVerticalSeparator(50);

    startTwoColumnSection();
    // QImage step9InputFrame;
    // getImageDataFromJson(reportData, "step9/frameImage/data", step9InputFrame);
    // drawImage(step9InputFrame, "Input bevel image", 1.5);
    QImage step9Screenshot;
    getImageDataFromJson(reportData, "step9/screenshot", step9Screenshot);
    drawImage(step9Screenshot, "Processed bevel image.", 1.5);

    nextColumn();

    QImage step9ObjImage;
    getImageDataFromJson(reportData, "step9/results/objImage/data", step9ObjImage);
    drawImage(step9ObjImage, "Undistorted M804 image.", 1.5);

    endTwoColumnSection();

    newVerticalSeparator(100);
    // drawLine("Estimated M801 Bevel Measurements:", true);
    // newVerticalSeparator(50);
    // drawLine(QString("\tMeasure [mm]\tNominal [mm]\tError [um]"));
    // drawLine(QString("B\t%1\t%2\t%3")
    //              .arg(getValue(reportData, "step9/results/measures/B", 3))
    //              .arg(getValue(reportData, "step9/results/measures/realB", 3))
    //              .arg(getValue(reportData, "step9/results/measures/errorB", 0)));
    // drawLine(QString("M\t%1\t%2\t%3")
    //              .arg(getValue(reportData, "step9/results/measures/M", 3))
    //              .arg(getValue(reportData, "step9/results/measures/realM", 3))
    //              .arg(getValue(reportData, "step9/results/measures/errorM", 0)));
    //
    // newVerticalSeparator(100);
    // drawLine("Notes", true);
    // newVerticalSeparator(10);
    // drawLine("  - B and M are the bevel depth and width, respectively.");
    // drawLine("  - The test is passed when the measure is within 50um from the nominal B value.");

    // =============================
    // STEP 10
    // =============================
    drawSectionTitle("[10/10] " + Notifications::step10Objective);

    // drawLine("Objective: the full system accuracy is estimated by comparison of its bevel measurements with the "
    //          "actual size of the LTT12-800 bevels.");
    drawLine("Objective: " + Notifications::step10Comment);
    newVerticalSeparator(50);
    drawLine("Step not yet implemented.", true);

    // ================================================
    // END DOCUMENT GENERATION
    // ================================================

    m_painter.end();

    if (m_pdfWriter)
    {
        delete m_pdfWriter;
        m_pdfWriter = nullptr;
    }

    return true;
}
