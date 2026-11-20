#pragma once

#include "vigine/format/idatavalue.h"
#include "vigine/format/vigineformat_export.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace vigine::format
{
[[nodiscard]] VIGINEFORMAT_API const char *kindName(DataKind kind);

// The neutral value tree every format reads into and writes from: a kind and the one value that kind holds.
// Asking a value for what it does not hold throws std::logic_error naming both kinds.
class VIGINEFORMAT_API DataValue final : public IDataValue
{
  public:
    [[nodiscard]] static DataValue table();
    [[nodiscard]] static DataValue array();
    [[nodiscard]] static DataValue string(std::string text);
    [[nodiscard]] static DataValue integer(std::int64_t number);
    [[nodiscard]] static DataValue floating(double number);
    [[nodiscard]] static DataValue boolean(bool flag);
    [[nodiscard]] static DataValue null();

    DataValue(const DataValue &other);
    DataValue(DataValue &&other) noexcept;
    DataValue &operator=(const DataValue &other);
    DataValue &operator=(DataValue &&other) noexcept;
    ~DataValue() override;

    [[nodiscard]] DataKind kind() const noexcept override;

    [[nodiscard]] const std::string &asString() const override;
    [[nodiscard]] std::int64_t asInteger() const override;
    [[nodiscard]] double asFloating() const override;
    [[nodiscard]] bool asBoolean() const override;

    [[nodiscard]] std::size_t size() const override;
    [[nodiscard]] std::string_view keyAt(std::size_t index) const override;
    [[nodiscard]] const DataValue &elementAt(std::size_t index) const override;
    [[nodiscard]] DataValue &elementAt(std::size_t index) override;
    [[nodiscard]] std::unique_ptr<IDataValue> clone() const override;
    [[nodiscard]] bool equals(const IDataValue &other) const override;

    // Table: the keys in the order they were set, the value of a key (find: nullptr when absent; at: throws std::out_of_range).
    [[nodiscard]] const std::vector<std::string> &keys() const;
    [[nodiscard]] const DataValue *find(std::string_view key) const override;
    [[nodiscard]] const DataValue &at(std::string_view key) const override;
    // The same lookups for a value that is changed in place.
    [[nodiscard]] DataValue *find(std::string_view key) override;
    [[nodiscard]] DataValue &at(std::string_view key) override;
    // Sets a key; an existing key takes the new value in its place.
    void set(std::string key, DataValue value);
    // An interface value is kept as a DataValue: moved when it is one, copied through the interface when it is not.
    void set(std::string_view key, std::unique_ptr<IDataValue> value) override;

    // Array: the elements in order.
    [[nodiscard]] const std::vector<DataValue> &elements() const;
    void append(DataValue value);
    void append(std::unique_ptr<IDataValue> value) override;

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
