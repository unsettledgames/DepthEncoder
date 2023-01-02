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

/** TODO
 *  - Write encoding type in json file
 *  - Add hilbert / morton
 *  - Add decoder
 *  - Add error calculator
 */

using namespace std;

void decodeImage(QString filePath, QString jsonPath) {
    QJsonDocument doc;
    QFile jsonFile(jsonPath);
    jsonFile.open(QIODevice::ReadOnly);
    auto content = jsonFile.readAll();
    string contentStr = content.toStdString();
    doc.fromJson(content);
    cout << contentStr << endl;

    QJsonObject object = doc.object();
    int width = 1000;object.value("width").toInt();
    int height = 1000;object.value("height").toInt();
    float max = 95.56;
    float min = 89.11;

    ofstream out("decoded.txt");
    QImage in(filePath);
    QImage outImage(in.width(), in.height(), QImage::Format_RGB32);
    const int w = 65536;

    // Function data
    int np = 512;
    float p = (float)np / w;

    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            QRgb c = in.pixel(x,y);
            float Ld = qRed(c) / 255.0, Ha = qGreen(c) / 255.0, Hb = qBlue(c) / 255.0;
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
            out << d * (max - min) + min << " ";
            outImage.setPixel(x, y, qRgb(d*255,d*255,d*255));
        }
        out << endl;
    }

    outImage.save(filePath + ".decoded.png");
}

using namespace std;
int main(int argc, char *argv[])
{
    DepthEncoder::Encoder encoder(argv[1]);
    DepthEncoder::EncodingProperties props(DepthEncoder::EncodingMode::MORTON, 100, false);

    encoder.Encode("elevation3Morton.jpg", props);

    bool decode = false;

    if (decode) {
        decodeImage("elevation3.png", "info.json");
        return 0;
    }

    // Save output images
    //dumpImage("slope.png", slopes, ncols-2, nrows-2);
    //dumpImage("aspect.png", aspects, ncols-2, nrows-2);
    return 0;
}
