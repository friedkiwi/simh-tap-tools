#pragma once

#include <cstdint>
#include <variant>
#include <vector>

namespace tap {

enum class ElementKind {
    DataRecord,
    TapeMark,
    EraseGap,
    EndOfMedium
};

enum class RecordClass : std::uint8_t {
    Good = 0x0,
    Bad = 0x8,
    TapeDescription = 0xE
};

struct Record {
    std::vector<std::uint8_t> data;
    std::uint64_t offset = 0;
    std::uint32_t encoded_length = 0;
    RecordClass record_class = RecordClass::Good;
};

struct TapeMark {
    std::uint64_t offset = 0;
};

struct EraseGap {
    std::uint64_t offset = 0;
};

struct EndOfMedium {
    std::uint64_t offset = 0;
};

class TapeElement {
public:
    TapeElement(Record record);
    TapeElement(TapeMark tape_mark);
    TapeElement(EraseGap erase_gap);
    TapeElement(EndOfMedium end_of_medium);

    ElementKind kind() const;

    bool isRecord() const;
    bool isTapeMark() const;
    bool isEraseGap() const;
    bool isEndOfMedium() const;

    const Record& record() const;
    Record& record();
    const TapeMark& tapeMark() const;
    const EraseGap& eraseGap() const;
    const EndOfMedium& endOfMedium() const;

private:
    using Storage = std::variant<Record, TapeMark, EraseGap, EndOfMedium>;

    Storage storage_;
};

} // namespace tap
