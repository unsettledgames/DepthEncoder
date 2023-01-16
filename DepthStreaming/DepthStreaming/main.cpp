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

                for (int i=0; i<4; i++)
                    for (int j=0; j<4; j++)
                        distanceMatrix[i][j] = std::abs(neighbors[i] - neighbors[j]);

                int outliers = 0;
                // Check if there's an outlier
                for (uint32_t i=0; i<4; i++)
                    if (distanceMatrix[i][0] > outlierThreshold)
                        outliers++;

                // Find the right outliers (for each neighbor, return the one with the biggest row + column sum)
                std::vector<int> outliersIdx;
                for (uint32_t i=0; i<outliers; i++)
                {
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
                            outliersIdx.push_back(j);
                            maxSum = row + col;
                        }
                    }
                }

                // Compute the average withouit maxIdx
                float avg = 0; float divisor = 4 - outliersIdx.size();
                for (uint32_t i=0; i<4; i++)
                    if (!std::count(outliersIdx.begin(), outliersIdx.end(), i))
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
    }
    avgErr /= nElements;

    // Save error texture
    for (uint32_t e=0; e<nElements; e++)
    {
        uint32_t colIdx = 255.0f * (errorTextureData[e] / maxErr);
        errorTexture.setPixel(e / width, e % height, colIdx);
    }
    errorTexture.save(QString(outPath.c_str()) + ".png");

    // Save error histogram (QChart? QChart.grab().save("path");

    // Save max error and avg error
    ofstream csv;
    csv.open(outPath + ".txt", ios::out);
    csv << "Max error: " << maxErr << ", avg error: " << avgErr;
    csv.close();
}

int main(int argc, char *argv[])
{
    string algorithms[6] = {"PACKED","TRIANGLE","MORTON","HILBERT","PHASE","SPLIT"};
    uint32_t minQuality = 80, maxQuality = 100;

    string inputFile = "", outFolder = "", algo = "", outFormat = "JPG";
    uint32_t quality = 101;

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
        if (algorithms[i].compare(""))
            uncompressedCsv << algorithms[i] << ",";
    uncompressedCsv << endl;

    for (uint32_t i=0; i<6; i++)
    {
        if (algorithms[i].compare(""))
            break;
        for (uint32_t q=minQuality; q<=maxQuality; q++)
        {
            compressedCsv << algorithms[i] << q << "Max";
            compressedCsv << algorithms[i] << q << "Avg";
        }
    }
    uncompressedCsv << endl;

    // Load image
    Parser parser(inputFile, InputFormat::ASC);
    DepthmapData mapData;
    uint16_t* originalData = parser.Parse(mapData);

    uint32_t nElements = mapData.Width * mapData.Height;
    vector<uint8_t> encodedDataHolder(nElements * 3, 0);
    vector<uint16_t> decodedDataHolder(nElements, 0);
    auto colorMap = LoadColorMap("error_color_map.csv");

    // Benchmark said image
    for (uint32_t a=0; a<6; a++)
    {
        float maxErr, avgErr;
        if (!algorithms[a].compare(""))
            break;
        if (!algorithms[a].compare("PACKED"))
            continue;

        // Encode and decode uncompressed data with current algorithm
        if (!algorithms[a].compare("MORTON"))
        {
            MortonCoder c(16, 6);
            c.Encode(originalData, encodedDataHolder.data(), nElements);
            c.Decode(encodedDataHolder.data(), decodedDataHolder.data(), nElements);
        }
        else if (!algorithms[a].compare("HILBERT"))
        {
            HilbertCoder c(16, 6);
            c.Encode(originalData, encodedDataHolder.data(), nElements);
            c.Decode(encodedDataHolder.data(), decodedDataHolder.data(), nElements);
        }
        else if (!algorithms[a].compare("PACKED"))
        {
            PackedCoder c(16);
            c.Encode(originalData, encodedDataHolder.data(), nElements);
            c.Decode(encodedDataHolder.data(), decodedDataHolder.data(), nElements);
        }
        else if (!algorithms[a].compare("SPLIT"))
        {
            SplitCoder c(16);
            c.Encode(originalData, encodedDataHolder.data(), nElements);
            c.Decode(encodedDataHolder.data(), decodedDataHolder.data(), nElements);
        }
        else if (!algorithms[a].compare("PHASE"))
        {
            PhaseCoder c(16);
            c.Encode(originalData, encodedDataHolder.data(), nElements);
            c.Decode(encodedDataHolder.data(), decodedDataHolder.data(), nElements);
        }
        else if (!algorithms[a].compare("TRIANGLE"))
        {
            TriangleCoder c(16);
            c.Encode(originalData, encodedDataHolder.data(), nElements);
            c.Decode(encodedDataHolder.data(), decodedDataHolder.data(), nElements);
        }


        SaveError(outFolder + "/Uncompressed_Decoding/error_" + algorithms[a], originalData, decodedDataHolder.data(), mapData.Width,
                  mapData.Height, colorMap, maxErr, avgErr);

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
                MortonCoder c(16, 6);
                c.Decode(bits, decodedDataHolder.data(), nElements);
            }
            else if (!algorithms[a].compare("HILBERT"))
            {
                HilbertCoder c(16, 6);
                c.Decode(bits, decodedDataHolder.data(), nElements);
            }
            else if (!algorithms[a].compare("PACKED"))
            {
                PackedCoder c(16);
                c.Decode(bits, decodedDataHolder.data(), nElements);
            }
            else if (!algorithms[a].compare("SPLIT"))
            {
                SplitCoder c(16);
                c.Decode(bits, decodedDataHolder.data(), nElements);
            }
            else if (!algorithms[a].compare("PHASE"))
            {
                PhaseCoder c(16);
                c.Decode(bits, decodedDataHolder.data(), nElements);
            }
            else if (!algorithms[a].compare("TRIANGLE"))
            {
                TriangleCoder c(16);
                c.Decode(bits, decodedDataHolder.data(), nElements);
            }

            // Save decoded textures
            writer.SetPath(ss.str() + algorithms[a] + "_decoded.png");
            writer.Write(decodedDataHolder.data(), mapData.Width, mapData.Height);

            // Save decoded error
            SaveError(ss.str() + algorithms[a] + "_error.png", originalData, decodedDataHolder.data(), mapData.Width, mapData.Height,
                      colorMap, maxErr, avgErr);
        }
    }

    delete[] originalData;


    //      Comprimi con algoritmo, usa quantizzazione specificata
    //      Decomprimi, confronta con dati originali non quantizzati
    // OK   Salva codifica in JPEG
    // OK   Leggi JPEG, decomprimi, confronta con dati quantizzati
    // OK   Salva decodifica in PNG
    //      Rimuovi outlier, confronta con dati quantizzati

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
