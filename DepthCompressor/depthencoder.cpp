#include "depthencoder.h"

#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>

#include <jpeg_encoder.h>

#include <iostream>

DepthEncoder::DepthEncoder(const QString& path) : m_Path(path)
{
    if (!QFile::exists(path))
    {
        std::cerr << "File " << path.toStdString() << " does not exist, couldn't create a decoder." << std::endl;
        return;
    }

    // Load asc data
    if (path.toLower().endsWith(".asc"))
    {
        FILE* fp = fopen(path.toStdString().c_str(), "rb");

        if(!fp)
        {
            std::cerr << "Could not open: " << path.toStdString() << std::endl;
            return;
        }

        fscanf(fp, "ncols %d\n", &m_Width);
        fscanf(fp, "nrows %d\n", &m_Height);
        fscanf(fp, "xllcenter %f\n", &m_CellCenterX);
        fscanf(fp, "yllcenter %f\n", &m_CellCenterY);
        fscanf(fp, "cellsize %f\n", &m_CellSize);
        //fscanf(fp, "nodata_value %f\n", &nodata);

        // Read heights
        m_Data.resize(m_Width * m_Height);
        m_Min = 1e20;
        m_Max = -1e20;

        for(int i = 0; i < m_Width*m_Height; i++)
        {
            float h;
            fscanf(fp, "%f", &h);
            m_Min = std::min(m_Min, h);
            m_Max = std::max(m_Max, h);
            m_Data[i] = h;
        }
    }
}

void DepthEncoder::Encode(const QString& outPath, const Properties& props)
{
    //highp float val = texture2d(samplerTop8bits, tex_coord) * (256.0 / 257.0);
    //val += texture2d(samplerBottom8bits, tex_coord) * (1.0 / 257.0);
    QImage img(m_Width, m_Height, QImage::Format_RGB32);
    QImage red(m_Width, m_Height, QImage::Format_RGB32);
    QImage green(m_Width, m_Height, QImage::Format_RGB32);
    QImage blue(m_Width, m_Height, QImage::Format_RGB32);

    const int w = 65536;

    // Function data
    int np = 512;
    float p = (float)np / w;

    // Encode depth, save into QImage img
    for(int y = 0; y < m_Height; y++) {
        for(int x = 0; x < m_Width; x++) {
            float Ld, Ha, Hb;

            // Quantize depth
            float d = m_Data[x + y*m_Width];
            d = ((d-m_Min)/(m_Max - m_Min)) * w;

            Ld = (d + 0.5) / w;

            float mod = fmod(Ld / (p/2.0f), 2.0f);
            if (mod <= 1)
                Ha = mod;
            else
                Ha = 2 - mod;

            float mod2 = fmod((Ld - p/4.0f) / (p/2.0f), 2.0f);
            if (mod2 <= 1)
                Hb = mod2;
            else
                Hb = 2 - mod2;

            Ld *= 255; Ha *= 255; Hb *= 255;
            img.setPixel(x, y, qRgb(Ld, Ha, Hb));

            if (props.SplitChannels)
            {
                red.setPixel(x, y, qRgb(Ld, Ld, Ld));
                green.setPixel(x, y, qRgb(Ha, Ha, Ha));
                blue.setPixel(x, y, qRgb(Hb, Hb, Hb));
            }
        }
    }

    // Encode to jpeg if needed
    if (outPath.toLower().endsWith(".jpg") || outPath.toLower().endsWith(".jpeg"))
        SaveJPEG(outPath, img, props);
    // Otherwise just save everything
    else
    {
        img.save(outPath);

        if (props.SplitChannels)
        {
            red.save(outPath + "red.png");
            green.save(outPath + "green.png");
            blue.save(outPath + "blue.png");
        }
    }

    QFile info("info.json");
    if(!info.open(QFile::WriteOnly))
        throw "Failed writing info.json";

    QJsonDocument doc;
    QJsonObject obj;

    obj.insert("width", QJsonValue::fromVariant(m_Width));
    obj.insert("height", QJsonValue::fromVariant(m_Height));
    obj.insert("type", QJsonValue::fromVariant("dem"));
    obj.insert("p", QJsonValue::fromVariant(p));
    obj.insert("cellsize", QJsonValue::fromVariant(m_CellSize));
    obj.insert("min", QJsonValue::fromVariant(m_Min));
    obj.insert("max", QJsonValue::fromVariant(m_Max));

    doc.setObject(obj);
    QTextStream stream(&info);
    stream << doc.toJson();
}

void SaveJPEG(const QString& path, const QImage& img, const Properties& props);
{
    QImage sourceImage(src);
    JpegEncoder encoder;
    uint8_t* retBuffer = new uint8_t[sourceImage.width() * sourceImage.height() * 3];
    unsigned long retSize;
    sourceImage = sourceImage.convertToFormat(QImage::Format_RGB888);

    encoder.setJpegColorSpace(J_COLOR_SPACE::JCS_RGB);
    encoder.setQuality(compression);

    encoder.init(dest.toStdString().c_str(), sourceImage.width(), sourceImage.height(), &retBuffer, &retSize);
    encoder.writeRows(sourceImage.bits(), sourceImage.height());
    encoder.finish();

    QFile out(dest);
    out.open(QIODevice::WriteOnly);
    out.write((const char*)retBuffer, retSize);
    out.close();
}
