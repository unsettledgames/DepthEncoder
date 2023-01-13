#include <Parser.h>
#include <Compressor.h>
#include <Writer.h>
#include <Algorithms.h>

#include <QImage>
#include <iostream>
#include <vector>

#define N_TO_TEST   5
#define N_TEST_PICS 1

using namespace DStream;

void ImageBenchmark()
{
    EncodingType encodingTypes[N_TO_TEST] = {EncodingType::TRIANGLE, EncodingType::SPLIT,
            EncodingType::MORTON, EncodingType::HILBERT, EncodingType::PHASE};
    std::string encodingLabels[N_TO_TEST] = {"Triangle", "Split", "Morton", "Hilbert", "Phase"};
    uint32_t jpegQualityDecrease = 20;
    uint32_t minJpegQuality = 20;
    uint32_t maxJpegQuality = 100;

    std::string testImages[N_TEST_PICS] = {"envoi_RTI/MNT.asc"};

    // Test per image, then try all the algos on it

    for (uint32_t i=0; i<N_TO_TEST; i++)
    {
        std::cout << "TESTING ALGORITHM " << encodingLabels[i] << std::endl;

        for (uint32_t j=0; j<N_TEST_PICS; j++)
        {
            std::cout << "TESTING IMAGE " << testImages[j] << std::endl;

            for (uint32_t q=maxJpegQuality; q>=minJpegQuality; q-=jpegQualityDecrease)
            {
                std::cout << "JPEG QUALITY: " << q << std::endl;

                // Get heightmap data
                Parser parser("envoi_RTI/MNT.asc", InputFormat::ASC);
                DepthmapData dmData;
                Writer writer("tmp.jpg");
                uint16_t* heightData = parser.Parse(dmData);

                std::vector<uint16_t> decodedData(dmData.Width * dmData.Height);
                std::vector<uint8_t> encodedData(3 * dmData.Width * dmData.Height);

                // Encode
                Compressor compressor(dmData.Width, dmData.Height);
                compressor.Encode(heightData, encodedData.data(), dmData.Width * dmData.Height, encodingTypes[i]);
                writer.Write(encodedData.data(), dmData.Width, dmData.Height, OutputFormat::JPG, false, q);

                // Read encoded image
                QImage img("tmp.jpg");
                img = img.convertToFormat(QImage::Format_RGB888);
                memcpy(encodedData.data(), img.bits(), 3 * dmData.Width * dmData.Height);

                // Decode
                compressor.Decode(encodedData.data(), decodedData.data(), dmData.Width * dmData.Height, encodingTypes[i]);

                // Measure error
                float max = -1e20;
                float avg = 0;

                for (uint32_t s=0; s<dmData.Width * dmData.Height; s++)
                {
                    float err = std::abs(heightData[s] - decodedData[s]);
                    max = std::max<float>(err, max);
                    avg += err;
                }
                avg /= dmData.Width * dmData.Height;

                std::cout << "Max error: " << max << "\nAverage error: " << avg << std::endl;
            }
        }
    }
}

void RangeBenchmark()
{
    EncodingType modes[N_TO_TEST] = {EncodingType::HILBERT,EncodingType::MORTON,
                                              EncodingType::PHASE, EncodingType::TRIANGLE,
                                              EncodingType::SPLIT};

    std::string labels[N_TO_TEST] = {"Hilbert", "Morton", "Phase", "Triangle", "Split"};

    for (int t=0; t<N_TO_TEST; t++)
    {
        std::cout << "TESTING " << labels[t] << std::endl;
        float maxErr = -1e20;
        float avgErr = 0;

        for (int i=0; i<=65535; i++)
        {
            Color encoded;

            switch (modes[t])
            {
            case EncodingType::TRIANGLE:
                encoded = TriangleToColor(i);
                break;
            case EncodingType::MORTON:
                encoded = MortonToColor(i, CurveOrder<uint16_t>());
                break;
            case EncodingType::HILBERT:
                encoded = HilbertToColor(i, CurveOrder<uint16_t>());
                break;
            case EncodingType::PHASE:
                encoded = PhaseToColor(i);
                break;
            case EncodingType::SPLIT:
                encoded = SplitToColor(i);
                break;
            }

            uint16_t decoded;

            switch (modes[t])
            {
            case EncodingType::TRIANGLE:
                decoded = ColorToTriangle(encoded);
                break;
            case EncodingType::MORTON:
                decoded = ColorToMorton(encoded, CurveOrder<uint16_t>());
                break;
            case EncodingType::HILBERT:
                decoded = ColorToHilbert(encoded, CurveOrder<uint16_t>());
                break;
            case EncodingType::PHASE:
                decoded = ColorToPhase(encoded);
                break;
            case EncodingType::SPLIT:
                decoded = ColorToSplit(encoded);
                break;
            default:
                break;
            }

            float err = std::abs(decoded - i);
            avgErr += err;
            maxErr = std::max(err, maxErr);
        }

        std::cout << "Max error: " << maxErr << ", average error: " << avgErr / 65535.0f << std::endl;
    }
}

void RemoveNoise(const std::string& fileName, uint32_t width, uint32_t height)
{
    QImage input(QString(fileName.c_str()));
    QImage output(width, height, QImage::Format_RGB888);
    input = input.convertToFormat(QImage::Format_RGB888);

    for (uint32_t y=0; y<height; y++)
    {
        for (uint32_t x=0; x<width; x++)
        {
            if (y != 0 && x != 0 && x != (width-1) && y != (height-1))
            {
                int current = qRed(input.pixel(x, y));
                // Get all neighbors
                std::vector<int> neighbors;
                neighbors.push_back(qRed(input.pixel(x, y + 1)));
                neighbors.push_back(qRed(input.pixel(x, y-1)));
                neighbors.push_back(qRed(input.pixel(x-1, y)));
                neighbors.push_back(qRed(input.pixel(x+1, y)));

                // Compute distance matrix
                int distanceMatrix[4][4];
                int outlierThreshold = 32;

                // Remove the neihbor with the max distance from the other ones, if said distance is more than the threshold
                for (int i=0; i<4; i++)
                    for (int j=0; j<4; j++)
                        distanceMatrix[i][j] = std::abs(neighbors[i] - neighbors[j]);

                bool outlier = false;
                // Check if there's an outlier
                for (uint32_t i=0; i<4; i++)
                    if (distanceMatrix[i][0] > outlierThreshold)
                        outlier = true;

                // Find the right outlier (for each neighbor, return the one with the biggest row + column sum)
                int maxSum = -1;
                int maxIdx = -1;
                if (outlier)
                {
                    for (uint32_t i=0; i<4; i++)
                    {
                        // Compute row + column
                        int row = 0, col = 0;
                        for (uint32_t k=0; k<4; k++)
                        {
                            row += distanceMatrix[i][k];
                            col += distanceMatrix[k][i];
                        }

                        if ((row + col) > maxSum)
                        {
                            maxIdx = i;
                            maxSum = row + col;
                        }
                    }
                }


                // Compute the average withouit maxIdx
                float avg = 0; float divisor = maxIdx == -1 ? 4 : 3;
                for (uint32_t i=0; i<4; i++)
                    if (i != maxIdx)
                        avg += neighbors[i];
                avg /= divisor;

                float err = std::abs(current - avg);

                if (err > 8)
                    output.setPixel(x, y, qRgb(avg, avg, avg));
                else
                    output.setPixel(x, y, qRgb(current, current, current));
            }
        }
    }

    output.save(QString((fileName + "polished.png").c_str()));
}

int main(int argc, char *argv[])
{
    //ImageBenchmark();
    //RangeBenchmark();

    uint16_t* data = nullptr;
    DepthmapData dmData;
    Writer writer("encoded.jpg");
    Parser parser("envoi_RTI/MNT.asc", InputFormat::ASC);

    data = parser.Parse(dmData);

    std::vector<uint8_t> compressed(dmData.Width * dmData.Height * 3);
    Compressor compressor(dmData.Width, dmData.Height);

    compressor.Encode(data, compressed.data(), dmData.Width * dmData.Height, EncodingType::HILBERT);
    writer.Write(compressed.data(), dmData.Width, dmData.Height, OutputFormat::JPG, false, 80);

    // Load encoded image
    QImage img("encoded.jpg");
    img = img.convertToFormat(QImage::Format_RGB888);
    memcpy(compressed.data(), img.bits(), 3 * dmData.Width * dmData.Height);

    // Decode it
    compressor.Decode(compressed.data(), data, dmData.Width * dmData.Height, EncodingType::HILBERT);

    writer.SetPath("decoded.png");
    writer.Write(data, dmData.Width, dmData.Height, false);

    RemoveNoise("decoded.png", dmData.Width, dmData.Height);

    delete[] data;
    return 0;
}
