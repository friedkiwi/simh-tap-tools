#include "Writer.h"

#include "SimhFormat.h"

#include <fstream>

namespace tap {
namespace {

std::uint32_t encodeRecordWord(const Record& record)
{
    std::uint32_t class_bits = 0;
    if (record.record_class == RecordClass::Bad) {
        class_bits = simh::BadRecordClass;
    } else if (record.record_class == RecordClass::TapeDescription) {
        class_bits = simh::TapeDescriptionClass;
    }

    return class_bits | static_cast<std::uint32_t>(record.data.size());
}

} // namespace

Result<void> Writer::write(const TapeImage& image, std::ostream& output) const
{
    std::uint64_t offset = 0;

    for (const auto& element : image.elements()) {
        if (element.isTapeMark()) {
            auto result = simh::writeWord(output, simh::TapeMarkWord, offset);
            if (!result) {
                return result;
            }
            offset += 4;
            continue;
        }

        if (element.isEraseGap()) {
            auto result = simh::writeWord(output, simh::EraseGapWord, offset);
            if (!result) {
                return result;
            }
            offset += 4;
            continue;
        }

        if (element.isEndOfMedium()) {
            auto result = simh::writeWord(output, simh::EndOfMediumWord, offset);
            if (!result) {
                return result;
            }
            offset += 4;
            continue;
        }

        const auto& record = element.record();
        if (record.data.size() > simh::StandardLengthMask) {
            return Result<void>::fail(Error{
                ErrorCode::InvalidRecordLength,
                "SIMH standard record length exceeds 24 bits",
                offset,
            });
        }

        const auto word = encodeRecordWord(record);
        auto result = simh::writeWord(output, word, offset);
        if (!result) {
            return result;
        }
        offset += 4;

        if (!record.data.empty()) {
            output.write(reinterpret_cast<const char*>(record.data.data()), static_cast<std::streamsize>(record.data.size()));
            if (!output) {
                return Result<void>::fail(Error{
                    ErrorCode::IoError,
                    "failed to write SIMH tape record payload",
                    offset,
                });
            }
            offset += record.data.size();
        }

        if ((record.data.size() % 2U) != 0U) {
            const char pad = 0;
            output.write(&pad, 1);
            if (!output) {
                return Result<void>::fail(Error{
                    ErrorCode::IoError,
                    "failed to write SIMH tape record pad byte",
                    offset,
                });
            }
            offset += 1;
        }

        result = simh::writeWord(output, word, offset);
        if (!result) {
            return result;
        }
        offset += 4;
    }

    return Result<void>::ok();
}

Result<void> Writer::write(const TapeImage& image, const std::filesystem::path& path) const
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return Result<void>::fail(Error{
            ErrorCode::IoError,
            "failed to open SIMH tape file for writing",
            0,
        });
    }

    return write(image, output);
}

} // namespace tap
