#include "vigine/format/datavalue.h"

#include "vigine/format/idatavaluefactory.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace vigine::format
{
const char *kindName(DataKind kind)
{
    switch (kind)
    {
    case DataKind::Table:
        return "table";
    case DataKind::Array:
        return "array";
    case DataKind::String:
        return "string";
    case DataKind::Integer:
        return "integer";
    case DataKind::Float:
        return "float";
    case DataKind::Boolean:
        return "boolean";
    case DataKind::Null:
        return "null";
    }
    throw std::logic_error("DataValue: unknown kind");
}

DataValue::DataValue(DataKind kind) : _kind(kind) {}

DataValue DataValue::table()
{
    return DataValue(DataKind::Table);
}

DataValue DataValue::array()
{
    return DataValue(DataKind::Array);
}

DataValue DataValue::string(std::string text)
{
    DataValue value(DataKind::String);
    value._text = std::move(text);
    return value;
}

DataValue DataValue::integer(std::int64_t number)
{
    DataValue value(DataKind::Integer);
    value._integer = number;
    return value;
}

DataValue DataValue::floating(double number)
{
    DataValue value(DataKind::Float);
    value._floating = number;
    return value;
}

DataValue DataValue::null()
{
    return DataValue(DataKind::Null);
}

DataValue DataValue::boolean(bool flag)
{
    DataValue value(DataKind::Boolean);
    value._boolean = flag;
    return value;
}

DataKind DataValue::kind() const noexcept
{
    return _kind;
}

void DataValue::require(DataKind wanted) const
{
    if (_kind != wanted)
    {
        throw std::logic_error(std::string("DataValue: a ") + kindName(_kind) + " was asked for as a "
                               + kindName(wanted));
    }
}

const std::string &DataValue::asString() const
{
    require(DataKind::String);
    return _text;
}

std::int64_t DataValue::asInteger() const
{
    require(DataKind::Integer);
    return _integer;
}

double DataValue::asFloating() const
{
    require(DataKind::Float);
    return _floating;
}

bool DataValue::asBoolean() const
{
    require(DataKind::Boolean);
    return _boolean;
}

const std::vector<std::string> &DataValue::keys() const
{
    require(DataKind::Table);
    return _keys;
}

const DataValue *DataValue::find(std::string_view key) const
{
    require(DataKind::Table);
    const auto found = std::find(_keys.begin(), _keys.end(), key);
    return found == _keys.end() ? nullptr : &_values[static_cast<std::size_t>(found - _keys.begin())];
}

const DataValue &DataValue::at(std::string_view key) const
{
    const DataValue *found = find(key);
    if (found == nullptr)
    {
        throw std::out_of_range("DataValue: the table has no key " + std::string(key));
    }
    return *found;
}

DataValue *DataValue::find(std::string_view key)
{
    return const_cast<DataValue *>(std::as_const(*this).find(key));
}

DataValue &DataValue::at(std::string_view key)
{
    return const_cast<DataValue &>(std::as_const(*this).at(key));
}

void DataValue::set(std::string key, DataValue value)
{
    require(DataKind::Table);
    const auto found = std::find(_keys.begin(), _keys.end(), key);
    if (found != _keys.end())
    {
        _values[static_cast<std::size_t>(found - _keys.begin())] = std::move(value);
        return;
    }
    _keys.push_back(std::move(key));
    _values.push_back(std::move(value));
}

const std::vector<DataValue> &DataValue::elements() const
{
    require(DataKind::Array);
    return _values;
}

void DataValue::append(DataValue value)
{
    require(DataKind::Array);
    _values.push_back(std::move(value));
}

bool DataValue::operator==(const DataValue &other) const
{
    return equals(other);
}

std::size_t DataValue::size() const
{
    if (_kind != DataKind::Table && _kind != DataKind::Array)
    {
        throw std::logic_error(std::string("DataValue: a ") + kindName(_kind) + " has no size");
    }
    return _kind == DataKind::Table ? _keys.size() : _values.size();
}

std::string_view DataValue::keyAt(std::size_t index) const
{
    const std::vector<std::string> &all = keys();
    if (index >= all.size())
    {
        throw std::out_of_range("DataValue: the table has no key at index " + std::to_string(index));
    }
    return all[index];
}

const DataValue &DataValue::elementAt(std::size_t index) const
{
    const std::vector<DataValue> &all = elements();
    if (index >= all.size())
    {
        throw std::out_of_range("DataValue: the array has no element at index " + std::to_string(index));
    }
    return all[index];
}

DataValue &DataValue::elementAt(std::size_t index)
{
    return const_cast<DataValue &>(std::as_const(*this).elementAt(index));
}

std::unique_ptr<IDataValue> DataValue::clone() const
{
    return std::make_unique<DataValue>(*this);
}

bool DataValue::equals(const IDataValue &other) const
{
    if (_kind != other.kind())
    {
        return false;
    }
    switch (_kind)
    {
    case DataKind::Table:
        if (_keys.size() != other.size())
        {
            return false;
        }
        for (std::size_t index = 0; index < _keys.size(); ++index)
        {
            const IDataValue *counterpart = other.find(_keys[index]);
            if (counterpart == nullptr || !_values[index].equals(*counterpart))
            {
                return false;
            }
        }
        return true;
    case DataKind::Array:
        if (_values.size() != other.size())
        {
            return false;
        }
        for (std::size_t index = 0; index < _values.size(); ++index)
        {
            if (!_values[index].equals(other.elementAt(index)))
            {
                return false;
            }
        }
        return true;
    case DataKind::String:
        return _text == other.asString();
    case DataKind::Integer:
        return _integer == other.asInteger();
    case DataKind::Float:
        return _floating == other.asFloating();
    case DataKind::Boolean:
        return _boolean == other.asBoolean();
    case DataKind::Null:
        return true;
    }
    throw std::logic_error("DataValue: unknown kind");
}

DataValue::DataValue(const DataValue &other) = default;
DataValue::DataValue(DataValue &&other) noexcept = default;
DataValue &DataValue::operator=(const DataValue &other) = default;
DataValue &DataValue::operator=(DataValue &&other) noexcept = default;
DataValue::~DataValue() = default;

IDataValue::~IDataValue() = default;

namespace
{
DataValue copyOf(const IDataValue &source)
{
    switch (source.kind())
    {
    case DataKind::Table:
    {
        DataValue result = DataValue::table();
        for (std::size_t index = 0; index < source.size(); ++index)
        {
            const std::string_view key = source.keyAt(index);
            result.set(std::string(key), copyOf(source.at(key)));
        }
        return result;
    }
    case DataKind::Array:
    {
        DataValue result = DataValue::array();
        for (std::size_t index = 0; index < source.size(); ++index)
        {
            result.append(copyOf(source.elementAt(index)));
        }
        return result;
    }
    case DataKind::String:
        return DataValue::string(source.asString());
    case DataKind::Integer:
        return DataValue::integer(source.asInteger());
    case DataKind::Float:
        return DataValue::floating(source.asFloating());
    case DataKind::Boolean:
        return DataValue::boolean(source.asBoolean());
    case DataKind::Null:
        return DataValue::null();
    }
    throw std::logic_error("DataValue: unknown kind");
}

DataValue takeOver(std::unique_ptr<IDataValue> value)
{
    if (value == nullptr)
    {
        throw std::invalid_argument("DataValue: an empty interface value was given");
    }
    if (auto *own = dynamic_cast<DataValue *>(value.get()))
    {
        return std::move(*own);
    }
    return copyOf(*value);
}

class DataValueFactory final : public IDataValueFactory
{
  public:
    std::unique_ptr<IDataValue> table() const override
    {
        return std::make_unique<DataValue>(DataValue::table());
    }
    std::unique_ptr<IDataValue> array() const override
    {
        return std::make_unique<DataValue>(DataValue::array());
    }
    std::unique_ptr<IDataValue> string(std::string_view text) const override
    {
        return std::make_unique<DataValue>(DataValue::string(std::string(text)));
    }
    std::unique_ptr<IDataValue> integer(std::int64_t number) const override
    {
        return std::make_unique<DataValue>(DataValue::integer(number));
    }
    std::unique_ptr<IDataValue> floating(double number) const override
    {
        return std::make_unique<DataValue>(DataValue::floating(number));
    }
    std::unique_ptr<IDataValue> boolean(bool flag) const override
    {
        return std::make_unique<DataValue>(DataValue::boolean(flag));
    }
    std::unique_ptr<IDataValue> null() const override
    {
        return std::make_unique<DataValue>(DataValue::null());
    }
};
} // namespace

void DataValue::set(std::string_view key, std::unique_ptr<IDataValue> value)
{
    set(std::string(key), takeOver(std::move(value)));
}

void DataValue::append(std::unique_ptr<IDataValue> value)
{
    append(takeOver(std::move(value)));
}

IDataValueFactory::~IDataValueFactory() = default;

std::unique_ptr<IDataValueFactory> createDataValueFactory()
{
    return std::make_unique<DataValueFactory>();
}
} // namespace vigine::format
