#pragma once

#include "vigine/format/datavalue.h"
#include "vigine/format/vigineformat_export.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace vigine::format
{
// Thrown by parse for malformed TOML; the message holds the reason, the line and the column.
class VIGINEFORMAT_API TomlParseError : public std::runtime_error
{
  public:
    TomlParseError(const std::string &message, std::uint32_t line, std::uint32_t column);

    [[nodiscard]] std::uint32_t line() const noexcept;
    [[nodiscard]] std::uint32_t column() const noexcept;

  private:
    std::uint32_t _line;
    std::uint32_t _column;
};

class VIGINEFORMAT_API ITomlCodec
{
  public:
    virtual ~ITomlCodec() = default;

    // TOML text to a Table value. Throws TomlParseError on malformed input.
    [[nodiscard]] virtual DataValue parse(std::string_view text) const = 0;
    // A Table value to TOML text. Throws std::invalid_argument when the root is not a table.
    [[nodiscard]] virtual std::string serialize(const DataValue &root) const = 0;
};

[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<ITomlCodec> createTomlCodec();

} // namespace vigine::format
