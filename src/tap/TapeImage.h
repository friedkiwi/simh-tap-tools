#pragma once

#include "TapeElement.h"

#include <cstddef>
#include <vector>

namespace tap {

class TapeImage {
public:
    const std::vector<TapeElement>& elements() const;

    bool empty() const;
    std::size_t elementCount() const;
    std::size_t recordCount() const;
    std::size_t tapeMarkCount() const;

    void appendRecord(std::vector<std::uint8_t> data);
    void appendTapeMark();
    void appendEraseGap();
    void appendEndOfMedium();

    void insert(std::size_t index, TapeElement element);
    void erase(std::size_t index);
    void move(std::size_t from, std::size_t to);

private:
    std::vector<TapeElement> elements_;
};

} // namespace tap
