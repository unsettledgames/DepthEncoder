#include "depthencoder.h"

#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>

#include <defs.h>
#include <conversions.h>
#include <jpeg_encoder.h>

#include <iostream>
#include <sstream>

static QString EncodingModeToStr(DepthEncoder::EncodingMode mode)
{
    if (mode == DepthEncoder::EncodingMode::HILBERT) return "Hilbert";
    else if (mode == DepthEncoder::EncodingMode::MORTON) return "Morton";
    else if (mode == DepthEncoder::EncodingMode::TRIANGLE) return "Triangle";
    else return "None";
}

namespace DepthEncoder
{
    Encoder::Encoder(const QString& path) : m_Path(path)
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

            float nodata;
            fscanf(fp, "ncols %d\n", &m_Width);
            fscanf(fp, "nrows %d\n", &m_Height);
            fscanf(fp, "xllcenter %f\n", &m_CellCenterX);
            fscanf(fp, "yllcenter %f\n", &m_CellCenterY);
            fscanf(fp, "cellsize %f\n", &m_CellSize);
            fscanf(fp, "nodata_value %f\n", &nodata);

            // Load depth data
            m_Data.resize(m_Width * m_Height);
            m_Min = 1e20;
            m_Max = -1e20;

            for(uint32_t i = 0; i < m_Width*m_Height; i++)
            {
                float h;
                fscanf(fp, "%f", &h);
                m_Min = std::min(m_Min, h);
                m_Max = std::max(m_Max, h);
                m_Data[i] = h;
            }
        }
    }

    Encoder::Encoder(const float* data, uint32_t width, uint32_t height)
    {
        uint32_t size = width * height;
        m_Width = width;
        m_Height = height;
        m_Min = 1e20;
        m_Max = -1e20;
        m_Data.resize(size);

        for (uint32_t i=0; i<size; i++)
        {
            m_Data[i] = data[i];
            m_Min = std::min(m_Min, m_Data[i]);
            m_Max = std::max(m_Max, m_Data[i]);
        }
    }

    void Encoder::Encode(const QString& outPath, const EncodingProperties& props)
    {
        std::vector<uint8_t> data(m_Width * m_Height * 3);
        QImage img(m_Width, m_Height, QImage::Format_RGB32);
        QImage splitChannels[3] = {QImage(), QImage(), QImage()};
        for (uint32_t i=0; i<3; i++)
            splitChannels[i] = QImage(m_Width, m_Height, QImage::Format_RGB32);

        // Encode data
        Encode(data, props);

        for (uint32_t y=0; y<m_Height; y++)
        {
            for (uint32_t x=0; x<m_Width; x++)
            {
                uint32_t idx = x*3 + y*m_Width*3;
                QRgb col = qRgb(data[idx], data[idx+1], data[idx+2]);

                // TODO: split channels
                img.setPixel(x, y, col);
                if (props.SplitChannels)
                    for (uint32_t i=0; i<3; i++)
                        splitChannels[i].setPixel(x, y, col);
            }
        }

        // Save results
        std::string labels[4] = {"", ".red", ".green", ".blue"};
        std::string extension;
        if (outPath.toLower().endsWith(".jpg") || outPath.toLower().endsWith(".jpeg"))
            extension = ".jpg";
        else
            extension = ".png";

        if (extension == ".jpg")
            SaveJPEG(outPath, img, props.Quality);
        else
            img.save(outPath);

        if (props.SplitChannels)
        {
            for (uint32_t i=0; i<3; i++)
            {
                std::stringstream ss;
                ss << outPath.toStdString() << labels[i] << extension;
                if (extension == ".jpg")
                    SaveJPEG(QString(ss.str().c_str()), splitChannels[i], props.Quality);
                else
                    splitChannels[i].save(QString(ss.str().c_str()));
            }
        }

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

    void Encoder::Encode(std::vector<uint8_t>& vec, const EncodingProperties &props)
    {
        std::vector<uint8_t> col;
        float d;

        for(uint32_t y = 0; y < m_Height; y++)
        {
            for(uint32_t x = 0; x < m_Width; x++)
            {
                d = m_Data[x + y*m_Width];
                d = DC_W * ((d-m_Min)/(m_Max - m_Min));

                switch (props.Mode)
                {
                    case EncodingMode::NONE:
                    {
                        uint8_t r = d/255;
                        uint8_t g = d/255;
                        uint8_t b = d/255;

                        col = {r,g,b};
                        break;
                    }
                    case EncodingMode::TRIANGLE:
                        col = TriangleToVec(d);
                        break;
                    case EncodingMode::MORTON:
                        col = MortonToVec(d);
                        break;
                    case EncodingMode::HILBERT:
                        col = HilbertToVec(d);
                        break;
                    case EncodingMode::PHASE:
                        col = PhaseToVec(d);
                        break;
                    default:
                        break;
                }

                vec[x*3 + y*m_Height*3 + 0] = col[0];
                vec[x*3 + y*m_Height*3 + 1] = col[1];
                vec[x*3 + y*m_Height*3 + 2] = col[2];
            }
        }
    }

    void Encoder::SaveJPEG(const QString& path, const QImage& sourceImage, uint32_t quality)
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

}
