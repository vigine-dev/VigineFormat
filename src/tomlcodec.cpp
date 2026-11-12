#include "vigine/format/tomlcodec.h"

#include <algorithm>
#include <sstream>
#include <toml++/toml.hpp>
#include <utility>

namespace vigine::format
{
namespace
{
template <typename Temporal> DataValue temporalText(const Temporal &temporal)
{
    std::ostringstream stream;
    stream << temporal;
    return DataValue::string(stream.str());
}

DataValue fromTable(const toml::table &source);
DataValue fromArray(const toml::array &source);

DataValue fromNode(const toml::node &node)
{
    switch (node.type())
    {
    case toml::node_type::table:
        return fromTable(*node.as_table());
    case toml::node_type::array:
        return fromArray(*node.as_array());
    case toml::node_type::string:
        return DataValue::string(node.as_string()->get());
    case toml::node_type::integer:
        return DataValue::integer(static_cast<std::int64_t>(node.as_integer()->get()));
    case toml::node_type::floating_point:
        return DataValue::floating(node.as_floating_point()->get());
    case toml::node_type::boolean:
        return DataValue::boolean(node.as_boolean()->get());
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

DataValue fromTable(const toml::table &source)
{
    DataValue result = DataValue::table();
    for (auto &&[key, value] : source)
    {
        result.set(std::string(key.str()), fromNode(value));
    }
    return result;
}

DataValue fromArray(const toml::array &source)
{
    DataValue result = DataValue::array();
    for (auto &&element : source)
    {
        result.append(fromNode(element));
    }
    return result;
}

toml::table toTable(const DataValue &source);
toml::array toArray(const DataValue &source);

void put(toml::table &parent, std::string_view key, const DataValue &value)
{
    switch (value.kind())
    {
    case DataKind::Table:
        parent.insert(key, toTable(value));
        break;
    case DataKind::Array:
        parent.insert(key, toArray(value));
        break;
    case DataKind::String:
        parent.insert(key, value.asString());
        break;
    case DataKind::Integer:
        parent.insert(key, value.asInteger());
        break;
    case DataKind::Float:
        parent.insert(key, value.asFloating());
        break;
    case DataKind::Boolean:
        parent.insert(key, value.asBoolean());
        break;
    case DataKind::Null:
        throw std::invalid_argument("TOML serialize: the key \"" + std::string(key)
                                    + "\" holds a null, which TOML cannot write");
    }
}

void push(toml::array &parent, const DataValue &value)
{
    switch (value.kind())
    {
    case DataKind::Table:
        parent.push_back(toTable(value));
        break;
    case DataKind::Array:
        parent.push_back(toArray(value));
        break;
    case DataKind::String:
        parent.push_back(value.asString());
        break;
    case DataKind::Integer:
        parent.push_back(value.asInteger());
        break;
    case DataKind::Float:
        parent.push_back(value.asFloating());
        break;
    case DataKind::Boolean:
        parent.push_back(value.asBoolean());
        break;
    case DataKind::Null:
        throw std::invalid_argument("TOML serialize: an array element is a null, which TOML cannot write");
    }
}

toml::table toTable(const DataValue &source)
{
    toml::table result;
    for (const std::string &key : source.keys())
    {
        put(result, key, source.at(key));
    }
    return result;
}

toml::array toArray(const DataValue &source)
{
    toml::array result;
    for (const DataValue &element : source.elements())
    {
        push(result, element);
    }
    return result;
}

class TomlCodec final : public ITomlCodec
{
  public:
    DataValue parse(std::string_view text) const override
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

    std::string serialize(const DataValue &root) const override
    {
        if (root.kind() != DataKind::Table)
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
