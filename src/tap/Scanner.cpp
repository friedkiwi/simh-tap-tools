#include "Scanner.h"

#include "SimhFormat.h"

#include <fstream>
#include <limits>
#include <utility>

namespace tap {
namespace {

RecordClass toRecordClass(std::uint32_t word)
{
    switch (simh::recordClass(word)) {
    case 0x8:
        return RecordClass::Bad;
    case 0xE:
        return RecordClass::TapeDescription;
    default:
        return RecordClass::Good;
    }
}

bool readPayload(std::istream& input, std::vector<std::uint8_t>& data, std::uint32_t padded_length)
{
    data.resize(padded_length);
    input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return input.gcount() == static_cast<std::streamsize>(data.size());
}

} // namespace

ScanResult Scanner::scan(std::istream& input) const
{
    ScanResult result;
    std::uint64_t offset = 0;

    while (true) {
        if (input.peek() == std::char_traits<char>::eof()) {
            break;
        }

        const auto word_result = simh::readWord(input, offset);
        if (!word_result) {
            result.diagnostics.push_back(word_result.error());
            break;
        }

        const auto word = word_result.value();
        if (word == simh::TapeMarkWord) {
            result.image.insert(result.image.elementCount(), TapeElement(TapeMark{offset}));
            offset += 4;
            continue;
        }
        if (word == simh::EraseGapWord) {
            result.image.insert(result.image.elementCount(), TapeElement(EraseGap{offset}));
            offset += 4;
            continue;
        }
        if (word == simh::EndOfMediumWord) {
            result.image.insert(result.image.elementCount(), TapeElement(EndOfMedium{offset}));
            break;
        }

        if (!simh::isStandardDataRecord(word)) {
            result.diagnostics.push_back(Error{
                ErrorCode::UnsupportedFormat,
                "unsupported SIMH tape object class or marker",
                offset,
            });
            break;
        }

        const auto length = simh::recordLength(word);
        if (length > simh::StandardLengthMask) {
            result.diagnostics.push_back(Error{
                ErrorCode::InvalidRecordLength,
                "SIMH standard record length exceeds 24 bits",
                offset,
            });
            break;
        }

        const auto padded_length = (length + 1U) & ~1U;
        std::vector<std::uint8_t> payload;
        if (!readPayload(input, payload, padded_length)) {
            result.diagnostics.push_back(Error{
                ErrorCode::UnexpectedEof,
                "unexpected end of file while reading SIMH tape record payload",
                offset,
            });
            break;
        }
        payload.resize(length);

        const auto trailer_offset = offset + 4 + padded_length;
        const auto trailer_result = simh::readWord(input, trailer_offset);
        if (!trailer_result) {
            result.diagnostics.push_back(trailer_result.error());
            break;
        }

        if (trailer_result.value() != word) {
            result.diagnostics.push_back(Error{
                ErrorCode::MismatchedRecordTrailer,
                "SIMH tape record trailing length does not match leading length",
                trailer_offset,
            });
            break;
        }

        Record record;
        record.data = std::move(payload);
        record.offset = offset;
        record.encoded_length = word;
        record.record_class = toRecordClass(word);
        result.image.insert(result.image.elementCount(), TapeElement(std::move(record)));

        offset = trailer_offset + 4;
    }

    return result;
}

ScanResult Scanner::scan(const std::filesystem::path& path) const
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        ScanResult result;
        result.diagnostics.push_back(Error{
            ErrorCode::IoError,
            "failed to open SIMH tape file for reading",
            0,
        });
        return result;
    }

    return scan(input);
}

} // namespace tap
