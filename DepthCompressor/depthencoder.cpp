#include "depthencoder.h"

#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>

#include <jpeg_encoder.h>

#include <iostream>
#include <sstream>

// Credits: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h
static void TransposeFromHilbertCoords(uint8_t* X, int nbits, int dim)
{
    uint32_t N = 2 << (nbits - 1), P, Q, t;

    // Gray decode by H ^ (H/2)
    t = X[dim - 1] >> 1;
    // Corrected error in Skilling's paper on the following line. The appendix had i >= 0 leading to negative array index.
    for (int i = dim - 1; i > 0; i--) X[i] ^= X[i - 1];
    X[0] ^= t;

    // Undo excess work
    for (Q = 2; Q != N; Q <<= 1) {
        P = Q - 1;
        for (int i = dim - 1; i >= 0; i--)
            if (X[i] & Q) // Invert
                X[0] ^= P;
            else { // Exchange
                t = (X[0] ^ X[i]) & P;
                X[0] ^= t;
                X[i] ^= t;
            }
    }
}

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

    void Encoder::Encode(const QString& outPath, const EncodingProperties& props)
    {
        std::vector<QImage> toSave;

        // Encode depending on selected mode
        switch (props.Mode)
        {
            case EncodingMode::NONE:
                toSave = EncodeNone(props.SplitChannels);
                break;
            case EncodingMode::TRIANGLE:
                toSave = EncodeTriangle(props.SplitChannels);
                break;
            case EncodingMode::MORTON:
                toSave = EncodeMorton(props.SplitChannels);
                break;
            case EncodingMode::HILBERT:
                toSave = EncodeHilbert(props.SplitChannels);
                break;
        }

        // Save results
        std::string labels[4] = {"", ".red", ".green", ".blue"};
        std::string extension;
        if (outPath.toLower().endsWith(".jpg") || outPath.toLower().endsWith(".jpeg"))
            extension = ".jpg";
        else
            extension = ".png";

        for (uint32_t i=0; i<std::min<uint32_t>(toSave.size(), 4); i++)
        {
            std::stringstream ss;
            ss << outPath.toStdString() << labels[i] << extension;
            SaveJPEG(QString(ss.str().c_str()), toSave[i], props.Quality);
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

    std::vector<QImage> Encoder::EncodeNone(bool splitChannels)
    {
        std::vector<QImage> ret;
        QImage img(m_Width, m_Height, QImage::Format_RGB32);
        QImage red(m_Width, m_Height, QImage::Format_RGB32);
        QImage green(m_Width, m_Height, QImage::Format_RGB32);
        QImage blue(m_Width, m_Height, QImage::Format_RGB32);

        for(uint32_t y = 0; y < m_Height; y++)
        {
            for(uint32_t x = 0; x < m_Width; x++)
            {
                float h = m_Data[x + y*m_Width];
                h = (h-m_Min)/(m_Max - m_Min);
                //quantize to 65k
                int height = floor(h*65535);
                int r = height/256;
                int g = height/256;
                int b = height/256;

                img.setPixel(x, y, qRgb(r, g, b));
                if (splitChannels)
                {
                    red.setPixel(x, y, r);
                    green.setPixel(x, y, g);
                    blue.setPixel(x, y, b);
                }
            }
        }

        ret.push_back(img);
        if (splitChannels)
        {
            ret.push_back(red);
            ret.push_back(green);
            ret.push_back(blue);
        }

        return ret;
    }

    std::vector<QImage> Encoder::EncodeTriangle(bool splitChannels)
    {
        std::vector<QImage> ret;
        QImage img(m_Width, m_Height, QImage::Format_RGB32);
        QImage red(m_Width, m_Height, QImage::Format_RGB32);
        QImage green(m_Width, m_Height, QImage::Format_RGB32);
        QImage blue(m_Width, m_Height, QImage::Format_RGB32);

        const int w = 65536;

        // Function data
        int np = 512;
        float p = (float)np / w;

        // Encode depth, save into QImage img
        for(uint32_t y = 0; y < m_Height; y++) {
            for(uint32_t x = 0; x < m_Width; x++) {
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

                if (splitChannels)
                {
                    red.setPixel(x, y, qRgb(Ld, Ld, Ld));
                    green.setPixel(x, y, qRgb(Ha, Ha, Ha));
                    blue.setPixel(x, y, qRgb(Hb, Hb, Hb));
                }
            }
        }

        ret.push_back(img);
        if (splitChannels)
        {
            ret.push_back(red);
            ret.push_back(green);
            ret.push_back(blue);
        }

        return ret;
    }

    // Encode high part in R channel, use morton to encode high precision values?
    std::vector<QImage> Encoder::EncodeMorton(bool splitChannels)
    {
        std::vector<QImage> ret;
        QImage img(m_Width, m_Height, QImage::Format_RGB32);
        QImage red(m_Width, m_Height, QImage::Format_RGB32);
        QImage green(m_Width, m_Height, QImage::Format_RGB32);
        QImage blue(m_Width, m_Height, QImage::Format_RGB32);

        const int w = 65535;

        // Encode depth, save into QImage img
        for(uint32_t y = 0; y < m_Height; y++) {
            for(uint32_t x = 0; x < m_Width; x++) {
                // Quantize depth
                float d = m_Data[x + y*m_Width];
                d = ((d-m_Min)/(m_Max - m_Min)) * w;

                // Convert to Morton coordinates
                std::vector<uint8_t> col = MortonToVec((uint16_t)std::round(d));

                img.setPixel(x, y, qRgb(col[0], col[1], col[2]));

                if (splitChannels)
                {
                    red.setPixel(x, y, qRgb(col[0], col[0], col[0]));
                    green.setPixel(x, y, qRgb(col[1], col[1], col[1]));
                    blue.setPixel(x, y, qRgb(col[2], col[2], col[2]));
                }
            }
        }

        ret.push_back(img);
        if (splitChannels)
        {
            ret.push_back(red);
            ret.push_back(green);
            ret.push_back(blue);
        }

        return ret;
    }

    std::vector<QImage> Encoder::EncodeHilbert(bool splitChannels)
    {
        std::vector<QImage> ret;
        QImage img(m_Width, m_Height, QImage::Format_RGB32);
        QImage red(m_Width, m_Height, QImage::Format_RGB32);
        QImage green(m_Width, m_Height, QImage::Format_RGB32);
        QImage blue(m_Width, m_Height, QImage::Format_RGB32);

        const int w = 65535;

        // Encode depth, save into QImage img
        for(uint32_t y = 0; y < m_Height; y++) {
            for(uint32_t x = 0; x < m_Width; x++) {
                // Quantize depth
                float d = m_Data[x + y*m_Width];
                d = ((d-m_Min)/(m_Max - m_Min)) * w;

                // Convert to Morton coordinates
                std::vector<uint8_t> col = HilbertToVec((uint16_t)std::round(d));

                img.setPixel(x, y, qRgb(col[0], col[1], col[2]));

                if (splitChannels)
                {
                    red.setPixel(x, y, qRgb(col[0], col[0], col[0]));
                    green.setPixel(x, y, qRgb(col[1], col[1], col[1]));
                    blue.setPixel(x, y, qRgb(col[2], col[2], col[2]));
                }
            }
        }

        ret.push_back(img);
        if (splitChannels)
        {
            ret.push_back(red);
            ret.push_back(green);
            ret.push_back(blue);
        }

        return ret;
    }

    // Credits: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h
    std::vector<uint8_t> Encoder::MortonToVec(uint16_t p)
    {
        const unsigned int nbits = std::ceil(CurveOrder<uint16_t>());
        std::vector<uint8_t> ret(3);
        ret[0] = 0; ret[1] = 0; ret[2] = 0;

        for (unsigned int i = 0; i <= nbits; ++i) {
            uint8_t selector = 1;
            unsigned int shift_selector = 3 * i;
            unsigned int shiftback = 2 * i;
            ret[0] |= (p & (selector << shift_selector)) >> (shiftback);
            ret[1] |= (p & (selector << (shift_selector + 1))) >> (shiftback + 1);
            ret[2] |= (p & (selector << (shift_selector + 2))) >> (shiftback + 2);
        }
        return ret;
    }

    // Credits: https://github.com/davemc0/DMcTools/blob/main/Math/SpaceFillCurve.h
    std::vector<uint8_t> Encoder::HilbertToVec(uint16_t p)
    {
        const int nbits = CurveOrder<uint16_t>();

        std::vector<uint8_t> v = MortonToVec(p);
        std::swap(v[0], v[2]);
        TransposeFromHilbertCoords(v.data(), nbits, 3);

        return v;
    }

    void Encoder::SaveJPEG(const QString& path, const QImage& sourceImage, uint32_t quality)
    {
        JpegEncoder encoder;
        uint8_t* retBuffer = new uint8_t[sourceImage.width() * sourceImage.height() * 3];
        unsigned long retSize;
        QImage src = sourceImage.convertToFormat(QImage::Format_RGB888);

        encoder.setJpegColorSpace(J_COLOR_SPACE::JCS_RGB);
        encoder.setQuality(quality);

        encoder.init(path.toStdString().c_str(), src.width(), src.height(), &retBuffer, &retSize);
        encoder.writeRows(src.bits(), src.height());
        encoder.finish();

        QFile out(path);
        out.open(QIODevice::WriteOnly);
        out.write((const char*)retBuffer, retSize);
        out.close();

        //delete[] retBuffer;
    }

}
