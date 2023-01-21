#include <Parser.h>
#include <Writer.h>

#include <HilbertCoder.h>
#include <MortonCoder.h>
#include <SplitCoder.h>
#include <PhaseCoder.h>
#include <TriangleCoder.h>
#include <PackedCoder.h>

#include <QImage>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#ifndef _WIN32
#include <unistd.h>
#else
#include <stdio.h>
#include <string.h>
int opterr = 1, optind = 1, optopt, optreset;
const char* optarg;
int getopt(int nargc, char* const nargv[], const char* ostr);
#endif

using namespace DStream;
using namespace std;

/** TODO LIST:
 *  - Support PNG
 */


void Usage()
{
    cerr <<
    R"use(Usage: dstreambenchmark [OPTIONS] <FILE>

    FILE is the path to the ASC file containing the depth data
      -d <output>: output folder in which data will be saved
      -a <algorithm>: algorithm to be tested (algorithm names: PACKED,TRIANGLE,MORTON,HILBERT,PHASE,SPLIT), if not specified, all of them will be tested
      -q <quality>: JPEG quality to be used (if not specified, values 80,85,90,95,100 will be tested)
      -f <format>: output format (JPEG or PNG), defaults to JPEG
      -?: display this message

    )use";
}


int ParseOptions(int argc, char** argv, string& inputFile, string& outFolder, string& algo, uint32_t& quality, string& outFormat)
{
    int c;

    while ((c = getopt(argc, argv, "d:a::q::f::")) != -1) {
        switch (c) {
        case 'd':
        {
            if (!filesystem::exists(outFolder) || filesystem::is_empty(outFolder))
                outFolder = optarg;
            else
            {
                cerr << "Output folder MUST be empty" << endl;
                return -5;
            }
            break;
        }
        case 'a':
        {
            std::string arg(optarg);
            if (arg=="PACKED" || arg=="TRIANGLE" || arg=="MORTON" || arg=="HILBERT" || arg=="PHASE" || arg=="SPLIT")
            {
                algo = optarg;
                break;
            }
            else
            {
                cerr << "Unknown algorithm " << arg << endl;
                Usage();
                return -1;
            }

        }
        case 'q':
        {
            int q = atoi(optarg);
            if (q >= 0 && q <= 100)
                quality = q;
            break;
        }
        case 'f':
        {
            std::string fmt(optarg);
            if (fmt=="JPEG" || fmt=="PNG")
                outFormat = optarg;
            break;
        }
        case '?': Usage(); return -1;
        default:
            cerr << "Unknown option: " << (char)c << endl;
            Usage();
            return -2;
        }
    }
    if (optind == argc) {
        cerr << "Missing filename" << endl;
        Usage();
        return -3;
    }

    if (optind != argc - 1) {
#ifdef _WIN32
        cerr << "Too many arguments or argument before other options\n";
#else
        cerr << "Too many arguments\n";
#endif
        Usage();
        return -4;
    }

    inputFile = argv[optind];
    return 0;
}


void RemoveNoiseNaive(std::vector<uint16_t>& data, uint32_t width, uint32_t height)
{
    for (uint32_t y=0; y<height; y++)
    {
        for (uint32_t x=0; x<width; x++)
        {
            if (y != 0 && x != 0 && x != (width-1) && y != (height-1))
            {
                int current = data[x + y * width];
                // Get all neighbors
                std::vector<int> neighbors;
                neighbors.push_back(data[x+1 + y * width]);
                neighbors.push_back(data[x-1 + y * width]);
                neighbors.push_back(data[x + (y+1) * width]);
                neighbors.push_back(data[x + (y-1) * width]);

                // Compute distance matrix
                int distanceMatrix[4][4];
                int outlierThreshold = 1000;

                for (int i=0; i<4; i++)
                    for (int j=0; j<4; j++)
                        distanceMatrix[i][j] = std::abs(neighbors[i] - neighbors[j]);

                uint32_t outliers = 0;
                // Check if there's an outlier
                for (uint32_t i=0; i<4; i++)
                    if (distanceMatrix[i][0] > outlierThreshold)
                        outliers++;

                // Find the right outliers (for each neighbor, return the one with the biggest row + column sum)
                std::vector<int> outliersIdx;

                for (uint32_t i=0; i<outliers; i++)
                {
                    int currOutlier = -1;
                    int maxSum = -1;
                    for (uint32_t j=0; j<4; j++)
                    {
                        // Compute row + column
                        int row = 0, col = 0;
                        for (uint32_t k=0; k<4; k++)
                        {
                            row += distanceMatrix[j][k];
                            col += distanceMatrix[k][j];
                        }

                        if ((row + col) > maxSum && !std::count(outliersIdx.begin(), outliersIdx.end(), j))
                        {
                            currOutlier = j;
                            maxSum = row + col;
                        }
                    }
                    if (currOutlier != -1)
                        outliersIdx.push_back(currOutlier);
                }

                // Compute the average withouit maxIdx
                float avg = 0; float divisor = 4 - outliersIdx.size();
                for (uint32_t i=0; i<4; i++)
                    if (!std::count(outliersIdx.begin(), outliersIdx.end(), i))
                        avg += neighbors[i];
                avg /= divisor;

                float err = std::abs(current - avg);

                if (err > 8)
                    data[x + y*width] = avg;
                else
                    data[x + y*width] = current;
            }
        }
    }
}


void RemoveNoiseMedian(std::vector<uint16_t>& data, uint32_t width, uint32_t height)
{
    int halfWind = 1;
    for (int y=0; y<height; y++)
    {
        for (int x=0; x<width; x++)
        {
            // Compute distance sqrt(matrix size)
            int matSize = (halfWind * 2 + 1) * (halfWind * 2 + 1);
            bool removed = false;
            if (y - halfWind < 0 || y+halfWind >= height)
            {
                matSize -= halfWind * 2 + 1;
                removed = true;
            }
            if (x - halfWind < 0 || x+halfWind >= width)
            {
                if (!removed)
                    matSize -= halfWind * 2 + 1;
                else
                    matSize -= halfWind * 2;
            }

            // Get all neighbors
            std::vector<int> neighbors;

            int current = data[x + y * width];
            for (int i=-halfWind; i<=halfWind; i++)
            {
                for (int j=-halfWind; j<=halfWind; j++)
                {
                    int xCoord = x + j;
                    int yCoord = (y+i);
                    if (xCoord >= 0 && xCoord < width && yCoord >= 0 && yCoord < height)
                        neighbors.push_back(data[xCoord + yCoord*width]);
                }
            }

            // Compute distance matrix
            int distanceMatrix[matSize][matSize];
            int outlierThreshold = 750;

            for (int i=0; i<matSize; i++)
                for (int j=0; j<matSize; j++)
                    distanceMatrix[i][j] = std::abs(neighbors[i] - neighbors[j]);

            uint32_t outliers = 0;
            // Check if there's an outlier
            for (uint32_t i=0; i<matSize; i++)
                if (distanceMatrix[i][0] > outlierThreshold)
                    outliers++;

            // Find the right outliers (for each neighbor, return the one with the biggest row + column sum)
            std::vector<int> outliersIdx;

            for (uint32_t i=0; i<outliers; i++)
            {
                int currOutlier = -1;
                int maxSum = -1;
                for (int j=0; j<matSize; j++)
                {
                    // Compute row + column
                    int row = 0, col = 0;
                    for (int k=0; k<matSize; k++)
                    {
                        row += distanceMatrix[j][k];
                        col += distanceMatrix[k][j];
                    }

                    if ((row + col) > maxSum && !std::count(outliersIdx.begin(), outliersIdx.end(), j))
                    {
                        currOutlier = j;
                        maxSum = row + col;
                    }
                }
                if (currOutlier != -1)
                    outliersIdx.push_back(currOutlier);
            }

            std::vector<uint16_t> goodNeighbors;
            for (int i=0; i<neighbors.size(); i++)
                if (!std::count(outliersIdx.begin(), outliersIdx.end(), i))
                    goodNeighbors.push_back(neighbors[i]);


            // Compute the average without maxIdx
            float avg = 0;
            for (uint32_t i=0; i<goodNeighbors.size(); i++)
                avg += goodNeighbors[i];
            avg /= goodNeighbors.size();

            float err = std::abs(current - avg);
            std::sort(goodNeighbors.begin(), goodNeighbors.end());

            if (err > 750)
                data[x + y*width] = goodNeighbors[goodNeighbors.size() / 2];
            else
                data[x + y*width] = current;
        }
    }
}

QVector<QRgb> LoadColorMap(string path)
{
    if (!filesystem::exists(path))
        return {};

    FILE* inFile = fopen(path.c_str(), "r");
    string line;
    QVector<QRgb> ret(256);

    fscanf(inFile, "%s\n", line.c_str());
    for (uint32_t i=0; i<256; i++)
    {
        float scalar;
        int r, g, b;

        fscanf(inFile, "%f,%d,%d,%d\n", &scalar, &r, &g, &b);
        ret[i] = qRgb(r,g,b);
    }

    return ret;
}

void SaveError(const std::string& outPath, uint16_t* originalData, uint16_t* decodedData, uint32_t width, uint32_t height,
               QVector<QRgb> colorMap, float& maxErr, float& avgErr)
{
    uint32_t nElements = width * height;
    vector<uint16_t> errorTextureData(nElements);
    unordered_map<uint16_t, int> errorFrequencies;
    QImage errorTexture(width, height, QImage::Format_Indexed8);
    errorTexture.setColorTable(colorMap);
    maxErr = -1e20;
    avgErr = 0.0f;

    // Compute error between decoded and original data
    for (uint32_t e=0; e<nElements; e++)
    {
        // Save errors
        float err = abs(originalData[e] - decodedData[e]);
        maxErr = max<float>(maxErr, err);
        avgErr += err;
        errorTextureData[e] = err;
        if (errorFrequencies.find(errorTextureData[e]) == errorFrequencies.end())
            errorFrequencies[errorTextureData[e]] = 1;
        else
            errorFrequencies[errorTextureData[e]]++;
    }
    avgErr /= nElements;

    // Save error texture
    for (uint32_t e=0; e<nElements; e++)
    {
        //uint32_t colIdx = 255.0f * (errorTextureData[e] / 65535.0f);
        float logErr = std::log2(1.0 + (float)errorTextureData[e]) * 16.0f;
        //logErr /= std::log(1.0 + maxErr);
        //logErr *= 255.0;
        errorTexture.setPixel(e % height, e / width, logErr);
    }
    errorTexture.save(QString(outPath.c_str()));

    // Save error histogram
    ofstream histogram;
    histogram.open(outPath + "histogram.csv");
    histogram << "Errors, Frequencies" << endl;
    for (uint32_t i=0; i<maxErr; i++)
        if (errorFrequencies[i] != 0)
            histogram << i << "," << errorFrequencies[i] << endl;

    // Save max error and avg error
    ofstream csv;
    csv.open(outPath + ".txt", ios::out);
    csv << "Max error: " << maxErr << ", avg error: " << avgErr;
    csv.close();
}

uint16_t* Quantize(uint16_t* input, uint32_t nElements, uint32_t quantization)
{
    uint16_t* ret = new uint16_t[nElements];
    for (uint32_t i=0; i<nElements; i++)
        ret[i] = (input[i] >> (16 - quantization)) << (16 - quantization);
    return ret;
}

int main(int argc, char *argv[])
{
    string algorithms[6] = {"HILBERT","PACKED","MORTON","TRIANGLE","PHASE","SPLIT"};
    uint32_t minQuality = 80, maxQuality = 100;

    string inputFile = "", outFolder = "", algo = "", outFormat = "JPG";
    uint32_t quality = 101;
    uint32_t quantization = 16;
    uint32_t hilbertBits = 3;

    /*
    for (uint16_t i=0; i<512; i++)
    {
        MortonCoder mc(16, 3);
        Color c = mc.ValueToColor(i);
        uint16_t d = mc.ColorToValue(c);

        if (d != i)
            std::cout << "Err on " << i << ": " << abs(d - i) << endl;
    }*/

    uint16_t max = 8;
    HilbertCoder hc(14, 3);

    /*
    for (uint16_t i=0; i<max; i++)
    {
        for (uint16_t j=0; j<max; j++)
        {
            for (uint16_t k=0; k<max; k++)
            {
                Color col = {(uint8_t)i, (uint8_t)j, (uint8_t)k};
                Color col2 = col;
                hc.TransposeToHilbertCoords(col2);
                hc.TransposeFromHilbertCoords(col2);

                if (col != col2)
                    std::cout << "Err " << (int)col.x << "," << (int)col.y << "," << (int)col.z << endl;
            }
        }
    }*/


    for (uint16_t i=260; i<16383; i++)
    {
        uint16_t val = i;
        HilbertCoder hc(14, 3);
        Color c = hc.ValueToColor(val);
        uint16_t d = hc.ColorToValue(c);

        if (d != val)
            cout << "Err on value " << i << ": " << abs(d - val) << endl;
    }

    if (ParseOptions(argc, argv, inputFile, outFolder, algo, quality, outFormat) < 0)
    {
        cout << "Error parsing command line arguments.\n";
        return -1;
    }

    // Assign user-specified algorithm and quality
    if (algo.compare(""))
    {
        algorithms[0] = algo;
        for (uint32_t i=1; i<6; i++)
            algorithms[i] = "";
    }

    if (quality <= 100)
    {
        minQuality = quality;
        maxQuality = quality;
    }

    // Prepare output folders
    if (!outFolder.compare(""))
        outFolder = "Output";
    filesystem::create_directory(outFolder);
    filesystem::create_directory(outFolder + "/Uncompressed_Decoding");
    for (uint32_t q=minQuality; q<=maxQuality; q+=5)
    {
        stringstream ss;
        ss << outFolder << "/Compressed_Encoding_" << q;
        filesystem::create_directory(ss.str());
    }

    // Prepare CSV file(s)
    ofstream uncompressedCsv;
    ofstream compressedCsv;
    //ofstream denoisedCsv;

    uncompressedCsv.open(outFolder + "/Uncompressed.csv", ios::out);
    compressedCsv.open(outFolder + "/Compressed.csv", ios::out);

    for (uint32_t i=0; i<6; i++)
    {
        if (algorithms[i].compare(""))
        {
            uncompressedCsv << algorithms[i] << "Max,";
            uncompressedCsv << algorithms[i] << "Avg,";
        }
    }
    uncompressedCsv << endl;

    for (uint32_t i=0; i<6; i++)
    {
        if (!algorithms[i].compare(""))
            break;
        for (uint32_t q=minQuality; q<=maxQuality; q+=5)
        {
            compressedCsv << algorithms[i] << q << "Max,";
            compressedCsv << algorithms[i] << q << "Avg,";
        }
    }
    compressedCsv << endl;

    // Load image
    Parser parser(inputFile, InputFormat::ASC);
    DepthmapData mapData;
    uint16_t* originalData = parser.Parse(mapData);

    uint32_t nElements = mapData.Width * mapData.Height;
    vector<uint8_t> encodedDataHolder(nElements * 3, 0);
    vector<uint16_t> decodedDataHolder(nElements, 0);
    auto colorMap = LoadColorMap("error_color_map.csv");
    uint16_t* quantizedData = Quantize(originalData, mapData.Width * mapData.Height, quantization);

    // Benchmark said image
    for (uint32_t a=0; a<6; a++)
    {
        float maxErr, avgErr;
        if (!algorithms[a].compare(""))
            break;

        // Encode and decode uncompressed data with current algorithm
        if (!algorithms[a].compare("MORTON"))
        {
            MortonCoder c(quantization, 6);
            c.Encode(quantizedData, encodedDataHolder.data(), nElements);
            c.Decode(encodedDataHolder.data(), decodedDataHolder.data(), nElements);
        }
        else if (!algorithms[a].compare("HILBERT"))
        {
            HilbertCoder c(14, 3);
            c.Encode(quantizedData, encodedDataHolder.data(), nElements);
            c.Decode(encodedDataHolder.data(), decodedDataHolder.data(), nElements);
        }
        else if (!algorithms[a].compare("PACKED"))
        {
            PackedCoder c(quantization);
            c.Encode(quantizedData, encodedDataHolder.data(), nElements);
            c.Decode(encodedDataHolder.data(), decodedDataHolder.data(), nElements);
        }
        else if (!algorithms[a].compare("SPLIT"))
        {
            SplitCoder c(quantization);
            c.Encode(quantizedData, encodedDataHolder.data(), nElements);
            c.Decode(encodedDataHolder.data(), decodedDataHolder.data(), nElements);
        }
        else if (!algorithms[a].compare("PHASE"))
        {
            PhaseCoder c(quantization);
            c.Encode(quantizedData, encodedDataHolder.data(), nElements);
            c.Decode(encodedDataHolder.data(), decodedDataHolder.data(), nElements);
        }
        else if (!algorithms[a].compare("TRIANGLE"))
        {
            TriangleCoder c(quantization);
            c.Encode(quantizedData, encodedDataHolder.data(), nElements);
            c.Decode(encodedDataHolder.data(), decodedDataHolder.data(), nElements);
        }


        SaveError(outFolder + "/Uncompressed_Decoding/error_" + algorithms[a], originalData, decodedDataHolder.data(), mapData.Width,
                  mapData.Height, colorMap, maxErr, avgErr);
        Writer outWriter(outFolder + "/Uncompressed_Decoding/decoded_" + algorithms[a] + ".png");
        outWriter.Write(decodedDataHolder.data(), mapData.Width, mapData.Height);
        uncompressedCsv << maxErr << "," << avgErr << ",";

        for (uint32_t q=minQuality; q<=maxQuality; q+=5)
        {
            // Output file name
            stringstream ss;
            ss << outFolder << "/Compressed_Encoding_" << q << "/";

            // Save in jpeg format, reload and check the error
            Writer writer(ss.str() + algorithms[a] + "_encoded.jpg");
            writer.Write(encodedDataHolder.data(), mapData.Width, mapData.Height, OutputFormat::JPG, false, q);

            cout << "Path: " << ss.str() + algorithms[a] + "_encoded.jpg" << endl;

            QImage img(QString((ss.str() + algorithms[a] + "_encoded.jpg").c_str()));
            img = img.convertToFormat(QImage::Format_RGB888);
            auto bits = img.bits();

            // Decode compressed data
            if (!algorithms[a].compare("MORTON"))
            {
                MortonCoder c(quantization, 6);
                c.Decode(bits, decodedDataHolder.data(), nElements);
            }
            else if (!algorithms[a].compare("HILBERT"))
            {
                HilbertCoder c(14, 3);
                c.Decode(bits, decodedDataHolder.data(), nElements);
                // Clean data
                //RemoveNoiseNaive(decodedDataHolder, mapData.Width, mapData.Height);
                //RemoveNoiseMedian(decodedDataHolder, mapData.Width, mapData.Height);
            }
            else if (!algorithms[a].compare("PACKED"))
            {
                PackedCoder c(quantization);
                c.Decode(bits, decodedDataHolder.data(), nElements);
            }
            else if (!algorithms[a].compare("SPLIT"))
            {
                SplitCoder c(quantization);
                c.Decode(bits, decodedDataHolder.data(), nElements);
            }
            else if (!algorithms[a].compare("PHASE"))
            {
                PhaseCoder c(quantization);
                c.Decode(bits, decodedDataHolder.data(), nElements);
            }
            else if (!algorithms[a].compare("TRIANGLE"))
            {
                TriangleCoder c(quantization);
                c.Decode(bits, decodedDataHolder.data(), nElements);
            }

            // Save decoded textures
            writer.SetPath(ss.str() + algorithms[a] + "_decoded.png");
            writer.Write(decodedDataHolder.data(), mapData.Width, mapData.Height);

            // Save decoded error
            SaveError(ss.str() + algorithms[a] + "_error.png", originalData, decodedDataHolder.data(), mapData.Width, mapData.Height,
                      colorMap, maxErr, avgErr);
            compressedCsv << maxErr << "," << avgErr << ",";
        }
    }

    delete[] originalData;
    delete[] quantizedData;

    return 0;
}


#ifdef _WIN32

int getopt(int nargc, char* const nargv[], const char* ostr) {
    static const char* place = "";        // option letter processing
    const char* oli;                      // option letter list index

    if (optreset || !*place) {             // update scanning pointer
        optreset = 0;
        if (optind >= nargc || *(place = nargv[optind]) != '-') {
            place = "";
            return -1;
        }

        if (place[1] && *++place == '-') { // found "--"
            ++optind;
            place = "";
            return -1;
        }
    }                                       // option letter okay?

    if ((optopt = (int)*place++) == (int)':' || !(oli = strchr(ostr, optopt))) {
        // if the user didn't specify '-' as an option,  assume it means -1.
        if (optopt == (int)'-')
            return (-1);
        if (!*place)
            ++optind;
        if (opterr && *ostr != ':')
            cout << "illegal option -- " << optopt << "\n";
        return ('?');
    }

    if (*++oli != ':') {                    // don't need argument
        optarg = NULL;
        if (!*place)
            ++optind;

    }
    else {                                // need an argument
        if (*place)                         // no white space
            optarg = place;
        else if (nargc <= ++optind) {       // no arg
            place = "";
            if (*ostr == ':')
                return (':');
            if (opterr)
                cout << "option requires an argument -- " << optopt << "\n";
            return (':');
        }
        else                              // white space
            optarg = nargv[optind];
        place = "";
        ++optind;
    }
    return optopt;                          // dump back option letter
}

#endif
