#pragma once

#include "Result.h"
#include "TapeImage.h"

#include <filesystem>
#include <iosfwd>

namespace tap {

class Writer {
public:
    Result<void> write(const TapeImage& image, std::ostream& output) const;
    Result<void> write(const TapeImage& image, const std::filesystem::path& path) const;
};

} // namespace tap
