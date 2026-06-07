#include "SimhFormat.h"

#include <array>
#include <istream>
#include <ostream>

namespace tap::simh {

std::uint8_t recordClass(std::uint32_t word)
{
    return static_cast<std::uint8_t>((word & ClassMask) >> 28U);
}

std::uint32_t recordLength(std::uint32_t word)
{
    return word & LengthMask;
}

bool isStandardDataRecord(std::uint32_t word)
{
    const auto cls = word & ClassMask;
    const auto length = recordLength(word);
    return length > 0 && (cls == 0 || cls == BadRecordClass || cls == TapeDescriptionClass);
}

bool isMetadataMarker(std::uint32_t word)
{
    return word == TapeMarkWord || word == EraseGapWord || word == EndOfMediumWord;
}

Result<std::uint32_t> readWord(std::istream& input, std::uint64_t offset)
{
    std::array<unsigned char, 4> bytes{};
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (input.gcount() != static_cast<std::streamsize>(bytes.size())) {
        return Result<std::uint32_t>::fail(Error{
            ErrorCode::UnexpectedEof,
            "unexpected end of file while reading SIMH tape word",
            offset,
        });
    }

    const auto value = static_cast<std::uint32_t>(bytes[0])
        | (static_cast<std::uint32_t>(bytes[1]) << 8U)
        | (static_cast<std::uint32_t>(bytes[2]) << 16U)
        | (static_cast<std::uint32_t>(bytes[3]) << 24U);
    return Result<std::uint32_t>::ok(value);
}

Result<void> writeWord(std::ostream& output, std::uint32_t word, std::uint64_t offset)
{
    const std::array<unsigned char, 4> bytes{
        static_cast<unsigned char>(word & 0xFFU),
        static_cast<unsigned char>((word >> 8U) & 0xFFU),
        static_cast<unsigned char>((word >> 16U) & 0xFFU),
        static_cast<unsigned char>((word >> 24U) & 0xFFU),
    };

    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!output) {
        return Result<void>::fail(Error{
            ErrorCode::IoError,
            "failed to write SIMH tape word",
            offset,
        });
    }

    return Result<void>::ok();
}

} // namespace tap::simh
