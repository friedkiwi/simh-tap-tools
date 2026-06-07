#include "Reader.h"

#include "Scanner.h"

#include <fstream>

namespace tap {

Result<TapeImage> Reader::read(std::istream& input) const
{
    Scanner scanner;
    auto scan_result = scanner.scan(input);
    if (!scan_result.diagnostics.empty()) {
        return Result<TapeImage>::fail(scan_result.diagnostics.front());
    }

    return Result<TapeImage>::ok(std::move(scan_result.image));
}

Result<TapeImage> Reader::read(const std::filesystem::path& path) const
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return Result<TapeImage>::fail(Error{
            ErrorCode::IoError,
            "failed to open SIMH tape file for reading",
            0,
        });
    }

    return read(input);
}

} // namespace tap
