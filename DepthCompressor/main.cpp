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

    for (int i=0; i<=65535; i++)
    {
        auto col = DepthEncoder::SplitToVec(i);
        int code = DepthEncoder::GetSplitCode(col[0], col[1], col[2]);
        float err = std::fabs(code - i);

        maxErr = std::max(err, maxErr);
        minErr = std::min(err, minErr);
        avgErr += err;
    }

    std::cout << "Min: " << minErr << ", max: " << maxErr << ", avg: " << avgErr / 65535.0f << std::endl;
}

int main(int argc, char *argv[])
{
    GeneratePics();

    /*
    DepthEncoder::Encoder encoder(argv[1]);
    DepthEncoder::EncodingProperties props(DepthEncoder::EncodingMode::SPLIT, 100, false);
    props.RemovedBits = 0;

    encoder.Encode("Split.jpg", props);

    DepthEncoder::Decoder decoder("Split.jpg");
    decoder.Decode("splitdecoded.png", DepthEncoder::EncodingMode::SPLIT);
*/
    return 0;
}
