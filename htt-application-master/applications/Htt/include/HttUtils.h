#ifndef __HTT_UTILS_H__
#define __HTT_UTILS_H__

#include "Htt.h"
#include "Image.h"
#include "json/json.h"
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSharedPointer>
#include <chrono>
#include <ctime>
#include <iomanip>

using namespace std;
using namespace Nidek::Libraries::HTT;
using namespace Nidek::Libraries::Utils;

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {

class HttUtils
{
public:
    static shared_ptr<Image<uint8_t>> convertQImageToImage(QSharedPointer<QImage> qImage,
                                                           QImage::Format format = QImage::Format_RGB888)
    {
        // Convert qImage to Image<uint8_t>
        QImage input = qImage.data()->convertToFormat(format);
        int nChannels = input.depth() / 8;
        int width = input.width();
        int height = input.height();
        int size = width * height * nChannels;
        shared_ptr<Image<uint8_t>> output(new Image<uint8_t>(width, height, nChannels));
        memcpy(output->getData(), input.constBits(), size);
        return output;
    }

    static QSharedPointer<QImage> convertImageToQImage(shared_ptr<Image<uint8_t>> image)
    {
        int nChannels = image->getNChannels();
        int width = image->getWidth();
        int height = image->getHeight();
        int size = width * height * nChannels;
        QImage::Format format;
        if (nChannels == 3)
            format = QImage::Format_RGB888;
        else
            format = QImage::Format_Grayscale8;
        QSharedPointer<QImage> output = QSharedPointer<QImage>(new QImage(width, height, format));
        memcpy(output->bits(), image->getData(), size);
        return output;
    }

    static QJsonObject loadJsonFile(QString filename)
    {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "File open error";
            return QJsonObject();
        }
        QString val = file.readAll();
        file.close();

        // Reads json file using jsoncpp lib in order to accept also the comments
        Json::Value root;
        stringstream ss(val.toStdString());
        ss >> root;
        return QJsonDocument::fromJson(HttUtils::toQString(root).toUtf8()).object();
    }

    static void saveJsonFile(const QJsonObject& root, const QString& filename)
    {
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            qDebug() << "File open error";
            return;
        }
        QJsonDocument document;
        document.setObject(root);
        file.write(document.toJson());
        file.close();
    }

#if 0
    static QString now()
    {
        time_t rawtime;
        struct tm timeinfo;
        char buffer[80];

        time(&rawtime);
        localtime_s(&timeinfo, &rawtime);

        strftime(buffer, sizeof(buffer), "%d%m%Y_%H%M%S", &timeinfo);
        return QString(buffer);
    }
#else
    static QString now()
    {
        using namespace chrono;
        // get current time
        auto now = system_clock::now();
        // get number of milliseconds for the current second
        // (remainder after division into seconds)
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        // convert to std::time_t in order to convert to std::tm (broken time)
        auto timer = system_clock::to_time_t(now);
        // convert to broken time
        tm bt = *localtime(&timer);

        ostringstream oss;

        // yyyy:mm:dd-HH:MM:SS-sss
        oss << put_time(&bt, "%Y%m%d-%H%M%S");
        oss << '-' << setfill('0') << setw(3) << ms.count();

        qDebug() << QString::fromStdString(oss.str());
        return QString::fromStdString(oss.str());
    }
#endif

private:
    static QString toQString(const Json::Value& json)
    {
        // Replace Json::FastWriter with Json::StreamWriterBuilder

        // Json::FastWriter w;
        // w.omitEndingLineFeed();
        // qDebug() << QString::fromStdString(w.write(json));
        // return QString::fromStdString(w.write(json));

        Json::StreamWriterBuilder wbuilder;
        wbuilder["commentStyle"] = "None";
        wbuilder["indentation"] = "";
        stringstream str;
        str << Json::writeString(wbuilder, json) << endl;
        // qDebug() << QString::fromStdString(str.str());
        return QString::fromStdString(str.str());
    }
};

} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __HTT_UTILS_H__
