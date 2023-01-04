#include <QImage>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRgb>

#include <stdio.h>
#include <iostream>
#include <vector>
#include <fstream>

#include <cmath>
#include <math.h>

#include <depthencoder.h>
#include <depthdecoder.h>

/** TODO
 *  - Polish code, remove repetition, probably pass an encoding lambda to Encode instead of
 *      having multiple functions, or template Encode and Decode
 *  - Remove vector to represent a color, check if Qt has something, otherwise just def a vec3 class
 *  - Add error calculator
 *  - Add benchmark
 *  - Reduce QImage usage in favor of raw uchar data
 *
 *
 *  NOTES
 *  - R channel to encode main part, G and B to encode morton / hilbert?
 *
 */

using namespace std;

void GeneratePics()
{
    int size = 1000 * 1000;
    std::vector<float> data1(size), data2(size);
    std::vector<uint16_t> encoded1(size), encoded2(size), decoded1(size), decoded2(size);

    QImage img1(1000, 1000, QImage::Format_ARGB32);
    QImage img2(1000, 1000, QImage::Format_ARGB32);

    for (int i=0; i<1000; i++)
    {
        for (int j=0; j<1000; j++)
        {
            float val = 255 * (i / 1000.0f);
            float val2 = 255 * ((i + j) / 2000.0f);

            img1.setPixel(i, j, qRgb(val, val, val));
            img2.setPixel(i, j, qRgb(val2, val2, val2));
        }
    }

    img1 = img1.convertToFormat(QImage::Format_RGB888);
    img2 = img2.convertToFormat(QImage::Format_RGB888);

    for (uint32_t i=0; i<1000; i++)
    {
        for (uint32_t j=0; j<1000; j++)
        {
            data1[j + i*1000] = 65535 * (j / 1000.0f);
            data2[j + i*1000] = 65535 * (i / 1000.0f + j/1000.0f) / 2.0f;
        }
    }

    DepthEncoder::Encoder encoder1(data1.data(), 1000, 1000);
    DepthEncoder::Encoder encoder2(data2.data(), 1000, 1000);

    DepthEncoder::EncodingProperties props(DepthEncoder::EncodingMode::HILBERT, 100, false);

    //encoder1.Encode(encoded1, props);
   // encoder2.Encode(encoded2, props);

    DepthEncoder::Decoder artDecoder1("artEncoded1.png.png");
    DepthEncoder::Decoder artDecoder2("artEncoded2.png.png");

    artDecoder1.Decode(decoded1, DepthEncoder::EncodingMode::HILBERT);
    artDecoder2.Decode(decoded2, DepthEncoder::EncodingMode::HILBERT);

    img1.save("artificial1.jpg");
    img2.save("artificial2.jpg");
}

int main(int argc, char *argv[])
{
    //GeneratePics();

    DepthEncoder::Encoder encoder(argv[1]);
    DepthEncoder::EncodingProperties props(DepthEncoder::EncodingMode::PHASE, 100, true);
    encoder.Encode("Phase.jpg", props);

    DepthEncoder::Decoder decoder("Phase.jpg");
    decoder.Decode("decodedlq.png", DepthEncoder::EncodingMode::PHASE);

    return 0;
}
