#include "vigine/format/datavalue.h"

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
    if (_kind != other._kind)
    {
        return false;
    }
    switch (_kind)
    {
    case DataKind::Table:
        if (_keys.size() != other._keys.size())
        {
            return false;
        }
        for (std::size_t index = 0; index < _keys.size(); ++index)
        {
            const DataValue *counterpart = other.find(_keys[index]);
            if (counterpart == nullptr || !(_values[index] == *counterpart))
            {
                return false;
            }
        }
        return true;
    case DataKind::Array:
        return _values == other._values;
    case DataKind::String:
        return _text == other._text;
    case DataKind::Integer:
        return _integer == other._integer;
    case DataKind::Float:
        return _floating == other._floating;
    case DataKind::Boolean:
        return _boolean == other._boolean;
    case DataKind::Null:
        return true;
    }
    throw std::logic_error("DataValue: unknown kind");
}
} // namespace vigine::format
