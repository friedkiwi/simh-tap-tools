#include "TapeImage.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace tap {

const std::vector<TapeElement>& TapeImage::elements() const
{
    return elements_;
}

bool TapeImage::empty() const
{
    return elements_.empty();
}

std::size_t TapeImage::elementCount() const
{
    return elements_.size();
}

std::size_t TapeImage::recordCount() const
{
    return static_cast<std::size_t>(std::count_if(elements_.begin(), elements_.end(), [](const TapeElement& element) {
        return element.isRecord();
    }));
}

std::size_t TapeImage::tapeMarkCount() const
{
    return static_cast<std::size_t>(std::count_if(elements_.begin(), elements_.end(), [](const TapeElement& element) {
        return element.isTapeMark();
    }));
}

void TapeImage::appendRecord(std::vector<std::uint8_t> data)
{
    const auto encoded_length = static_cast<std::uint32_t>(data.size());
    elements_.emplace_back(Record{std::move(data), 0, encoded_length, RecordClass::Good});
}

void TapeImage::appendTapeMark()
{
    elements_.emplace_back(TapeMark{});
}

void TapeImage::appendEraseGap()
{
    elements_.emplace_back(EraseGap{});
}

void TapeImage::appendEndOfMedium()
{
    elements_.emplace_back(EndOfMedium{});
}

void TapeImage::insert(std::size_t index, TapeElement element)
{
    if (index > elements_.size()) {
        throw std::out_of_range("tape element insert index is out of range");
    }
    elements_.insert(elements_.begin() + static_cast<std::vector<TapeElement>::difference_type>(index), std::move(element));
}

void TapeImage::erase(std::size_t index)
{
    if (index >= elements_.size()) {
        throw std::out_of_range("tape element erase index is out of range");
    }
    elements_.erase(elements_.begin() + static_cast<std::vector<TapeElement>::difference_type>(index));
}

void TapeImage::move(std::size_t from, std::size_t to)
{
    if (from >= elements_.size() || to >= elements_.size()) {
        throw std::out_of_range("tape element move index is out of range");
    }
    if (from == to) {
        return;
    }

    auto element = std::move(elements_[from]);
    elements_.erase(elements_.begin() + static_cast<std::vector<TapeElement>::difference_type>(from));
    elements_.insert(elements_.begin() + static_cast<std::vector<TapeElement>::difference_type>(to), std::move(element));
}

} // namespace tap
