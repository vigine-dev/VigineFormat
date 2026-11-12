#pragma once

#include "vigine/format/vigineformat_export.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

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

// The neutral value tree every format reads into and writes from: a kind and the one value that kind holds.
// Asking a value for what it does not hold throws std::logic_error naming both kinds.
class VIGINEFORMAT_API DataValue
{
  public:
    [[nodiscard]] static DataValue table();
    [[nodiscard]] static DataValue array();
    [[nodiscard]] static DataValue string(std::string text);
    [[nodiscard]] static DataValue integer(std::int64_t number);
    [[nodiscard]] static DataValue floating(double number);
    [[nodiscard]] static DataValue boolean(bool flag);
    [[nodiscard]] static DataValue null();

    [[nodiscard]] DataKind kind() const noexcept;

    [[nodiscard]] const std::string &asString() const;
    [[nodiscard]] std::int64_t asInteger() const;
    [[nodiscard]] double asFloating() const;
    [[nodiscard]] bool asBoolean() const;

    // Table: the keys in the order they were set, the value of a key (find: nullptr when absent; at: throws std::out_of_range).
    [[nodiscard]] const std::vector<std::string> &keys() const;
    [[nodiscard]] const DataValue *find(std::string_view key) const;
    [[nodiscard]] const DataValue &at(std::string_view key) const;
    // Sets a key; an existing key takes the new value in its place.
    void set(std::string key, DataValue value);

    // Array: the elements in order.
    [[nodiscard]] const std::vector<DataValue> &elements() const;
    void append(DataValue value);

    // Tables compare by key and value, not by the order of the keys.
    [[nodiscard]] bool operator==(const DataValue &other) const;

  private:
    explicit DataValue(DataKind kind);
    void require(DataKind wanted) const;

    DataKind _kind;
    std::string _text;
    std::int64_t _integer = 0;
    double _floating = 0.0;
    bool _boolean = false;
    std::vector<std::string> _keys;
    std::vector<DataValue> _values;
};
} // namespace vigine::format
