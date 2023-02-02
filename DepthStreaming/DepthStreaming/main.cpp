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
#include <cmath>

#ifndef _WIN32
#include <unistd.h>
#else
#include <HueCoder.h>
#include <stdio.h>
#include <string.h>
int opterr = 1, optind = 1, optopt, optreset;
const char* optarg;
int getopt(int nargc, char* const nargv[], const char* ostr);
#endif

using namespace DStream;
using namespace std;

unsigned char turbo_srgb_bytes[256][3] = {{48,18,59},{50,21,67},{51,24,74},{52,27,81},{53,30,88},{54,33,95},{55,36,102},{56,39,109},{57,42,115},{58,45,121},{59,47,128},{60,50,134},{61,53,139},{62,56,145},{63,59,151},{63,62,156},{64,64,162},{65,67,167},{65,70,172},{66,73,177},{66,75,181},{67,78,186},{68,81,191},{68,84,195},{68,86,199},{69,89,203},{69,92,207},{69,94,211},{70,97,214},{70,100,218},{70,102,221},{70,105,224},{70,107,227},{71,110,230},{71,113,233},{71,115,235},{71,118,238},{71,120,240},{71,123,242},{70,125,244},{70,128,246},{70,130,248},{70,133,250},{70,135,251},{69,138,252},{69,140,253},{68,143,254},{67,145,254},{66,148,255},{65,150,255},{64,153,255},{62,155,254},{61,158,254},{59,160,253},{58,163,252},{56,165,251},{55,168,250},{53,171,248},{51,173,247},{49,175,245},{47,178,244},{46,180,242},{44,183,240},{42,185,238},{40,188,235},{39,190,233},{37,192,231},{35,195,228},{34,197,226},{32,199,223},{31,201,221},{30,203,218},{28,205,216},{27,208,213},{26,210,210},{26,212,208},{25,213,205},{24,215,202},{24,217,200},{24,219,197},{24,221,194},{24,222,192},{24,224,189},{25,226,187},{25,227,185},{26,228,182},{28,230,180},{29,231,178},{31,233,175},{32,234,172},{34,235,170},{37,236,167},{39,238,164},{42,239,161},{44,240,158},{47,241,155},{50,242,152},{53,243,148},{56,244,145},{60,245,142},{63,246,138},{67,247,135},{70,248,132},{74,248,128},{78,249,125},{82,250,122},{85,250,118},{89,251,115},{93,252,111},{97,252,108},{101,253,105},{105,253,102},{109,254,98},{113,254,95},{117,254,92},{121,254,89},{125,255,86},{128,255,83},{132,255,81},{136,255,78},{139,255,75},{143,255,73},{146,255,71},{150,254,68},{153,254,66},{156,254,64},{159,253,63},{161,253,61},{164,252,60},{167,252,58},{169,251,57},{172,251,56},{175,250,55},{177,249,54},{180,248,54},{183,247,53},{185,246,53},{188,245,52},{190,244,52},{193,243,52},{195,241,52},{198,240,52},{200,239,52},{203,237,52},{205,236,52},{208,234,52},{210,233,53},{212,231,53},{215,229,53},{217,228,54},{219,226,54},{221,224,55},{223,223,55},{225,221,55},{227,219,56},{229,217,56},{231,215,57},{233,213,57},{235,211,57},{236,209,58},{238,207,58},{239,205,58},{241,203,58},{242,201,58},{244,199,58},{245,197,58},{246,195,58},{247,193,58},{248,190,57},{249,188,57},{250,186,57},{251,184,56},{251,182,55},{252,179,54},{252,177,54},{253,174,53},{253,172,52},{254,169,51},{254,167,50},{254,164,49},{254,161,48},{254,158,47},{254,155,45},{254,153,44},{254,150,43},{254,147,42},{254,144,41},{253,141,39},{253,138,38},{252,135,37},{252,132,35},{251,129,34},{251,126,33},{250,123,31},{249,120,30},{249,117,29},{248,114,28},{247,111,26},{246,108,25},{245,105,24},{244,102,23},{243,99,21},{242,96,20},{241,93,19},{240,91,18},{239,88,17},{237,85,16},{236,83,15},{235,80,14},{234,78,13},{232,75,12},{231,73,12},{229,71,11},{228,69,10},{226,67,10},{225,65,9},{223,63,8},{221,61,8},{220,59,7},{218,57,7},{216,55,6},{214,53,6},{212,51,5},{210,49,5},{208,47,5},{206,45,4},{204,43,4},{202,42,4},{200,40,3},{197,38,3},{195,37,3},{193,35,2},{190,33,2},{188,32,2},{185,30,2},{183,29,2},{180,27,1},{178,26,1},{175,24,1},{172,23,1},{169,22,1},{167,20,1},{164,19,1},{161,18,1},{158,16,1},{155,15,1},{152,14,1},{149,13,1},{146,11,1},{142,10,1},{139,9,2},{136,8,2},{133,7,2},{129,6,2},{126,5,2},{122,4,3}};

/*
 * Bug:
 * - Q:14, P:4 (either >= or >)
 * - Q:16, P:4 (either >= or >)
 * - Q:16, P:5
 */

struct ErrorData
{
    float MaxError = -1e20;
    float AvgError = 0.0f;

    float DespeckledMaxError = -1e20;
    float DespeckledAvgError = 0.0f;

    uint32_t EncodedTextureSize;
};

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

    while ((c = getopt(argc, argv, "d:a::q:f::")) != -1) {
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

void RemoveNoiseMedianSimple(std::vector<uint16_t>& data, uint32_t width, uint32_t height)
{
    int halfWind = 1;
    for (int y=0; y<height; y++)
    {
        for (int x=0; x<width; x++)
        {
            // Get all neighbors
            std::vector<uint16_t> neighbors;
            int current = data[x + y * width];

            for (int i=-halfWind; i<=halfWind; i++)
            {
                for (int j=-halfWind; j<=halfWind; j++)
                {
                    int xCoord = x + j;
                    int yCoord = (y+i);
                    // Don't include current point
                    if (xCoord >= 0 && xCoord < width && yCoord >= 0 && yCoord < height)
                        neighbors.push_back(data[xCoord + yCoord*width]);
                }
            }

            std::sort(neighbors.begin(), neighbors.end());
            float err = std::abs(current - neighbors[neighbors.size()/2]);

            if (err > 5000)
                data[x + y*width] = neighbors[neighbors.size() / 2];
            else
                data[x + y*width] = current;
        }
    }
}

QVector<QRgb> LoadColorMap(string path)
{
    QVector<QRgb> ret(256);

    for (uint32_t i=0; i<256; i++)
        ret[i] = qRgb(turbo_srgb_bytes[i][0],turbo_srgb_bytes[i][1],turbo_srgb_bytes[i][2]);

    return ret;
}

ErrorData SaveError(const std::string& outPath, uint16_t* originalData, uint16_t* decodedData, uint32_t width, uint32_t height, QVector<QRgb> colorMap)
{
    ErrorData ret;

    uint32_t nElements = width * height;
    vector<uint16_t> errorTextureData(nElements);
    unordered_map<uint16_t, int> errorFrequencies;
    QImage errorTexture(width, height, QImage::Format_Indexed8);
    errorTexture.setColorTable(colorMap);

    // Compute error between decoded and original data
    for (uint32_t e=0; e<nElements; e++)
    {
        // Save errors
        float err = abs(originalData[e] - decodedData[e]);
        ret.MaxError = max<float>(ret.MaxError, err);
        ret.AvgError += err;
        errorTextureData[e] = err;
        if (errorFrequencies.find(errorTextureData[e/ 255]) == errorFrequencies.end())
            errorFrequencies[errorTextureData[e/ 255]] = 1;
        else
            errorFrequencies[errorTextureData[e/ 255]]++;
    }
    ret.AvgError /= nElements;

    // Save error texture
    for (uint32_t e=0; e<nElements; e++)
    {
        float logErr = std::log2(1.0 + (float)errorTextureData[e]) * std::log2((1<<16)-1);
        errorTexture.setPixel(e % height, e / width, logErr);
    }
    errorTexture.save(QString(outPath.c_str()));

    // Save error histogram
    ofstream histogram;
    histogram.open(outPath + "histogram.csv");
    histogram << "Errors, Frequencies" << endl;
    for (uint32_t i=0; i<ret.MaxError; i++)
        if (errorFrequencies[i] != 0)
            histogram << i*255 << "-" << i*255+255 << "," << errorFrequencies[i] << endl;

    return ret;
}

uint16_t* Quantize(uint16_t* input, uint32_t nElements, uint32_t quantization)
{
    uint16_t* ret = new uint16_t[nElements];
    for (uint32_t i=0; i<nElements; i++)
        ret[i] = (input[i] >> (16 - quantization)) << (16 - quantization);
    return ret;
}

string GetPathFromComponents(vector<string> path)
{
    string ret = "";
    for (uint32_t i=0; i<path.size(); i++)
        ret += path[i] + "/";
    return ret;
}

void Encode(string algo, uint16_t* srcData, uint8_t* destData, int quantization, int parameter, int nElements)
{
    // Encode uncompressed data with current algorithm
    if (!algo.compare("MORTON"))
    {
        MortonCoder c(quantization, 6);
        c.Encode(srcData, destData, nElements);
    }
    else if (!algo.compare("HILBERT"))
    {
        HilbertCoder c(quantization, parameter);
        c.Encode(srcData, destData, nElements);
    }
    else if (!algo.compare("PACKED"))
    {
        PackedCoder c(quantization, parameter);
        c.Encode(srcData, destData, nElements);
    }
    else if (!algo.compare("SPLIT"))
    {
        SplitCoder c(quantization);
        c.Encode(srcData, destData, nElements);
    }
    else if (!algo.compare("PHASE"))
    {
        PhaseCoder c(quantization);
        c.Encode(srcData, destData, nElements);
    }
    else if (!algo.compare("TRIANGLE"))
    {
        TriangleCoder c(quantization);
        c.Encode(srcData, destData, nElements);
    }
    else if (!algo.compare("HUE"))
    {
        HueCoder c(quantization);
        c.Encode(srcData, destData, nElements);
    }
}

void Decode(string algo, uint8_t* srcData, uint16_t* destData, int quantization, int parameter, int nElements)
{
    // Decode compressed data
    if (!algo.compare("MORTON"))
    {
        MortonCoder c(quantization, 6);
        c.Decode(srcData, destData, nElements);
    }
    else if (!algo.compare("HILBERT"))
    {
        HilbertCoder c(quantization, parameter);
        c.Decode(srcData, destData, nElements);
    }
    else if (!algo.compare("PACKED"))
    {
        PackedCoder c(quantization, parameter);
        c.Decode(srcData, destData, nElements);
    }
    else if (!algo.compare("SPLIT"))
    {
        SplitCoder c(quantization);
        c.Decode(srcData, destData, nElements);
    }
    else if (!algo.compare("PHASE"))
    {
        PhaseCoder c(quantization);
        c.Decode(srcData, destData, nElements);
    }
    else if (!algo.compare("TRIANGLE"))
    {
        TriangleCoder c(quantization);
        c.Decode(srcData, destData, nElements);
    }
    else if (!algo.compare("HUE"))
    {
        HueCoder c(quantization);
        c.Decode(srcData, destData, nElements);
    }
}

void AddBenchmarkResult(ofstream& file, string algo, string outFormat, int quantization, int jpegQuality, int parameter, const ErrorData& errData)
{
    stringstream configName;
    configName << algo << "_Q:" << quantization;
    if (!outFormat.compare("JPG"))
        configName << "_J:" << jpegQuality;
    if (!algo.compare("HILBERT") || !algo.compare("PACKED"))
        configName << "_P:" << parameter;

    //"Configuration, Max Error, Avg Error, Despeckle Max Error, Despeckle Avg Error, Compressed Size\n";
    file << configName.str() << "," << errData.MaxError << "," << errData.AvgError << ",";
    if (errData.DespeckledMaxError >= 0.0f)
        file << errData.DespeckledMaxError << "," << errData.DespeckledAvgError << ",";
    else
        file << "/,/,";
    file << errData.EncodedTextureSize << "\n";

    cout << errData.AvgError << endl;
}

int main(int argc, char *argv[])
{
    string algorithms[7] = {"HUE", "HILBERT", "PACKED", "SPLIT", "TRIANGLE", "PHASE", "MORTON"};
    string outFormats[2] = {"JPG", "PNG"};

    string inputFile = "", outFolder = "", algo = "", outFormat = "JPG";
    uint32_t quality = 101;

    uint32_t minQuality = 70;
    uint32_t maxQuality = 100;
    uint32_t minQuantization = 10;
    uint32_t maxQuantization = 16;

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

    // Prepare CSV file(s)
    ofstream compressedCsv;
    compressedCsv.open(outFolder + "/BenchmarkData.csv", ios::out);
    compressedCsv << "Configuration, Max Error, Avg Error, Despeckle Max Error, Despeckle Avg Error, Compressed Size\n";

    // Load image, setup benchmark
    Parser parser(inputFile, InputFormat::ASC);
    DepthmapData mapData;
    uint16_t* originalData = parser.Parse(mapData);
    uint32_t nElements = mapData.Width * mapData.Height;

    vector<uint8_t> encodedDataHolder(nElements * 3, 0);
    vector<uint16_t> decodedDataHolder(nElements, 0);
    vector<string> folders;

    auto colorMap = LoadColorMap("error_color_map.csv");

    stringstream utilitySs;
    std::filesystem::path currPathFs;
    folders.push_back(outFolder);

    HueCoder hc(10);

    for (uint32_t i=21952; i<65536; i++)
    {
        Color col = hc.ValueToColor(i);
        uint16_t val = hc.ColorToValue(col);
        int err = abs((uint16_t)((i >> 6) << 6) - val);

        if (err > 64)
        {
            cout << "Err: " << err << ",i: " << i << endl;
        }
    }
    /*
    uint32_t q = 14;
    int noiseAmount = 20;

    HilbertCoder hc(q, 4);
    for (int j=0; j<(1 << q)-1; j++)
    {
        Color c = hc.ValueToColor(j << (16 - q));

        for (int i=-noiseAmount; i<=noiseAmount; i+=noiseAmount)
        {
            for (uint32_t k=0; k<3; k++)
                if (c[k] != 0 || i >= 0)
                    c[k] += i;

            uint16_t val = hc.ColorToValue(c);
            int err = abs((int)val - (j << (16-q)));
            if ((j << (16-q)) != val)
                cout << "HILBERT ERROR : " << err << " on value: " << j << endl;
        }
    }
*/

    // ALGORITHM
    for (uint32_t a=0; a<1; a++)
    {
        if (!algorithms[a].compare(""))
            break;
        folders.push_back(algorithms[a]);
        currPathFs = GetPathFromComponents(folders);
        filesystem::create_directory(currPathFs);

        // QUANTIZATION
        for (uint32_t q=minQuantization; q<=maxQuantization; q+=2)
        {
            utilitySs.str("");
            utilitySs << "Quantization" << q;
            folders.push_back(utilitySs.str());

            currPathFs = GetPathFromComponents(folders);
            filesystem::create_directory(currPathFs);

            uint16_t* quantizedData = Quantize(originalData, mapData.Width * mapData.Height, q);
            uint32_t parameterStart = 0;
            uint32_t parameterMax = 0;
            uint32_t parameterIncrease = 1;

            if (!algorithms[a].compare("HILBERT"))
            {
                uint32_t maxHilbertBits, minHilbertBits;
                bool foundMin = false;

                for (uint32_t i=2; i<=6; i++)
                {
                    if (!(i * 3 < q)) continue;
                    int seg = q - 3 * i;
                    if (i + seg > 8) continue;

                    if (!foundMin)
                    {
                        foundMin = true;
                        minHilbertBits = i;
                    }
                    else
                        maxHilbertBits = i;
                }

                parameterStart = minHilbertBits;
                parameterMax = maxHilbertBits;
                parameterIncrease = 1;

                /*
                for (uint32_t i=4; i<=parameterMax; i++)
                {
                    HilbertCoder hc(q, i);
                    for (uint32_t j=0; j<(1 << q)-1; j++)
                    {
                        uint16_t v = j << (16 - q);
                        Color c = hc.ValueToColor(v);
                        c[0] += 8;
                        uint16_t val = hc.ColorToValue(c);

                        if (abs(v - val) > 500)
                        {
                            c = hc.ValueToColor(v);
                            c[0] += 8;
                            val = hc.ColorToValue(c);
                        }
                        if (v != val)
                            cout << "HILBERT ERROR, q: " << q << ", curve: " << i << ", value: " << j << ", error: " << abs(v - val) << endl;
                    }
                }
                */
            }
            else if (!algorithms[a].compare("PACKED"))
            {
                uint32_t minLeftBits = q - 8;
                uint32_t maxLeftBits = 8;

                parameterStart = minLeftBits;
                parameterMax = maxLeftBits;
                parameterIncrease = 1;
            }

            // Cycle through parameters
            for (uint32_t p=parameterStart; p<=parameterMax; p+=parameterIncrease)
            {
                if (parameterMax > 1)
                {
                    utilitySs.str("");
                    utilitySs << "Parameter" << p;
                    folders.push_back(utilitySs.str());

                    currPathFs = GetPathFromComponents(folders);
                    filesystem::create_directory(currPathFs);
                }

                // Encode uncompressed data with current algorithm
                Encode(algorithms[a], quantizedData, encodedDataHolder.data(), q, p, nElements);

                // Save encoded to uncompressed texture
                string currPath = GetPathFromComponents(folders);
                Writer png(currPath + "uncompressed.png");
                png.Write(encodedDataHolder.data(), mapData.Width, mapData.Height, OutputFormat::PNG);

                cout << currPath << endl;

                // Save encoded to compressed JPG texture
                for (uint32_t j=minQuality; j<=maxQuality; j+=5)
                {
                    std::stringstream ss;
                    ss << "Quality" << j;
                    Writer jpg(currPath + ss.str() + ".jpg");
                    jpg.Write(encodedDataHolder.data(), mapData.Width, mapData.Height, OutputFormat::JPG, false, j);
                }

                // Load JPG, load PNG, decode and measure
                for (uint32_t f=0; f<2; f++)
                {
                    // IF JPG: QUALITY
                    if (!outFormats[f].compare("JPG"))
                    {
                        for (uint32_t j=minQuality; j<=maxQuality; j+=5)
                        {
                            ErrorData errData;
                            std::stringstream ss;
                            ss << currPath << "Quality" << j;

                            QImage img(QString((ss.str() + ".jpg").c_str()));
                            img = img.convertToFormat(QImage::Format_RGB888);
                            auto bits = img.bits();

                            // Decode the image
                            Decode(algorithms[a], bits, decodedDataHolder.data(), q, p, nElements);

                            // Save decoded textures
                            Writer writer(ss.str() + "_decoded.png");
                            writer.Write(decodedDataHolder.data(), mapData.Width, mapData.Height);

                            // Save decoded error
                            errData = SaveError(ss.str() + "_error.png", originalData, decodedDataHolder.data(), mapData.Width, mapData.Height, colorMap);
                            errData.EncodedTextureSize = std::filesystem::file_size(ss.str() + ".jpg");
                            // Add result if you don't need to despeckle
                            if (algorithms[a].compare("HILBERT") != 0)
                                AddBenchmarkResult(compressedCsv, algorithms[a], outFormats[f], q, j, p, errData);

                            // Despeckle if needed
                            if (!algorithms[a].compare("HILBERT"))
                            {
                                ErrorData errData2;
                                // Clean data
                                //RemoveNoiseMedian(decodedDataHolder, mapData.Width, mapData.Height);
                                RemoveNoiseMedianSimple(decodedDataHolder, mapData.Width, mapData.Height);
                                // Save denoised, save error
                                Writer writer(ss.str() + "_decoded_denoised.png");
                                writer.Write(decodedDataHolder.data(), mapData.Width, mapData.Height);

                                errData2 = SaveError(ss.str() + "denoised_error.png", originalData, decodedDataHolder.data(), mapData.Width, mapData.Height, colorMap);
                                errData.DespeckledAvgError = errData2.AvgError;
                                errData.DespeckledMaxError = errData2.MaxError;

                                AddBenchmarkResult(compressedCsv, algorithms[a], outFormats[f], q, j, p, errData);
                            }
                        }
                    }
                    else
                    {
                        // Decode the uncompressed texture
                        QImage img(QString((currPath + "uncompressed.png").c_str()));

                        img = img.convertToFormat(QImage::Format_RGB888);
                        auto bits = img.bits();

                        Decode(algorithms[a], bits, decodedDataHolder.data(), q, p, nElements);
                        Writer writer(currPath + "uncompressed_decoded.png");
                        writer.Write(decodedDataHolder.data(), mapData.Width, mapData.Height);

                        // Save decoded error
                        ErrorData errData = SaveError(currPath + "uncompressed_decoded_error.png", originalData, decodedDataHolder.data(), mapData.Width, mapData.Height, colorMap);
                        errData.EncodedTextureSize = img.sizeInBytes();

                        if (!algorithms[a].compare("HILBERT"))
                        {
                            // Clean data
                            RemoveNoiseMedianSimple(decodedDataHolder, mapData.Width, mapData.Height);
                            // Save denoised, save error
                            Writer writer(currPath + "uncompressed_decoded_denoised.png");
                            writer.Write(decodedDataHolder.data(), mapData.Width, mapData.Height);

                            ErrorData errData2 = SaveError(currPath + "uncompressed_decoded_denoised_error.png", originalData, decodedDataHolder.data(), mapData.Width, mapData.Height, colorMap);
                            errData.EncodedTextureSize = img.sizeInBytes();
                            errData.DespeckledAvgError = errData2.AvgError;
                            errData.DespeckledMaxError = errData2.MaxError;

                            AddBenchmarkResult(compressedCsv, algorithms[a], outFormats[f], q, -1, p, errData);
                        }
                    }
                }

                if (parameterMax > 1)
                    folders.pop_back();
            }

            folders.pop_back();
            delete[] quantizedData;
        }
        folders.pop_back();
    }

    delete[] originalData;
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
