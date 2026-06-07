#pragma once

#include "Result.h"
#include "TapeImage.h"

#include <filesystem>
#include <iosfwd>

namespace tap {

class Reader {
public:
    Result<TapeImage> read(std::istream& input) const;
    Result<TapeImage> read(const std::filesystem::path& path) const;
};

} // namespace tap
