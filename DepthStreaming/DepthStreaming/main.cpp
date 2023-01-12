#include <Parser.h>
#include <Compressor.h>
#include <Writer.h>

#include <iostream>
#include <vector>

int main(int argc, char *argv[])
{
    uint16_t* data = nullptr;
    DStream::DepthmapData dmData;
    DStream::Writer writer("encoded.jpg");
    DStream::Parser parser("envoi_RTI/MNT.asc", DStream::InputFormat::ASC);

    data = parser.Parse(dmData);

    std::vector<uint8_t> compressed(dmData.Width * dmData.Height * 3);
    DStream::Compressor compressor(dmData.Width, dmData.Height);

    compressor.Encode(data, compressed.data(), dmData.Width * dmData.Height, DStream::EncodingType::SPLIT);
    writer.Write(compressed.data(), dmData.Width, dmData.Height, DStream::OutputFormat::JPG, false, 100);

    compressor.Decode(compressed.data(), data, dmData.Width * dmData.Height, DStream::EncodingType::SPLIT);

    writer.SetPath("decoded.png");
    writer.Write(data, dmData.Width, dmData.Height, false);

    delete[] data;
    return 0;
}
