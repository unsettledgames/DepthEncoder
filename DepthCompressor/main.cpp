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

#include <jpeg_encoder.h>
#include <jpeg_decoder.h>

using namespace std;

// TODO: hilbert & morton
void dumpImageQuantized(QString name, std::vector<float> &buffer, int width, int height, double cellsize)
{
    float min = 1e20;
    float max = -1e20;
    for(float f: buffer) {
        min = std::min(f, min);
        max = std::max(f, max);
    }

    cout << "Min: " << min << " max:" << max << " steps: " << (max - min)/0.01 << endl;

    //highp float val = texture2d(samplerTop8bits, tex_coord) * (256.0 / 257.0);
    //val += texture2d(samplerBottom8bits, tex_coord) * (1.0 / 257.0);

    QImage img(width, height, QImage::Format_RGB32);

    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            float h = buffer[x + y*width];
            h = (h-min)/(max - min);
            //quantize to 65k
            int height = floor(h*65535);
            int r = height/256;
            int g = height/256;
            int b = height/256;
            img.setPixel(x, y, qRgb(r, g, b));
        }
    }
    img.save(name);
    QFile info("info.json");
    if(!info.open(QFile::WriteOnly))
        throw "Failed writing info.json";

    QTextStream stream(&info);
    stream << "{\n"
    << "width: " << width << ",\n"
    << "heigh: " << height << ",\n"
    << "type: \"dem\"\n"
    << "cellsize: " << cellsize << ",\n"
    << "min: " << min << ",\n"
    << "max: " << max << "\n"
    << "}\n";
}


void dumpImageJPEG(QString src, QString dest, int compression)
{
    QImage sourceImage(src);
    JpegEncoder encoder;
    uint8_t* retBuffer = new uint8_t[sourceImage.width() * sourceImage.height() * 3];
    unsigned long retSize;
    sourceImage = sourceImage.convertToFormat(QImage::Format_RGB888);

    encoder.setJpegColorSpace(J_COLOR_SPACE::JCS_RGB);
    encoder.setQuality(compression);

    encoder.init(dest.toStdString().c_str(), sourceImage.width(), sourceImage.height(), &retBuffer, &retSize);
    encoder.writeRows(sourceImage.bits(), sourceImage.height());
    encoder.finish();

    QFile out(dest);
    out.open(QIODevice::WriteOnly);
    out.write((const char*)retBuffer, retSize);
    out.close();
}

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
    bool decode = false;
    bool encodeJpeg = true;

    if (encodeJpeg) {
        dumpImageJPEG("elevation3.png", "elevation3compressed.jpg", 50);
        return 0;
    }

    if (decode) {
        decodeImage("elevation3.png", "info.json");
        return 0;
    }

    // Save output images
    //dumpImage("slope.png", slopes, ncols-2, nrows-2);
    //dumpImage("aspect.png", aspects, ncols-2, nrows-2);
    dumpImageQuantized("elevation_quantized.png", heights, ncols, nrows, cellsize);

    return 0;
}
