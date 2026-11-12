#include "vigine/format/tomlcodec.h"

#include <algorithm>
#include <sstream>
#include <toml++/toml.hpp>
#include <utility>

namespace vigine::format
{
namespace
{
const char *kindName(TomlKind kind)
{
    switch (kind)
    {
    case TomlKind::Table:
        return "table";
    case TomlKind::Array:
        return "array";
    case TomlKind::String:
        return "string";
    case TomlKind::Integer:
        return "integer";
    case TomlKind::Floating:
        return "floating";
    case TomlKind::Boolean:
        return "boolean";
    }
    throw std::logic_error("TomlValue: unknown kind");
}

template <typename Temporal> TomlValue temporalText(const Temporal &temporal)
{
    std::ostringstream stream;
    stream << temporal;
    return TomlValue::string(stream.str());
}

TomlValue fromTable(const toml::table &source);
TomlValue fromArray(const toml::array &source);

TomlValue fromNode(const toml::node &node)
{
    switch (node.type())
    {
    case toml::node_type::table:
        return fromTable(*node.as_table());
    case toml::node_type::array:
        return fromArray(*node.as_array());
    case toml::node_type::string:
        return TomlValue::string(node.as_string()->get());
    case toml::node_type::integer:
        return TomlValue::integer(static_cast<std::int64_t>(node.as_integer()->get()));
    case toml::node_type::floating_point:
        return TomlValue::floating(node.as_floating_point()->get());
    case toml::node_type::boolean:
        return TomlValue::boolean(node.as_boolean()->get());
    case toml::node_type::date:
        return temporalText(node.as_date()->get());
    case toml::node_type::time:
        return temporalText(node.as_time()->get());
    case toml::node_type::date_time:
        return temporalText(node.as_date_time()->get());
    case toml::node_type::none:
        break;
    }
    throw std::logic_error("TOML parse: a node of no type");
}

TomlValue fromTable(const toml::table &source)
{
    TomlValue result = TomlValue::table();
    for (auto &&[key, value] : source)
    {
        result.set(std::string(key.str()), fromNode(value));
    }
    return result;
}

TomlValue fromArray(const toml::array &source)
{
    TomlValue result = TomlValue::array();
    for (auto &&element : source)
    {
        result.append(fromNode(element));
    }
    return result;
}

toml::table toTable(const TomlValue &source);
toml::array toArray(const TomlValue &source);

void put(toml::table &parent, std::string_view key, const TomlValue &value)
{
    switch (value.kind())
    {
    case TomlKind::Table:
        parent.insert(key, toTable(value));
        break;
    case TomlKind::Array:
        parent.insert(key, toArray(value));
        break;
    case TomlKind::String:
        parent.insert(key, value.asString());
        break;
    case TomlKind::Integer:
        parent.insert(key, value.asInteger());
        break;
    case TomlKind::Floating:
        parent.insert(key, value.asFloating());
        break;
    case TomlKind::Boolean:
        parent.insert(key, value.asBoolean());
        break;
    }
}

void push(toml::array &parent, const TomlValue &value)
{
    switch (value.kind())
    {
    case TomlKind::Table:
        parent.push_back(toTable(value));
        break;
    case TomlKind::Array:
        parent.push_back(toArray(value));
        break;
    case TomlKind::String:
        parent.push_back(value.asString());
        break;
    case TomlKind::Integer:
        parent.push_back(value.asInteger());
        break;
    case TomlKind::Floating:
        parent.push_back(value.asFloating());
        break;
    case TomlKind::Boolean:
        parent.push_back(value.asBoolean());
        break;
    }
}

toml::table toTable(const TomlValue &source)
{
    toml::table result;
    for (const std::string &key : source.keys())
    {
        put(result, key, source.at(key));
    }
    return result;
}

toml::array toArray(const TomlValue &source)
{
    toml::array result;
    for (const TomlValue &element : source.elements())
    {
        push(result, element);
    }
    return result;
}

class TomlCodec final : public ITomlCodec
{
  public:
    TomlValue parse(std::string_view text) const override
    {
        try
        {
            return fromTable(toml::parse(text));
        } catch (const toml::parse_error &error)
        {
            const auto begin = error.source().begin;
            std::ostringstream message;
            message << "TOML parse error: " << error.description() << " (line " << begin.line
                    << ", column " << begin.column << ")";
            throw TomlParseError(message.str(), begin.line, begin.column);
        }
    }

    std::string serialize(const TomlValue &root) const override
    {
        if (root.kind() != TomlKind::Table)
        {
            throw std::invalid_argument("TOML serialize: the root value must be a table, it is a "
                                        + std::string(kindName(root.kind())));
        }
        std::ostringstream stream;
        stream << toTable(root);
        return stream.str();
    }
};
} // namespace

TomlValue::TomlValue(TomlKind kind) : _kind(kind) {}

TomlValue TomlValue::table()
{
    return TomlValue(TomlKind::Table);
}

TomlValue TomlValue::array()
{
    return TomlValue(TomlKind::Array);
}

TomlValue TomlValue::string(std::string text)
{
    TomlValue value(TomlKind::String);
    value._text = std::move(text);
    return value;
}

TomlValue TomlValue::integer(std::int64_t number)
{
    TomlValue value(TomlKind::Integer);
    value._integer = number;
    return value;
}

TomlValue TomlValue::floating(double number)
{
    TomlValue value(TomlKind::Floating);
    value._floating = number;
    return value;
}

TomlValue TomlValue::boolean(bool flag)
{
    TomlValue value(TomlKind::Boolean);
    value._boolean = flag;
    return value;
}

TomlKind TomlValue::kind() const noexcept
{
    return _kind;
}

void TomlValue::require(TomlKind wanted) const
{
    if (_kind != wanted)
    {
        throw std::logic_error(std::string("TomlValue: a ") + kindName(_kind) + " was asked for as a "
                               + kindName(wanted));
    }
}

const std::string &TomlValue::asString() const
{
    require(TomlKind::String);
    return _text;
}

std::int64_t TomlValue::asInteger() const
{
    require(TomlKind::Integer);
    return _integer;
}

double TomlValue::asFloating() const
{
    require(TomlKind::Floating);
    return _floating;
}

bool TomlValue::asBoolean() const
{
    require(TomlKind::Boolean);
    return _boolean;
}

const std::vector<std::string> &TomlValue::keys() const
{
    require(TomlKind::Table);
    return _keys;
}

const TomlValue *TomlValue::find(std::string_view key) const
{
    require(TomlKind::Table);
    const auto found = std::find(_keys.begin(), _keys.end(), key);
    return found == _keys.end() ? nullptr : &_values[static_cast<std::size_t>(found - _keys.begin())];
}

const TomlValue &TomlValue::at(std::string_view key) const
{
    const TomlValue *found = find(key);
    if (found == nullptr)
    {
        throw std::out_of_range("TomlValue: the table has no key " + std::string(key));
    }
    return *found;
}

void TomlValue::set(std::string key, TomlValue value)
{
    require(TomlKind::Table);
    const auto found = std::find(_keys.begin(), _keys.end(), key);
    if (found != _keys.end())
    {
        _values[static_cast<std::size_t>(found - _keys.begin())] = std::move(value);
        return;
    }
    _keys.push_back(std::move(key));
    _values.push_back(std::move(value));
}

const std::vector<TomlValue> &TomlValue::elements() const
{
    require(TomlKind::Array);
    return _values;
}

void TomlValue::append(TomlValue value)
{
    require(TomlKind::Array);
    _values.push_back(std::move(value));
}

bool TomlValue::operator==(const TomlValue &other) const
{
    if (_kind != other._kind)
    {
        return false;
    }
    switch (_kind)
    {
    case TomlKind::Table:
        if (_keys.size() != other._keys.size())
        {
            return false;
        }
        for (std::size_t index = 0; index < _keys.size(); ++index)
        {
            const TomlValue *counterpart = other.find(_keys[index]);
            if (counterpart == nullptr || !(_values[index] == *counterpart))
            {
                return false;
            }
        }
        return true;
    case TomlKind::Array:
        return _values == other._values;
    case TomlKind::String:
        return _text == other._text;
    case TomlKind::Integer:
        return _integer == other._integer;
    case TomlKind::Floating:
        return _floating == other._floating;
    case TomlKind::Boolean:
        return _boolean == other._boolean;
    }
    throw std::logic_error("TomlValue: unknown kind");
}

TomlParseError::TomlParseError(const std::string &message, std::uint32_t line, std::uint32_t column)
    : std::runtime_error(message), _line(line), _column(column)
{
}

std::uint32_t TomlParseError::line() const noexcept
{
    return _line;
}

std::uint32_t TomlParseError::column() const noexcept
{
    return _column;
}

std::unique_ptr<ITomlCodec> createTomlCodec()
{
    return std::make_unique<TomlCodec>();
}
} // namespace vigine::format
