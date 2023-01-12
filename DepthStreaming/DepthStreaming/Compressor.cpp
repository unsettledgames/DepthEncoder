#include <Compressor.h>
#include <Algorithms.h>

namespace DStream
{
    Compressor::Compressor(uint32_t width, uint32_t height) : m_Width(width), m_Height(height) {}

    bool Compressor::Encode(uint16_t* values, uint8_t* dest, uint32_t count, EncodingType type)
    {
        for (uint32_t i=0; i<count; i++)
        {
            Color encoded;

            switch (type)
            {
            case EncodingType::TRIANGLE:
                encoded = TriangleToColor(values[i]);
                break;
            case EncodingType::MORTON:
                encoded = MortonToColor(values[i], CurveOrder<uint16_t>());
                break;
            case EncodingType::HILBERT:
                encoded = HilbertToColor(values[i], CurveOrder<uint16_t>());
                break;
            case EncodingType::PHASE:
                encoded = PhaseToColor(values[i]);
                break;
            case EncodingType::SPLIT:
                encoded = SplitToColor(values[i]);
                break;
            default:
                break;
            }

            // Add color to result
            for (uint32_t j=0; j<3; j++)
                dest[i*3+j] = encoded[j];
        }

        return true;
    }

    bool Compressor::Decode(uint8_t* values, uint16_t* dest, uint32_t count, EncodingType type)
    {
        for (uint32_t i=0; i<count; i++)
        {
            uint16_t decoded;
            Color currColor = {values[i*3], values[i*3+1], values[i*3+2]};

            switch (type)
            {
            case EncodingType::TRIANGLE:
                decoded = ColorToTriangle(currColor);
                break;
            case EncodingType::MORTON:
                decoded = ColorToMorton(currColor, CurveOrder<uint16_t>());
                break;
            case EncodingType::HILBERT:
                decoded = ColorToHilbert(currColor, CurveOrder<uint16_t>());
                break;
            case EncodingType::PHASE:
                decoded = ColorToPhase(currColor);
                break;
            case EncodingType::SPLIT:
                decoded = ColorToSplit(currColor);
                break;
            default:
                break;
            }

            // Add color to result
            dest[i] = decoded;
        }

        return true;
    }
}
