#include "TapeElement.h"

#include <utility>

namespace tap {

TapeElement::TapeElement(Record record)
    : storage_(std::move(record))
{
}

TapeElement::TapeElement(TapeMark tape_mark)
    : storage_(tape_mark)
{
}

TapeElement::TapeElement(EraseGap erase_gap)
    : storage_(erase_gap)
{
}

TapeElement::TapeElement(EndOfMedium end_of_medium)
    : storage_(end_of_medium)
{
}

ElementKind TapeElement::kind() const
{
    if (std::holds_alternative<Record>(storage_)) {
        return ElementKind::DataRecord;
    }
    if (std::holds_alternative<TapeMark>(storage_)) {
        return ElementKind::TapeMark;
    }
    if (std::holds_alternative<EraseGap>(storage_)) {
        return ElementKind::EraseGap;
    }
    return ElementKind::EndOfMedium;
}

bool TapeElement::isRecord() const
{
    return std::holds_alternative<Record>(storage_);
}

bool TapeElement::isTapeMark() const
{
    return std::holds_alternative<TapeMark>(storage_);
}

bool TapeElement::isEraseGap() const
{
    return std::holds_alternative<EraseGap>(storage_);
}

bool TapeElement::isEndOfMedium() const
{
    return std::holds_alternative<EndOfMedium>(storage_);
}

const Record& TapeElement::record() const
{
    return std::get<Record>(storage_);
}

Record& TapeElement::record()
{
    return std::get<Record>(storage_);
}

const TapeMark& TapeElement::tapeMark() const
{
    return std::get<TapeMark>(storage_);
}

const EraseGap& TapeElement::eraseGap() const
{
    return std::get<EraseGap>(storage_);
}

const EndOfMedium& TapeElement::endOfMedium() const
{
    return std::get<EndOfMedium>(storage_);
}

} // namespace tap
