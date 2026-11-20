#pragma once

#include "vigine/format/datavalue.h"
#include "vigine/format/idatavalue.h"
#include "vigine/format/vigineformat_export.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace vigine::format
{
// Thrown by parse for malformed JSON; the message holds the reason, the line and the column.
class VIGINEFORMAT_API JsonParseError : public std::runtime_error
{
  public:
    JsonParseError(const std::string &message, std::uint32_t line, std::uint32_t column);

    [[nodiscard]] std::uint32_t line() const noexcept;
    [[nodiscard]] std::uint32_t column() const noexcept;

  private:
    std::uint32_t _line;
    std::uint32_t _column;
};

// How serialize writes: indent >= 0 gives the same text as nlohmann's dump(indent), -1 is compact; sortKeys writes
// the keys of every table in byte order at every depth; replaceInvalidUtf8 writes U+FFFD for each invalid sequence.
struct JsonWriteOptions
{
    int indent = -1;
    bool sortKeys = false;
    bool replaceInvalidUtf8 = false;
};

class VIGINEFORMAT_API IJsonCodec
{
  public:
    virtual ~IJsonCodec();

    // JSON text to a value of any kind. Throws JsonParseError on malformed input.
    [[nodiscard]] virtual DataValue parse(std::string_view text) const = 0;
    // A value of any kind to compact JSON text. Throws std::invalid_argument for a float JSON cannot write (NaN, infinity).
    [[nodiscard]] virtual std::string serialize(const DataValue &root) const = 0;
    // The same text as serialize, but each invalid UTF-8 sequence in a string is written as U+FFFD instead of throwing.
    [[nodiscard]] virtual std::string serializeReplacingInvalidUtf8(const DataValue &root) const = 0;

    // The same through the interfaces alone. Throws JsonParseError / std::invalid_argument as parse / serialize do.
    [[nodiscard]] virtual std::unique_ptr<IDataValue> parseValue(std::string_view text) const = 0;
    [[nodiscard]] virtual std::string serialize(const IDataValue &root, const JsonWriteOptions &options) const = 0;
};

[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IJsonCodec> createJsonCodec();

} // namespace vigine::format
