#include "tap/Reader.h"
#include "tap/Scanner.h"
#include "tap/Writer.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string bytes(std::initializer_list<unsigned char> values)
{
    std::string result;
    result.reserve(values.size());
    for (const auto value : values) {
        result.push_back(static_cast<char>(value));
    }
    return result;
}

void testTapeImageEditing()
{
    tap::TapeImage image;
    assert(image.empty());

    image.appendRecord({0x01, 0x02});
    image.appendTapeMark();
    image.appendRecord({0x03});

    assert(image.elementCount() == 3);
    assert(image.recordCount() == 2);
    assert(image.tapeMarkCount() == 1);

    image.move(2, 0);
    assert(image.elements()[0].isRecord());
    assert(image.elements()[0].record().data[0] == 0x03);

    image.erase(2);
    assert(image.elementCount() == 2);
    assert(image.recordCount() == 2);
    assert(image.tapeMarkCount() == 0);
}

void testRoundTripWithOddRecord()
{
    tap::TapeImage image;
    image.appendRecord({0x41, 0x42, 0x43});
    image.appendTapeMark();
    image.appendEraseGap();
    image.appendEndOfMedium();

    std::ostringstream output(std::ios::binary);
    tap::Writer writer;
    const auto write_result = writer.write(image, output);
    assert(write_result);

    const auto encoded = output.str();
    const auto expected_prefix = bytes({
        0x03, 0x00, 0x00, 0x00,
        0x41, 0x42, 0x43, 0x00,
        0x03, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xFE, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
    });
    assert(encoded == expected_prefix);

    std::istringstream input(encoded, std::ios::binary);
    tap::Reader reader;
    auto read_result = reader.read(input);
    assert(read_result);

    const auto& round_trip = read_result.value();
    assert(round_trip.elementCount() == 4);
    assert(round_trip.recordCount() == 1);
    assert(round_trip.tapeMarkCount() == 1);
    assert(round_trip.elements()[0].record().data == std::vector<std::uint8_t>({0x41, 0x42, 0x43}));
    assert(round_trip.elements()[2].isEraseGap());
    assert(round_trip.elements()[3].isEndOfMedium());
}

void testMismatchedTrailerFailsStrictRead()
{
    const auto encoded = bytes({
        0x03, 0x00, 0x00, 0x00,
        0x41, 0x42, 0x43, 0x00,
        0x04, 0x00, 0x00, 0x00,
    });

    std::istringstream input(encoded, std::ios::binary);
    tap::Reader reader;
    const auto result = reader.read(input);
    assert(!result);
    assert(result.error().code == tap::ErrorCode::MismatchedRecordTrailer);
    assert(result.error().offset == 8);
}

void testSampleScannerReportsValidPrefixAndDiagnostic()
{
    const auto sample_path = std::filesystem::path(TAP_TEST_DATA_DIR) / "blank.tap";
    tap::Scanner scanner;
    const auto result = scanner.scan(sample_path);

    assert(result.image.recordCount() == 1);
    assert(result.image.tapeMarkCount() == 21);
    assert(!result.diagnostics.empty());
    assert(result.diagnostics.front().code == tap::ErrorCode::UnexpectedEof);
    assert(result.diagnostics.front().offset == 172);

    tap::Reader reader;
    const auto strict_read = reader.read(sample_path);
    assert(!strict_read);
    assert(strict_read.error().code == tap::ErrorCode::UnexpectedEof);
}

} // namespace

int main()
{
    testTapeImageEditing();
    testRoundTripWithOddRecord();
    testMismatchedTrailerFailsStrictRead();
    testSampleScannerReportsValidPrefixAndDiagnostic();
    return 0;
}
