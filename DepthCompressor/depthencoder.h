#ifndef DEPTHENCODER_H
#define DEPTHENCODER_H

#include <QString>
#include <vector>

class QImage;

class DepthEncoder
{
    enum EncodingMode {NONE = 0, TRIANGLE = 1, MORTON = 2, HILBERT = 3};

    struct Properties
    {
        EncodingMode Mode = TRIANGLE;
        uint32_t Quality = 90;
        bool SplitChannels = false;
    };

public:
    DepthEncoder(const QString& path);
    void Encode(const QString& outPath, const Properties& props);

private:
    void SaveJPEG(const QString& path, const QImage& img, const Properties& props);

private:
    QString m_Path;
    std::vector<float> m_Data;

    float m_Width;
    float m_Height;
    float m_CellSize;
    float m_CellCenterX;
    float m_CellCenterY;

    float m_Min;
    float m_Max;
};

#endif // DEPTHENCODER_H
