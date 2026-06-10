#include "vigine/format/identify.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace vigine::format
{
namespace
{
std::string asciiLower(std::string_view text)
{
    std::string out(text);
    for (char &ch : out)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return out;
}

bool contains(std::string_view haystack, std::string_view needle)
{
    return haystack.find(needle) != std::string_view::npos;
}
} // namespace

std::string_view formatName(KnownFormat format) noexcept
{
    switch (format)
    {
    case KnownFormat::Dot:        return "dot";
    case KnownFormat::Mermaid:    return "mermaid";
    case KnownFormat::GraphMl:    return "graphml";
    case KnownFormat::PlantUml:   return "plantuml";
    case KnownFormat::Json:       return "json";
    case KnownFormat::EdgeList:   return "edgelist";
    case KnownFormat::GraphQl:    return "graphql";
    case KnownFormat::Protobuf:   return "protobuf";
    case KnownFormat::Sql:        return "sql";
    case KnownFormat::FlameGraph: return "flamegraph";
    case KnownFormat::Yaml:       return "yaml";
    case KnownFormat::Toml:       return "toml";
    case KnownFormat::Xml:        return "xml";
    case KnownFormat::Pdf:        return "pdf";
    case KnownFormat::OpenApi:    return "openapi";
    case KnownFormat::Obj:        return "obj";
    case KnownFormat::Stl:        return "stl";
    case KnownFormat::Ply:        return "ply";
    case KnownFormat::Xyz:        return "xyz";
    case KnownFormat::Vox:        return "vox";
    case KnownFormat::Unknown:
    default:                      return "unknown";
    }
}

KnownFormat identifyContent(std::string_view content) noexcept
{
    if (content.empty())
        return KnownFormat::Unknown;

    // Binary magic first -- it cannot be confused with any text probe.
    if (content.rfind("%PDF-", 0) == 0)
        return KnownFormat::Pdf;
    if (content.rfind("VOX ", 0) == 0)
        return KnownFormat::Vox;
    if (content.rfind("ply", 0) == 0 && content.find("end_header") != std::string_view::npos)
        return KnownFormat::Ply;
    if (content.rfind("solid", 0) == 0 && contains(content, "facet"))
        return KnownFormat::Stl;

    const std::size_t firstGlyph = content.find_first_not_of(" \t\r\n");
    if (firstGlyph == std::string_view::npos)
        return KnownFormat::Unknown;
    const char lead = content[firstGlyph];

    // A schema-on-top wins over its carrier: an OpenAPI spec IS json or yaml,
    // but the more specific verdict is the useful one.
    if (contains(content, "\"openapi\"") || contains(content, "openapi:") ||
        contains(content, "\"swagger\"") || contains(content, "swagger:"))
        return KnownFormat::OpenApi;

    if (lead == '{' || lead == '[')
        return KnownFormat::Json;
    if (contains(content, "@startuml") || contains(content, "<|--"))
        return KnownFormat::PlantUml;
    if (contains(content, "<graphml"))
        return KnownFormat::GraphMl;
    if (lead == '<')
        return KnownFormat::Xml;
    if (content.compare(firstGlyph, 3, "---") == 0)
        return KnownFormat::Yaml;

    const std::string lower = asciiLower(content);
    if (contains(lower, "create table"))
        return KnownFormat::Sql;
    if (contains(content, "syntax =") ||
        (contains(content, "message ") && contains(content, "{")))
        return KnownFormat::Protobuf;
    if (contains(content, "scalar ") || contains(content, "type Query") ||
        contains(content, "schema {"))
        return KnownFormat::GraphQl;
    // Mermaid before DOT: a Mermaid "A-->B" arrow contains the DOT "->", and
    // both languages open with the word "graph" -- Mermaid follows it with a
    // direction token, DOT with a brace or a name.
    const auto mermaidHeader = [&]() {
        if (content.compare(firstGlyph, 9, "flowchart") == 0)
            return true;
        if (content.compare(firstGlyph, 6, "graph ") != 0)
            return false;
        const std::string_view direction = content.substr(firstGlyph + 6, 2);
        return direction == "TD" || direction == "TB" || direction == "LR" ||
               direction == "RL" || direction == "BT";
    };
    if (mermaidHeader() || contains(content, "-->") || contains(content, "-.->") ||
        contains(content, "==>"))
        return KnownFormat::Mermaid;
    if (contains(content, "digraph") || contains(content, "->") ||
        content.compare(firstGlyph, 6, "graph ") == 0)
        return KnownFormat::Dot;

    return KnownFormat::Unknown;
}

KnownFormat identifyExtension(std::string_view fileName) noexcept
{
    const std::size_t dot = fileName.find_last_of('.');
    if (dot == std::string_view::npos || dot + 1 >= fileName.size())
        return KnownFormat::Unknown;
    const std::string ext = asciiLower(fileName.substr(dot + 1));

    struct Mapping
    {
        std::string_view extension;
        KnownFormat      format;
    };
    static constexpr std::array<Mapping, 28> kMappings = {{
        {"dot", KnownFormat::Dot},          {"gv", KnownFormat::Dot},
        {"mmd", KnownFormat::Mermaid},      {"mermaid", KnownFormat::Mermaid},
        {"graphml", KnownFormat::GraphMl},  {"puml", KnownFormat::PlantUml},
        {"plantuml", KnownFormat::PlantUml},{"wsd", KnownFormat::PlantUml},
        {"json", KnownFormat::Json},        {"ipynb", KnownFormat::Json},
        {"gql", KnownFormat::GraphQl},      {"graphql", KnownFormat::GraphQl},
        {"proto", KnownFormat::Protobuf},   {"sql", KnownFormat::Sql},
        {"ddl", KnownFormat::Sql},          {"folded", KnownFormat::FlameGraph},
        {"collapsed", KnownFormat::FlameGraph}, {"yaml", KnownFormat::Yaml},
        {"yml", KnownFormat::Yaml},         {"toml", KnownFormat::Toml},
        {"xml", KnownFormat::Xml},          {"pdf", KnownFormat::Pdf},
        {"obj", KnownFormat::Obj},          {"stl", KnownFormat::Stl},
        {"ply", KnownFormat::Ply},          {"xyz", KnownFormat::Xyz},
        {"pts", KnownFormat::Xyz},          {"vox", KnownFormat::Vox},
    }};
    for (const auto &mapping : kMappings)
        if (ext == mapping.extension)
            return mapping.format;
    return KnownFormat::Unknown;
}

KnownFormat identify(std::string_view fileName, std::string_view content) noexcept
{
    const KnownFormat byContent = identifyContent(content);
    if (byContent != KnownFormat::Unknown)
        return byContent;
    return identifyExtension(fileName);
}

} // namespace vigine::format
