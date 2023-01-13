#ifndef WRITER_H
#define WRITER_H

#include <string>

class QString;
class QImage;

namespace DStream
{
    enum OutputFormat { JPG = 0, PNG };

    class Writer
    {
    public:
        Writer(const std::string& path);
        bool Write(uint8_t* data, uint32_t width, uint32_t height, OutputFormat format, bool splitChannels = false, uint32_t quality = 100);
        bool Write(uint16_t* data, uint32_t width, uint32_t height);

        inline void SetPath(const std::string& path) {m_OutputPath = path;}
    private:
        std::string m_OutputPath;
    };
}

#endif // WRITER_H
