#pragma once

#include "Error.h"
#include "TapeImage.h"

#include <filesystem>
#include <iosfwd>
#include <vector>

namespace tap {

struct ScanResult {
    TapeImage image;
    std::vector<Error> diagnostics;
};

class Scanner {
public:
    ScanResult scan(std::istream& input) const;
    ScanResult scan(const std::filesystem::path& path) const;
};

} // namespace tap
