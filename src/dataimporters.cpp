#include "vigine/format/diagramimporters.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <functional>
#include <stdexcept>
#include <string>

// The amalgamated single-header YAML backend defines its implementation in
// exactly this translation unit; everything stays an implementation detail.
#define RYML_SINGLE_HDR_DEFINE_NOW
#include <ryml/rapidyaml.hpp>

#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

#include <nlohmann/json.hpp>

namespace vigine::format
{
namespace
{
std::string truncateLabel(std::string text)
{
    constexpr std::size_t kMaxLabel = 40;
    if (text.size() > kMaxLabel)
    {
        text.resize(kMaxLabel - 2);
        text += "..";
    }
    return text;
}

// rapidyaml aborts through its callbacks on malformed input by default; route
// every error flavour into an exception the importer can catch instead.
[[noreturn]] void rymlErrorBasic(ryml::csubstr msg, ryml::ErrorDataBasic const &, void *)
{
    throw std::runtime_error(std::string(msg.str, msg.len));
}
[[noreturn]] void rymlErrorParse(ryml::csubstr msg, ryml::ErrorDataParse const &, void *)
{
    throw std::runtime_error(std::string(msg.str, msg.len));
}
[[noreturn]] void rymlErrorVisit(ryml::csubstr msg, ryml::ErrorDataVisit const &, void *)
{
    throw std::runtime_error(std::string(msg.str, msg.len));
}

struct RymlThrowingCallbacks
{
    RymlThrowingCallbacks()
    {
        ryml::Callbacks callbacks = ryml::get_callbacks();
        callbacks.m_error_basic   = rymlErrorBasic;
        callbacks.m_error_parse   = rymlErrorParse;
        callbacks.m_error_visit   = rymlErrorVisit;
        ryml::set_callbacks(callbacks);
    }
};

// Parses YAML and re-emits it as JSON so the JSON tree walker serves both
// data languages. Empty on malformed input.
std::string yamlToJson(std::string_view text)
{
    static const RymlThrowingCallbacks kInstallOnce;
    try
    {
        const ryml::Tree tree =
            ryml::parse_in_arena(ryml::csubstr(text.data(), text.size()));
        return ryml::emitrs_json<std::string>(tree);
    }
    catch (const std::exception &)
    {
        return {};
    }
}

std::string tomlScalarText(const toml::node &node)
{
    if (const auto *str = node.as_string())
        return str->get();
    if (const auto *integer = node.as_integer())
        return std::to_string(integer->get());
    if (const auto *real = node.as_floating_point())
        return std::to_string(real->get());
    if (const auto *boolean = node.as_boolean())
        return boolean->get() ? "true" : "false";
    if (node.is_date() || node.is_time() || node.is_date_time())
        return "datetime";
    return {};
}
} // namespace

std::optional<DiagramModel> YamlTreeImporter::import(std::string_view text) const
{
    const std::string json = yamlToJson(text);
    if (json.empty())
        return std::nullopt;
    return JsonTreeImporter{}.import(json);
}

std::optional<DiagramModel> TomlTreeImporter::import(std::string_view text) const
{
    const toml::parse_result parsed = toml::parse(text);
    if (!parsed)
        return std::nullopt;

    constexpr int         kMaxDepth = 12;
    constexpr std::size_t kMaxNodes = 256;
    DiagramModel          model;
    std::size_t           counter = 1; // root takes id n0

    const std::function<void(const toml::node &, const std::string &, const std::string &, int)>
        walk = [&](const toml::node &node, const std::string &nodeId,
                   const std::string &nodeLabel, int depth) {
            model.addNode(nodeId, nodeLabel);
            if (depth >= kMaxDepth || counter >= kMaxNodes)
                return;
            if (const auto *table = node.as_table())
            {
                for (const auto &[key, value] : *table)
                {
                    if (counter >= kMaxNodes)
                        break;
                    const std::string childId    = "n" + std::to_string(counter++);
                    std::string       childLabel = std::string(key.str());
                    if (!value.is_table() && !value.is_array())
                        childLabel += ": " + tomlScalarText(value);
                    walk(value, childId, truncateLabel(childLabel), depth + 1);
                    model.addEdge(nodeId, childId);
                }
            }
            else if (const auto *array = node.as_array())
            {
                std::size_t index = 0;
                for (const auto &element : *array)
                {
                    if (counter >= kMaxNodes)
                        break;
                    const std::string childId    = "n" + std::to_string(counter++);
                    std::string       childLabel = "[" + std::to_string(index) + "]";
                    if (!element.is_table() && !element.is_array())
                        childLabel += " " + tomlScalarText(element);
                    walk(element, childId, truncateLabel(childLabel), depth + 1);
                    model.addEdge(nodeId, childId);
                    ++index;
                }
            }
        };
    walk(parsed.table(), "n0", "table", 0);

    if (model.nodes.size() < 2)
        return std::nullopt; // an empty document carries no structure to draw
    return model;
}

std::optional<DiagramModel> OpenApiImporter::import(std::string_view text) const
{
    // Accept both flavours: JSON as-is, YAML normalised to JSON first.
    const std::size_t firstGlyph = text.find_first_not_of(" \t\r\n");
    if (firstGlyph == std::string_view::npos)
        return std::nullopt;
    std::string jsonText;
    if (text[firstGlyph] == '{' || text[firstGlyph] == '[')
        jsonText.assign(text);
    else
        jsonText = yamlToJson(text);
    if (jsonText.empty())
        return std::nullopt;

    const nlohmann::json spec =
        nlohmann::json::parse(jsonText, nullptr, /*allow_exceptions=*/false);
    if (spec.is_discarded() || !spec.is_object() || !spec.contains("paths") ||
        !spec["paths"].is_object())
        return std::nullopt;

    DiagramModel model;
    std::string  title = "api";
    if (spec.contains("info") && spec["info"].is_object() && spec["info"].contains("title") &&
        spec["info"]["title"].is_string())
        title = spec["info"]["title"].get<std::string>();
    model.addNode("api", truncateLabel(title));

    static constexpr std::array<std::string_view, 8> kMethods = {
        "get", "post", "put", "delete", "patch", "head", "options", "trace"};
    for (const auto &[path, operations] : spec["paths"].items())
    {
        model.addNode(path, truncateLabel(path));
        model.addEdge("api", path);
        if (!operations.is_object())
            continue;
        for (const auto &[method, operation] : operations.items())
        {
            if (std::find(kMethods.begin(), kMethods.end(), method) == kMethods.end())
                continue;
            std::string label = method;
            std::transform(label.begin(), label.end(), label.begin(),
                           [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
            if (operation.is_object())
            {
                if (operation.contains("summary") && operation["summary"].is_string())
                    label += " " + operation["summary"].get<std::string>();
                else if (operation.contains("operationId") &&
                         operation["operationId"].is_string())
                    label += " " + operation["operationId"].get<std::string>();
            }
            const std::string operationId = path + "#" + method;
            model.addNode(operationId, truncateLabel(label));
            model.addEdge(path, operationId);
        }
    }

    if (model.nodes.size() < 2)
        return std::nullopt;
    return model;
}

} // namespace vigine::format
