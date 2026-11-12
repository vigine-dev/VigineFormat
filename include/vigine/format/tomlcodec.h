#pragma once

#include "vigine/format/vigineformat_export.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace vigine::format
{
enum class TomlKind : std::uint8_t
{
    Table,
    Array,
    String,
    Integer,
    Floating,
    Boolean,
};

class TomlValue;
class TomlParseError;
class ITomlCodec;

// A neutral TOML value: a kind and the one value that kind holds. Dates and times are Strings in ISO-8601 form.
// Asking a value for what it does not hold throws std::logic_error naming both kinds.
class VIGINEFORMAT_API TomlValue
{
  public:
    [[nodiscard]] static TomlValue table();
    [[nodiscard]] static TomlValue array();
    [[nodiscard]] static TomlValue string(std::string text);
    [[nodiscard]] static TomlValue integer(std::int64_t number);
    [[nodiscard]] static TomlValue floating(double number);
    [[nodiscard]] static TomlValue boolean(bool flag);

    [[nodiscard]] TomlKind kind() const noexcept;

    [[nodiscard]] const std::string &asString() const;
    [[nodiscard]] std::int64_t asInteger() const;
    [[nodiscard]] double asFloating() const;
    [[nodiscard]] bool asBoolean() const;

    // Table: the keys in the order they were set, the value of a key (find: nullptr when absent; at: throws std::out_of_range).
    [[nodiscard]] const std::vector<std::string> &keys() const;
    [[nodiscard]] const TomlValue *find(std::string_view key) const;
    [[nodiscard]] const TomlValue &at(std::string_view key) const;
    // Sets a key; an existing key takes the new value in its place.
    void set(std::string key, TomlValue value);

    // Array: the elements in order.
    [[nodiscard]] const std::vector<TomlValue> &elements() const;
    void append(TomlValue value);

    // Tables compare by key and value, not by the order of the keys.
    [[nodiscard]] bool operator==(const TomlValue &other) const;

  private:
    explicit TomlValue(TomlKind kind);
    void require(TomlKind wanted) const;

    TomlKind _kind;
    std::string _text;
    std::int64_t _integer = 0;
    double _floating = 0.0;
    bool _boolean = false;
    std::vector<std::string> _keys;
    std::vector<TomlValue> _values;
};

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
    [[nodiscard]] virtual TomlValue parse(std::string_view text) const = 0;
    // A Table value to TOML text. Throws std::invalid_argument when the root is not a table.
    [[nodiscard]] virtual std::string serialize(const TomlValue &root) const = 0;
};

[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<ITomlCodec> createTomlCodec();

} // namespace vigine::format
