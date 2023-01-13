#include <Writer.h>
#include <jpeg_encoder.h>

#include <QString>
#include <QImage>
#include <QFile>

namespace DStream
{
    Writer::Writer(const std::string& path) : m_OutputPath(path) {}

    bool Writer::Write(uint8_t* data, uint32_t width, uint32_t height, OutputFormat format,
                       bool splitChannels/* = false*/, uint32_t quality/* = 100*/)
    {
        uint8_t* encodedData = new uint8_t[width * height * 3];
        ulong retSize;
        JpegEncoder encoder;

        encoder.setJpegColorSpace(J_COLOR_SPACE::JCS_RGB);
        encoder.setQuality(quality);

        encoder.init(width, height, &encodedData, &retSize);
        encoder.writeRows(data, height);
        encoder.finish();

        QFile out(QString(m_OutputPath.c_str()));
        out.open(QIODevice::WriteOnly);
        out.write((const char*)encodedData, retSize);
        out.close();

        return true;
    }

    bool Writer::Write(uint16_t* data, uint32_t width, uint32_t height)
    {
        QImage out(width, height, QImage::Format_RGB888);

        for (uint32_t y=0; y<height; y++)
        {
            for (uint32_t x=0; x<width; x++)
            {
                float val = 255.0f * (data[x + y*width] / 65535.0f);
                out.setPixel(x, y, qRgb(val, val, val));
            }
        }

        out.save(QString(m_OutputPath.c_str()));
        return true;
    }
        /*

        SaveJPEG(QString(m_OutputPath.c_str()), data, quality);
        // Save json data
        QFile info("info.json");
        if(!info.open(QFile::WriteOnly))
            throw "Failed writing info.json";

        QJsonDocument doc;
        QJsonObject obj;

        obj.insert("width", QJsonValue::fromVariant(m_Width));
        obj.insert("height", QJsonValue::fromVariant(m_Height));
        obj.insert("type", QJsonValue::fromVariant("dem"));
        obj.insert("encoding", QJsonValue::fromVariant(EncodingModeToStr(props.Mode)));
        // TODO: the result of an Encode function is a structure containing a set of images and additional
        // properties to be saved in form of a QJsonObject
        //obj.insert("p", QJsonValue::fromVariant(p));
        obj.insert("cellsize", QJsonValue::fromVariant(m_CellSize));
        obj.insert("min", QJsonValue::fromVariant(m_Min));
        obj.insert("max", QJsonValue::fromVariant(m_Max));

        doc.setObject(obj);
        QTextStream stream(&info);
        stream << doc.toJson();

    }
    void Writer::SaveJPEG(const QString& path, const uint8_t* data, uint32_t quality)
    {
        JpegEncoder encoder;
        uint8_t* retBuffer = new uint8_t[sourceImage.width() * sourceImage.height() * 3];
        unsigned long retSize;
        QImage src = sourceImage.convertToFormat(QImage::Format_RGB888);

        encoder.setJpegColorSpace(J_COLOR_SPACE::JCS_RGB);
        encoder.setQuality(quality);

        encoder.init(src.width(), src.height(), &retBuffer, &retSize);
        encoder.writeRows(src.bits(), src.height());
        encoder.finish();

        QFile out(path);
        out.open(QIODevice::WriteOnly);
        out.write((const char*)retBuffer, retSize);
        out.close();

        //delete[] retBuffer;
    }
     */
}
