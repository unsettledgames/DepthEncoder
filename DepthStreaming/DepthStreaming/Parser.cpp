#include <Parser.h>
#include <iostream>

#include <QFile>

namespace DStream
{
    Parser::Parser(const std::string& path, InputFormat format) : m_InputPath(path), m_Format(format) {}

    uint16_t* Parser::Parse(DepthmapData& dmData)
    {
        switch (m_Format)
        {
        case InputFormat::ASC:
            return ParseASC(dmData);
            break;
        default:
            std::cout << "Unsupported input type" << std::endl;
            break;
        }
    }

    uint16_t* Parser::ParseASC(DepthmapData& dmData)
    {
        if (!QFile::exists(QString(m_InputPath.c_str())))
        {
            std::cerr << "File " << m_InputPath << " does not exist" << std::endl;
            return nullptr;
        }

        // Load asc data
        FILE* fp = fopen(m_InputPath.c_str(), "rb");

        if(!fp)
        {
            std::cerr << "Could not open: " << m_InputPath << std::endl;
            return nullptr;
        }

        float nodata;
        fscanf(fp, "ncols %d\n", &dmData.Width);
        fscanf(fp, "nrows %d\n", &dmData.Height);
        fscanf(fp, "xllcenter %f\n", &dmData.CenterX);
        fscanf(fp, "yllcenter %f\n", &dmData.CenterY);
        fscanf(fp, "cellsize %f\n", &dmData.CellSize);
        fscanf(fp, "nodata_value %f\n", &nodata);

        // Load depth data
        uint16_t* dest = new uint16_t[dmData.Width * dmData.Height];
        float* tmp = new float[dmData.Width * dmData.Height];
        float min = 1e20;
        float max = -1e20;

        for(uint32_t i = 0; i < dmData.Width*dmData.Height; i++)
        {
            float h;
            fscanf(fp, "%f", &h);
            min = std::min(min, h);
            max = std::max(max, h);
            tmp[i] = h;
        }

        // Quantize
        for (uint32_t i=0; i<dmData.Width*dmData.Height; i++)
            dest[i] = ((tmp[i] - min) / (float)(max - min)) * 65535.0f;

        dmData.Valid = true;

        delete[] tmp;
        return dest;
    }
}
