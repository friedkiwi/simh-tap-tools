#pragma once

#include "RecordParser.h"
#include "tap/Progress.h"

#include <cstddef>
#include <string>
#include <vector>

namespace tap {
class TapeImage;
}

namespace as400 {

struct FileListEntry {
    std::size_t element_index = 0;
    RecordInfo record;
    std::string file_name;
    std::string set;
    std::string section;
    std::string sequence;
    std::string generation;
    std::string created;
    std::string expires;
    std::string system;
};

std::vector<FileListEntry> collectAs400FileList(
    const tap::TapeImage& image,
    const RecordParser& parser,
    const tap::ProgressCallback& progress = {});

} // namespace as400
