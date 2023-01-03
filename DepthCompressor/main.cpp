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
 *  - Add error calculator
 *  - Add benchmark
 *  - Reduce QImage usage in favor of raw uchar data
 *
 *  BUGS
 *  - Left part of the depthmap is inverted
 *
 *  NOTES
 *  - Not all bits are used, 3 bits are free in case of 8bit components / 16bit depth
 *  - R channel to encode main part, G and B to encode morton / hilbert?
 *  - Compression for Hilbert and Morton may pass through YUV conversion
 *
 */

using namespace std;

void GeneratePics()
{
    std::vector<uint8_t> data1;
    std::vector<uint8_t> data2;

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

    data1 = std::vector<uint8_t>(img1.bits(), img1.bits() + 1000 * 1000 * 3);
    data2 = std::vector<uint8_t>(img2.bits(), img2.bits() + 1000 * 1000 * 3);

    DepthEncoder::Encoder encoder1(img1.bits(), 1000, 1000);
    DepthEncoder::Encoder encoder2(img2.bits(), 1000, 1000);

    DepthEncoder::EncodingProperties props(DepthEncoder::EncodingMode::HILBERT, 100, false);

    encoder1.Encode("artEncoded1.png", props);
    encoder2.Encode("artEncoded2.png", props);

    DepthEncoder::Decoder artDecoder1("artEncoded1.png.png");
    DepthEncoder::Decoder artDecoder2("artEncoded2.png.png");

    artDecoder1.Decode("artDecoded.png", DepthEncoder::EncodingMode::HILBERT);
    artDecoder2.Decode("artDecoded2.png", DepthEncoder::EncodingMode::HILBERT);

    img1.save("artificial1.jpg");
    img2.save("artificial2.jpg");
}

int main(int argc, char *argv[])
{
    GeneratePics();

    DepthEncoder::Encoder encoder(argv[1]);
    DepthEncoder::EncodingProperties props(DepthEncoder::EncodingMode::HILBERT, 100, false);
    encoder.Encode("Hilbert.jpg", props);

    DepthEncoder::Decoder decoder("Hilbert.jpg.jpg");
    decoder.Decode("decoded2.png", DepthEncoder::EncodingMode::HILBERT);


    return 0;
}
