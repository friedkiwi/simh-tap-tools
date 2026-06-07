#pragma once

#include "Error.h"
#include "Result.h"

#include <cstdint>
#include <iosfwd>

namespace tap::simh {

constexpr std::uint32_t TapeMarkWord = 0x00000000U;
constexpr std::uint32_t EraseGapWord = 0xFFFFFFFEU;
constexpr std::uint32_t EndOfMediumWord = 0xFFFFFFFFU;
constexpr std::uint32_t HalfGapForwardWord = 0xFFFEFFFFU;
constexpr std::uint32_t ClassMask = 0xF0000000U;
constexpr std::uint32_t LengthMask = 0x0FFFFFFFU;
constexpr std::uint32_t StandardLengthMask = 0x00FFFFFFU;
constexpr std::uint32_t BadRecordClass = 0x80000000U;
constexpr std::uint32_t TapeDescriptionClass = 0xE0000000U;

std::uint8_t recordClass(std::uint32_t word);
std::uint32_t recordLength(std::uint32_t word);
bool isStandardDataRecord(std::uint32_t word);
bool isMetadataMarker(std::uint32_t word);

Result<std::uint32_t> readWord(std::istream& input, std::uint64_t offset);
Result<void> writeWord(std::ostream& output, std::uint32_t word, std::uint64_t offset);

} // namespace tap::simh
