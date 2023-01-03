#ifndef DEPTHENCODER_H
#define DEPTHENCODER_H

#include <QString>
#include <vector>

#include <Defs.h>

class QImage;

namespace DepthEncoder
{
    struct EncodingProperties
    {
        EncodingMode Mode = TRIANGLE;
        uint32_t Quality = 90;
        bool SplitChannels = false;

        EncodingProperties() = default;
        EncodingProperties(const EncodingProperties& props) = default;
        EncodingProperties(EncodingMode mode, uint32_t quality, bool splitChannels) :
            Mode(mode), Quality(quality), SplitChannels(splitChannels) {}
    };

    class Encoder
    {
    public:
        Encoder(const QString& path);
        void Encode(const QString& outPath, const EncodingProperties& props);

    private:
        template <typename intcode_t> constexpr int CurveOrder() { return (sizeof(intcode_t) * 8) / 3; }
        void SaveJPEG(const QString& path, const QImage& img, uint32_t quality);

        std::vector<QImage> EncodeNone(bool splitChannels);
        std::vector<QImage> EncodeTriangle(bool splitChannels);
        std::vector<QImage> EncodeMorton(bool splitChannels);
        std::vector<QImage> EncodeHilbert(bool splitChannels);

        std::vector<uint8_t> MortonToVec(uint16_t point);
        std::vector<uint8_t> HilbertToVec(uint16_t point);

    private:
        QString m_Path;
        std::vector<float> m_Data;

        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_CellSize;
        float m_CellCenterX;
        float m_CellCenterY;

        float m_Min;
        float m_Max;
    };

}
#endif // DEPTHENCODER_H
