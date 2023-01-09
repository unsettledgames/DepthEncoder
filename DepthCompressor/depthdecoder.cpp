#include "depthdecoder.h"
#include <QImage>

#include <conversions.h>
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

    Decoder::Decoder(const uint8_t* data, uint32_t width, uint32_t height)
    {
        // Copy data
        m_Data = std::vector<uint8_t>(data, data + width * height * 3);
        m_Width = width;
        m_Height = height;
    }

    void Decoder::Decode(QString outPath, EncodingMode mode)
    {
        QImage result(m_Width, m_Height, QImage::Format_RGB888);
        std::vector<uint16_t> ret(m_Width * m_Height);

        Decode(ret, mode);

        for (uint32_t i=0; i<m_Width; i++)
        {
            for (uint32_t j=0; j<m_Height; j++)
            {
                float d = ret[j + i * m_Height] / 255.0f;
                result.setPixel(j, i, qRgb(d,d,d));
            }
        }
        result.save(outPath);
    }

    void Decoder::Decode(std::vector<uint16_t>& data, EncodingMode mode)
    {
        float depth = 0;
        int nbits = CurveOrder<uint16_t>();

        for(uint32_t y = 0; y < m_Height; y++)
        {
            for(uint32_t x = 0; x < m_Width; x++)
            {
                int r = m_Data[x*3 + y * m_Width*3];
                int g = m_Data[x*3 + y * m_Width*3 + 1];
                int b = m_Data[x*3 + y * m_Width*3 + 2];

                switch (mode)
                {
                    case EncodingMode::NONE:
                        depth = 0;
                        break;
                    case EncodingMode::TRIANGLE:
                        depth = GetTriangleCode(r, g, b);
                        break;
                    case EncodingMode::MORTON:
                        depth = GetMortonCode(r, g, b, nbits);
                        break;
                    case EncodingMode::HILBERT:
                        depth = GetHilbertCode(r, g, b, nbits);
                        break;
                    case EncodingMode::PHASE:
                        depth = GetPhaseCode(r, g, b);
                        break;
                    case EncodingMode::SPLIT:
                        depth = GetSplitCode(r, g, b);
                        break;
                    default:
                        break;
                }
                data[x + m_Width * y] = depth;
            }
        }
    }
}
