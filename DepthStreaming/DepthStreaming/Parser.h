#ifndef PARSER_H
#define PARSER_H

#include <string>

namespace DStream
{
    enum InputFormat { ASC = 0 };

    struct DepthmapData
    {
        bool Valid = false;

        uint32_t Width;
        uint32_t Height;

        float CenterX;
        float CenterY;
        float CellSize;

        DepthmapData() = default;
        DepthmapData(const DepthmapData& data) = default;
    };

    class Parser
    {
    public:
        Parser(const std::string& path, InputFormat format);
        uint16_t* Parse(DepthmapData& dmData);

    private:
        uint16_t* ParseASC(DepthmapData& dmData);

    private:
        std::string m_InputPath;
        InputFormat m_Format;
    };
}

#endif // PARSER_H
