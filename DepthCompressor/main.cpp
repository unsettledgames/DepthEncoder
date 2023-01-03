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

int main(int argc, char *argv[])
{
    DepthEncoder::Encoder encoder(argv[1]);
    DepthEncoder::Decoder decoder("elevation3Morton.jpg.jpg");
    DepthEncoder::EncodingProperties props(DepthEncoder::EncodingMode::HILBERT, 100, false);

    encoder.Encode("elevation3Morton.jpg", props);
    decoder.Decode("decoded2.png", DepthEncoder::EncodingMode::HILBERT);

    // Save output images
    //dumpImage("slope.png", slopes, ncols-2, nrows-2);
    //dumpImage("aspect.png", aspects, ncols-2, nrows-2);
    return 0;
}
