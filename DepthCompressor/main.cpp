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

#include <conversions.h>
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
    DepthEncoder::EncodingMode modes[4] = {DepthEncoder::EncodingMode::HILBERT,DepthEncoder::EncodingMode::MORTON,
                                          DepthEncoder::EncodingMode::PHASE, DepthEncoder::EncodingMode::TRIANGLE};
    QString labels[4] = {"Hilbert", "Morton", "Phase", "Triangle"};
    std::ofstream out;
    out.open("output.txt");

    float maxErr = -1e20;
    float minErr = 1e20;
    float avgErr = 0;

    for (int i=0; i<65535; i++)
    {
        auto col = DepthEncoder::PhaseToVec(i);
        int code = DepthEncoder::GetPhaseCode(col[0], col[1], col[2]);
        float err = std::fabs(code - i);
        maxErr = std::max(err, maxErr);
        minErr = std::min(err, minErr);
        avgErr += err;

        if (maxErr > 65535)
            std::cout << "err";
    }

    std::cout << "Min: " << minErr << ", max: " << maxErr << ", avg: " << avgErr / 65535.0f << std::endl;


    for (uint32_t m=0; m<4; m++)
    {
        int size = 1000 * 1000;
        std::vector<float> data1(size), data2(size);
        std::vector<uint16_t> decoded1(size), decoded2(size);

        for (uint32_t i=0; i<1000; i++)
        {
            for (uint32_t j=0; j<1000; j++)
            {
                data1[j + i*1000] = std::floor((65535 * j) / 1000.0f);
                data2[j + i*1000] = std::floor((65535 * (i + j)) / 2000.0f);
            }
        }

        DepthEncoder::Encoder encoder1(data1.data(), 1000, 1000);
        DepthEncoder::Encoder encoder2(data2.data(), 1000, 1000);

        DepthEncoder::EncodingProperties props(modes[m], 100, false);

        encoder1.Encode("artencoded1.png", props);
        encoder2.Encode("artencoded2.png", props);

        DepthEncoder::Decoder artDecoder1("artencoded1.png");
        DepthEncoder::Decoder artDecoder2("artencoded2.png");

        artDecoder1.Decode(decoded1, modes[m]);
        artDecoder2.Decode(decoded2, modes[m]);

        /*
        QImage out1(1000, 1000, QImage::Format_RGB888);
        QImage out2(1000, 1000, QImage::Format_RGB888);
        for (uint32_t i=0; i<1000; i++)
        {
            for (uint32_t j=0; j<1000; j++)
            {
                uint32_t idx = j + i * 1000;
                out1.setPixel(j, i, qRgb(decoded1[idx]/255.0f,decoded1[idx+1]/255.0f,decoded1[idx+2]/255.0f));
                out2.setPixel(j, i, qRgb(decoded2[idx]/255.0f,decoded2[idx+1]/255.0f,decoded2[idx+2]/255.0f));
            }
        }
        out1.save("artdecoded1" + labels[m] + ".png");
        out2.save("artdecoded2" + labels[m] + ".png");
        */

        out << "MODE: " << labels[m].toStdString() << "\n";

        float maxErr = -1e20;
        float minErr = 1e20;
        float avgErr = 0;

        for (uint32_t i=0; i<decoded1.size(); i++)
        {
            float err = std::fabs(decoded1[i] - data1[i]);
            maxErr = std::max(err, maxErr);
            minErr = std::min(err, minErr);
            avgErr += err;
        }
        avgErr /= (1000 * 1000);

        out << "LEFT TO RIGHT" << std::endl;
        out << "Min: " << minErr << "\nMax: " << maxErr << "\nAvg: " << avgErr << std::endl << std::endl;

        maxErr = 1e-20;
        minErr = 1e20;
        avgErr = 0;


        for (uint32_t i=0; i<decoded2.size(); i++)
        {
            float err = std::fabs(decoded2[i] - data2[i]);
            maxErr = std::max(err, maxErr);
            minErr = std::min(err, minErr);
            avgErr += err;
        }
        avgErr /= (1000 * 1000);
        out << "DIAGONAL" << std::endl;
        out << "Min: " << minErr << "\nMax: " << maxErr << "\nAvg: " << avgErr << std::endl;
        out << std::endl << "---------------------------" << std::endl << std::endl;
    }

    out.flush();
    out.close();
}

int main(int argc, char *argv[])
{
    GeneratePics();

    DepthEncoder::Encoder encoder(argv[1]);
    DepthEncoder::EncodingProperties props(DepthEncoder::EncodingMode::PHASE, 100, true);
    encoder.Encode("Phase.jpg", props);

    DepthEncoder::Decoder decoder("Phase.jpg");
    decoder.Decode("decodedlq.png", DepthEncoder::EncodingMode::PHASE);

    return 0;
}
