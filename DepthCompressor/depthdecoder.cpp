#include "depthdecoder.h"
#include <QImage>
#include <jpeg_encoder.h>

namespace DepthEncoder
{

    Decoder::Decoder(QString filePath)
    {
        // Load image data
        QImage src(filePath);
        src = src.convertToFormat(QImage::Format_RGB888);

        m_Data = std::vector<uint8_t>(src.bits(), src.bits() + src.width() * src.height() * 3);
        m_Width = src.width();
        m_Height = src.height();
    }

    void Decoder::Decode(QString outPath, EncodingMode mode)
    {
        QImage result;

        switch (mode)
        {
            case EncodingMode::NONE:
                result = DecodeNone();
                break;
            case EncodingMode::TRIANGLE:
                result = DecodeTriangle();
                break;
            case EncodingMode::MORTON:
                result = DecodeMorton();
                break;
            case EncodingMode::HILBERT:
                result = DecodeHilbert();
                break;
            default:
                break;
        }

        result.save(outPath);
    }

    QImage Decoder::DecodeNone() { return QImage();}

    QImage Decoder::DecodeMorton()
    {
        QImage outImage(m_Width, m_Height, QImage::Format_RGB888);

        const int nbits = CurveOrder<uint16_t>();

        for(uint32_t y = 0; y < m_Height; y++) {
            for(uint32_t x = 0; x < m_Width; x++) {
                int r = std::round(m_Data[x*3 + y * m_Width*3]);
                int g = std::round(m_Data[x*3 + y * m_Width*3 + 1]);
                int b = std::round(m_Data[x*3 + y * m_Width*3 + 2]);

                int codex = 0, codey = 0, codez = 0;

                const int nbits2 = 2 * nbits;

                for (int i = 0, andbit = 1; i < nbits2; i += 2, andbit <<= 1) {
                    codex |= (int)(r & andbit) << i;
                    codey |= (int)(g & andbit) << i;
                    codez |= (int)(b & andbit) << i;
                }

                float d = ((codez << 2) | (codey << 1) | codex) / 255.0f;
                outImage.setPixel(x, y, qRgb(d,d,d));
            }
        }

        return outImage;
    }

    QImage Decoder::DecodeHilbert()
    {
        return QImage();
    }

    QImage Decoder::DecodeTriangle()
    {
        QImage outImage(m_Width, m_Height, QImage::Format_RGB888);

        const int w = 65536;

        // Function data
        int np = 512;
        float p = (float)np / w;

        for(uint32_t y = 0; y < m_Height; y++) {
            for(uint32_t x = 0; x < m_Width; x++) {
                float Ld = m_Data[x*3 + y * m_Width*3] / 255.0;
                float Ha = m_Data[x*3 + y * m_Width*3 + 1] / 255.0;
                float Hb = m_Data[x*3 + y * m_Width*3 + 2] / 255.0;
                int m = (int)std::floor(4.0 * (Ld / p) - 0.5f) % 4;
                float L0 = Ld - (fmod( Ld - p/8.0f, p)) + (p/4.0) * m - p/8.0;
                float delta = 0;

                switch (m) {
                case 0:
                    delta = (p/2.0f) * Ha;
                    break;
                case 1:
                    delta = (p/2.0f) * Hb;
                    break;
                case 2:
                    delta = (p/2.0f) * (1.0f - Ha);
                    break;
                case 3:
                    delta = (p/2.0f) * (1.0f - Hb);
                    break;
                }

                float d = (L0 + delta);
                outImage.setPixel(x, y, qRgb(d*255,d*255,d*255));
            }
        }

        return outImage;
    }
}
