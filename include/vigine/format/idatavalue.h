#pragma once

#include "vigine/format/vigineformat_export.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace vigine::format
{
enum class DataKind : std::uint8_t
{
    Table,
    Array,
    String,
    Integer,
    Float,
    Boolean,
    Null,
};

// The neutral value tree through interfaces alone: built by IDataValueFactory, read and changed here.
// Asking a value for what it does not hold throws std::logic_error naming both kinds.
class VIGINEFORMAT_API IDataValue
{
  public:
    virtual ~IDataValue();

    [[nodiscard]] virtual DataKind kind() const noexcept = 0;

    [[nodiscard]] virtual const std::string &asString() const = 0;
    [[nodiscard]] virtual std::int64_t asInteger() const = 0;
    [[nodiscard]] virtual double asFloating() const = 0;
    [[nodiscard]] virtual bool asBoolean() const = 0;

    // Table: the number of keys; array: the number of elements.
    [[nodiscard]] virtual std::size_t size() const = 0;
    // Table: the key at an index in the order the keys were set; throws std::out_of_range past the end.
    [[nodiscard]] virtual std::string_view keyAt(std::size_t index) const = 0;
    // Table: find gives nullptr when the key is absent; at throws std::out_of_range.
    [[nodiscard]] virtual const IDataValue *find(std::string_view key) const = 0;
    [[nodiscard]] virtual const IDataValue &at(std::string_view key) const = 0;
    [[nodiscard]] virtual IDataValue *find(std::string_view key) = 0;
    [[nodiscard]] virtual IDataValue &at(std::string_view key) = 0;
    // Sets a key; an existing key takes the new value in its place.
    virtual void set(std::string_view key, std::unique_ptr<IDataValue> value) = 0;

    // Array: the element at an index; throws std::out_of_range past the end.
    [[nodiscard]] virtual const IDataValue &elementAt(std::size_t index) const = 0;
    [[nodiscard]] virtual IDataValue &elementAt(std::size_t index) = 0;
    virtual void append(std::unique_ptr<IDataValue> value) = 0;

    [[nodiscard]] virtual std::unique_ptr<IDataValue> clone() const = 0;
    // Tables compare by key and value, not by the order of the keys.
    [[nodiscard]] virtual bool equals(const IDataValue &other) const = 0;

  protected:
    IDataValue() = default;
    IDataValue(const IDataValue &) = default;
    IDataValue &operator=(const IDataValue &) = default;
};
} // namespace vigine::format
