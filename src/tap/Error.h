#pragma once

#include <cstdint>
#include <string>

namespace tap {

enum class ErrorCode {
    IoError,
    UnexpectedEof,
    InvalidRecordLength,
    MismatchedRecordTrailer,
    UnsupportedFormat
};

struct Error {
    ErrorCode code;
    std::string message;
    std::uint64_t offset = 0;
};

} // namespace tap
