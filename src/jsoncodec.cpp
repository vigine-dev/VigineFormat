#include "vigine/format/jsoncodec.h"

#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include <utility>

namespace vigine::format
{
namespace
{
using Json = nlohmann::ordered_json;

DataValue fromJson(const Json &source)
{
    switch (source.type())
    {
    case Json::value_t::object:
    {
        DataValue result = DataValue::table();
        for (auto entry = source.begin(); entry != source.end(); ++entry)
        {
            result.set(entry.key(), fromJson(entry.value()));
        }
        return result;
    }
    case Json::value_t::array:
    {
        DataValue result = DataValue::array();
        for (const Json &element : source)
        {
            result.append(fromJson(element));
        }
        return result;
    }
    case Json::value_t::string:
        return DataValue::string(source.get<std::string>());
    case Json::value_t::boolean:
        return DataValue::boolean(source.get<bool>());
    case Json::value_t::number_integer:
        return DataValue::integer(source.get<std::int64_t>());
    case Json::value_t::number_unsigned:
    {
        const std::uint64_t number = source.get<std::uint64_t>();
        if (number > static_cast<std::uint64_t>(INT64_MAX))
        {
            return DataValue::floating(static_cast<double>(number));
        }
        return DataValue::integer(static_cast<std::int64_t>(number));
    }
    case Json::value_t::number_float:
        return DataValue::floating(source.get<double>());
    case Json::value_t::null:
        return DataValue::null();
    case Json::value_t::binary:
    case Json::value_t::discarded:
        break;
    }
    throw std::logic_error("JSON parse: a node of no usable type");
}

Json toJson(const DataValue &source)
{
    switch (source.kind())
    {
    case DataKind::Table:
    {
        Json result = Json::object();
        for (const std::string &key : source.keys())
        {
            result[key] = toJson(source.at(key));
        }
        return result;
    }
    case DataKind::Array:
    {
        Json result = Json::array();
        for (const DataValue &element : source.elements())
        {
            result.push_back(toJson(element));
        }
        return result;
    }
    case DataKind::String:
        return source.asString();
    case DataKind::Integer:
        return source.asInteger();
    case DataKind::Float:
        if (!std::isfinite(source.asFloating()))
        {
            throw std::invalid_argument("JSON serialize: a float that is not finite cannot be written");
        }
        return source.asFloating();
    case DataKind::Boolean:
        return source.asBoolean();
    case DataKind::Null:
        return nullptr;
    }
    throw std::logic_error("JSON serialize: a value of no usable kind");
}

class JsonCodec final : public IJsonCodec
{
  public:
    DataValue parse(std::string_view text) const override
    {
        try
        {
            return fromJson(Json::parse(text.begin(), text.end()));
        } catch (const Json::parse_error &error)
        {
            const std::size_t offset = std::min<std::size_t>(error.byte > 0 ? error.byte - 1 : 0, text.size());
            const std::string_view before = text.substr(0, offset);
            const auto line = static_cast<std::uint32_t>(std::count(before.begin(), before.end(), '\n') + 1);
            const std::size_t lastBreak = before.rfind('\n');
            const auto column = static_cast<std::uint32_t>(lastBreak == std::string_view::npos
                                                               ? offset + 1
                                                               : offset - lastBreak);
            throw JsonParseError(std::string("JSON parse error: ") + error.what() + " (line " + std::to_string(line)
                                     + ", column " + std::to_string(column) + ")",
                                 line, column);
        }
    }

    std::string serialize(const DataValue &root) const override
    {
        return toJson(root).dump();
    }
};
} // namespace

JsonParseError::JsonParseError(const std::string &message, std::uint32_t line, std::uint32_t column)
    : std::runtime_error(message), _line(line), _column(column)
{
}

std::uint32_t JsonParseError::line() const noexcept
{
    return _line;
}

std::uint32_t JsonParseError::column() const noexcept
{
    return _column;
}

IJsonCodec::~IJsonCodec() = default;

std::unique_ptr<IJsonCodec> createJsonCodec()
{
    return std::make_unique<JsonCodec>();
}
} // namespace vigine::format
