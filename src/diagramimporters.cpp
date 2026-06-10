#include "vigine/format/diagramimporters.h"

#include <array>
#include <cctype>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace vigine::format
{
namespace
{
bool isSpace(char ch)
{
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

std::string_view trim(std::string_view text)
{
    std::size_t begin = 0;
    std::size_t end   = text.size();
    while (begin < end && isSpace(text[begin]))
    {
        ++begin;
    }
    while (end > begin && isSpace(text[end - 1]))
    {
        --end;
    }
    return text.substr(begin, end - begin);
}

// Splits on any character in `delims`, dropping empty pieces.
std::vector<std::string_view> splitAny(std::string_view text, std::string_view delims)
{
    std::vector<std::string_view> pieces;
    std::size_t start = 0;
    for (std::size_t index = 0; index <= text.size(); ++index)
    {
        if (index == text.size() || delims.find(text[index]) != std::string_view::npos)
        {
            if (index > start)
            {
                pieces.push_back(trim(text.substr(start, index - start)));
            }
            start = index + 1;
        }
    }
    return pieces;
}

std::string unquote(std::string_view text)
{
    text = trim(text);
    if (text.size() >= 2 && (text.front() == '"' || text.front() == '\'') &&
        text.back() == text.front())
    {
        text = text.substr(1, text.size() - 2);
    }
    return std::string(text);
}

char closingFor(char open)
{
    switch (open)
    {
    case '[': return ']';
    case '(': return ')';
    case '{': return '}';
    default:  return '\0';
    }
}

// Reads an endpoint from one side of an edge (or a standalone node def):
// skips leading punctuation/noise, takes the identifier run, and captures a
// trailing shape label A[..]/A(..)/A{..} when present.
struct Endpoint
{
    std::string id;
    std::string label;
};

Endpoint parseEndpoint(std::string_view side)
{
    side = trim(side);
    std::size_t cursor = 0;
    while (cursor < side.size() && side[cursor] != '"' && side[cursor] != '\'' &&
           std::isalnum(static_cast<unsigned char>(side[cursor])) == 0)
    {
        ++cursor;
    }
    Endpoint endpoint;
    if (cursor < side.size() && (side[cursor] == '"' || side[cursor] == '\''))
    {
        const char quote      = side[cursor];
        const std::size_t end = side.find(quote, cursor + 1);
        if (end != std::string_view::npos)
        {
            endpoint.id = std::string(side.substr(cursor + 1, end - cursor - 1));
            cursor      = end + 1;
        }
    }
    else
    {
        const std::size_t idStart = cursor;
        while (cursor < side.size() &&
               (std::isalnum(static_cast<unsigned char>(side[cursor])) != 0 || side[cursor] == '_'))
        {
            ++cursor;
        }
        endpoint.id = std::string(side.substr(idStart, cursor - idStart));
    }
    while (cursor < side.size() && isSpace(side[cursor]))
    {
        ++cursor;
    }

    if (cursor < side.size() && closingFor(side[cursor]) != '\0')
    {
        const char open  = side[cursor];
        const char close = closingFor(open);
        // Walk to the matching (possibly doubled, e.g. ((..)) ) closer.
        std::size_t labelStart = cursor + 1;
        while (labelStart < side.size() && side[labelStart] == open)
        {
            ++labelStart;
        }
        std::size_t labelEnd = side.rfind(close);
        if (labelEnd != std::string_view::npos && labelEnd >= labelStart)
        {
            while (labelEnd > labelStart && side[labelEnd - 1] == close)
            {
                --labelEnd;
            }
            endpoint.label = unquote(side.substr(labelStart, labelEnd - labelStart));
        }
    }
    return endpoint;
}

const std::array<std::string_view, 8> kMermaidArrows = {"-.->", "==>", "-->", "--x", "--o",
                                                        "---", "===", "-.-"};

// Finds the first Mermaid arrow in `line`; returns its offset, length, and
// whether it is directed. offset == npos when none.
struct ArrowMatch
{
    std::size_t offset{std::string_view::npos};
    std::size_t length{0};
    bool directed{true};
};

ArrowMatch findArrow(std::string_view line)
{
    ArrowMatch best;
    for (const std::string_view arrow : kMermaidArrows)
    {
        const std::size_t found = line.find(arrow);
        if (found != std::string_view::npos && found < best.offset)
        {
            best.offset   = found;
            best.length   = arrow.size();
            best.directed = arrow.find('>') != std::string_view::npos ||
                            arrow.find('x') != std::string_view::npos ||
                            arrow.find('o') != std::string_view::npos;
        }
    }
    return best;
}
} // namespace

std::optional<DiagramModel> DotImporter::import(std::string_view text) const
{
    DiagramModel model;
    std::string  body(text);
    const std::size_t open  = body.find('{');
    const std::size_t close = body.rfind('}');
    if (open != std::string::npos && close != std::string::npos && close > open)
    {
        body = body.substr(open + 1, close - open - 1);
    }

    for (const std::string_view raw : splitAny(body, ";\n"))
    {
        const std::string_view stmt = trim(raw);
        if (stmt.empty() || stmt == "{" || stmt == "}")
        {
            continue;
        }
        const std::size_t arrow = stmt.find("->");
        const std::size_t undir = stmt.find("--");
        if (arrow != std::string_view::npos || undir != std::string_view::npos)
        {
            const bool directed = arrow != std::string_view::npos;
            const std::string_view op = directed ? "->" : "--";
            // Chain a -> b -> c.
            std::vector<std::string> ids;
            std::size_t start = 0;
            while (start <= stmt.size())
            {
                const std::size_t next = stmt.find(op, start);
                const std::string_view token =
                    stmt.substr(start, (next == std::string_view::npos ? stmt.size() : next) - start);
                ids.push_back(parseEndpoint(token).id);
                if (next == std::string_view::npos)
                {
                    break;
                }
                start = next + op.size();
            }
            for (std::size_t index = 1; index < ids.size(); ++index)
            {
                model.addEdge(ids[index - 1], ids[index], {}, directed);
            }
            continue;
        }
        const std::size_t bracket = stmt.find('[');
        if (bracket != std::string_view::npos)
        {
            const std::string id = unquote(trim(stmt.substr(0, bracket)));
            const std::size_t labelPos = stmt.find("label", bracket);
            std::string label;
            if (labelPos != std::string_view::npos)
            {
                const std::size_t eq = stmt.find('=', labelPos);
                if (eq != std::string_view::npos)
                {
                    std::size_t valueEnd = stmt.find_first_of(",]", eq + 1);
                    if (valueEnd == std::string_view::npos)
                    {
                        valueEnd = stmt.size();
                    }
                    label = unquote(stmt.substr(eq + 1, valueEnd - eq - 1));
                }
            }
            model.addNode(id, label);
            continue;
        }
        if (stmt.find('=') == std::string_view::npos)
        {
            model.addNode(unquote(stmt));
        }
    }

    if (model.nodes.empty())
    {
        return std::nullopt;
    }
    return model;
}

std::optional<DiagramModel> MermaidImporter::import(std::string_view text) const
{
    DiagramModel model;
    for (const std::string_view raw : splitAny(text, ";\n"))
    {
        std::string_view line = trim(raw);
        if (line.empty() || line.rfind("%%", 0) == 0 || line.rfind("graph", 0) == 0 ||
            line.rfind("flowchart", 0) == 0 || line.rfind("subgraph", 0) == 0 || line == "end" ||
            line.rfind("classDef", 0) == 0 || line.rfind("class ", 0) == 0 ||
            line.rfind("style", 0) == 0 || line.rfind("linkStyle", 0) == 0)
        {
            continue;
        }

        const ArrowMatch arrow = findArrow(line);
        if (arrow.offset == std::string_view::npos)
        {
            const Endpoint single = parseEndpoint(line);
            model.addNode(single.id, single.label);
            continue;
        }

        const std::string_view leftSide = line.substr(0, arrow.offset);
        std::string_view rightSide      = line.substr(arrow.offset + arrow.length);
        std::string edgeLabel;
        rightSide = trim(rightSide);
        if (!rightSide.empty() && rightSide.front() == '|')
        {
            const std::size_t closePipe = rightSide.find('|', 1);
            if (closePipe != std::string_view::npos)
            {
                edgeLabel = unquote(rightSide.substr(1, closePipe - 1));
                rightSide = rightSide.substr(closePipe + 1);
            }
        }
        const Endpoint left  = parseEndpoint(leftSide);
        const Endpoint right = parseEndpoint(rightSide);
        if (!left.label.empty())
        {
            model.addNode(left.id, left.label);
        }
        if (!right.label.empty())
        {
            model.addNode(right.id, right.label);
        }
        model.addEdge(left.id, right.id, edgeLabel, arrow.directed);
    }

    if (model.nodes.empty())
    {
        return std::nullopt;
    }
    return model;
}

namespace
{
// Reads the value of attribute `name` from an opening-tag slice like
// `node id="a" foo="bar"`. Handles single and double quotes. Empty when absent.
std::string attributeValue(std::string_view tag, std::string_view name)
{
    std::size_t cursor = 0;
    while ((cursor = tag.find(name, cursor)) != std::string_view::npos)
    {
        const std::size_t after = cursor + name.size();
        std::size_t scan        = after;
        while (scan < tag.size() && isSpace(tag[scan]))
        {
            ++scan;
        }
        if (scan < tag.size() && tag[scan] == '=')
        {
            ++scan;
            while (scan < tag.size() && isSpace(tag[scan]))
            {
                ++scan;
            }
            if (scan < tag.size() && (tag[scan] == '"' || tag[scan] == '\''))
            {
                const char quote      = tag[scan];
                const std::size_t end = tag.find(quote, scan + 1);
                if (end != std::string_view::npos)
                {
                    return std::string(tag.substr(scan + 1, end - scan - 1));
                }
            }
        }
        cursor = after;
    }
    return {};
}

// Extracts the text content of the first <data ...>TEXT</data> child found in
// `slice` (the span between a node's opening tag and its closer).
std::string firstDataText(std::string_view slice)
{
    const std::size_t open = slice.find("<data");
    if (open == std::string_view::npos)
    {
        return {};
    }
    const std::size_t gt = slice.find('>', open);
    if (gt == std::string_view::npos)
    {
        return {};
    }
    const std::size_t close = slice.find("</data>", gt);
    if (close == std::string_view::npos)
    {
        return {};
    }
    return std::string(trim(slice.substr(gt + 1, close - gt - 1)));
}
} // namespace

std::optional<DiagramModel> GraphMlImporter::import(std::string_view text) const
{
    DiagramModel model;
    std::size_t  cursor = 0;
    while (cursor < text.size())
    {
        const std::size_t lt = text.find('<', cursor);
        if (lt == std::string_view::npos)
        {
            break;
        }
        const std::size_t gt = text.find('>', lt);
        if (gt == std::string_view::npos)
        {
            break;
        }
        std::string_view tag = text.substr(lt + 1, gt - lt - 1);
        const bool selfClose = !tag.empty() && tag.back() == '/';
        if (selfClose)
        {
            tag = tag.substr(0, tag.size() - 1);
        }
        const std::string_view name = trim(tag).substr(0, trim(tag).find_first_of(" \t\r\n/"));

        if (name == "node")
        {
            const std::string id = attributeValue(tag, "id");
            std::string label;
            std::size_t next = gt + 1;
            if (!selfClose)
            {
                const std::size_t nodeClose = text.find("</node>", gt);
                if (nodeClose != std::string_view::npos)
                {
                    label = firstDataText(text.substr(gt + 1, nodeClose - gt - 1));
                    next  = nodeClose + 7;
                }
            }
            model.addNode(id, label);
            cursor = next;
            continue;
        }
        if (name == "edge")
        {
            model.addEdge(attributeValue(tag, "source"), attributeValue(tag, "target"));
        }
        cursor = gt + 1;
    }

    if (model.nodes.empty())
    {
        return std::nullopt;
    }
    return model;
}

namespace
{
// True when `ch` can be part of a PlantUML relationship connector run.
bool isConnectorChar(char ch)
{
    return ch == '-' || ch == '.' || ch == '<' || ch == '>' || ch == '|' || ch == '*' ||
           ch == 'o' || ch == '+' || ch == '#';
}

// A PlantUML endpoint is the last token on the left side / first on the right.
// Strips quotes and multiplicity decorations (a leading/trailing quoted run).
std::string plantUmlEndpoint(std::string_view side, bool takeLast)
{
    const std::vector<std::string_view> tokens = splitAny(side, " \t");
    if (tokens.empty())
    {
        return {};
    }
    // Multiplicity like "1" or "*" sits in quotes next to the arrow; the class
    // name is the outermost non-multiplicity token.
    auto isMultiplicity = [](std::string_view token) {
        const std::string bare = unquote(token);
        return !bare.empty() &&
               bare.find_first_not_of("0123456789*.") == std::string::npos;
    };
    if (takeLast)
    {
        for (auto it = tokens.rbegin(); it != tokens.rend(); ++it)
        {
            if (!isMultiplicity(*it))
            {
                return unquote(*it);
            }
        }
    }
    else
    {
        for (const std::string_view token : tokens)
        {
            if (!isMultiplicity(token))
            {
                return unquote(token);
            }
        }
    }
    return unquote(tokens.front());
}
} // namespace

std::optional<DiagramModel> PlantUmlImporter::import(std::string_view text) const
{
    DiagramModel model;
    int          blockDepth = 0;
    for (const std::string_view raw : splitAny(text, "\n"))
    {
        std::string_view line = trim(raw);
        // Ignore member lines inside a `class A { ... }` body.
        if (blockDepth > 0)
        {
            if (line.find('}') != std::string_view::npos)
            {
                --blockDepth;
            }
            continue;
        }
        if (line.empty() || line.rfind("@", 0) == 0 || line.rfind("'", 0) == 0 ||
            line.rfind("skinparam", 0) == 0 || line.rfind("title", 0) == 0 ||
            line.rfind("note", 0) == 0 || line.rfind("hide", 0) == 0 ||
            line.rfind("package", 0) == 0)
        {
            continue;
        }

        // Type declaration: [abstract] class|interface|enum|entity Name [{].
        std::string_view declScan = line;
        if (declScan.rfind("abstract", 0) == 0)
        {
            declScan = trim(declScan.substr(8));
        }
        static constexpr std::array<std::string_view, 4> kTypeKeywords = {"class", "interface",
                                                                          "enum", "entity"};
        for (const std::string_view keyword : kTypeKeywords)
        {
            if (declScan.rfind(keyword, 0) == 0 && declScan.size() > keyword.size() &&
                isSpace(declScan[keyword.size()]))
            {
                std::string_view rest = trim(declScan.substr(keyword.size()));
                if (rest.find('}') == std::string_view::npos &&
                    rest.find('{') != std::string_view::npos)
                {
                    ++blockDepth;
                }
                const std::size_t brace = rest.find_first_of("{");
                if (brace != std::string_view::npos)
                {
                    rest = trim(rest.substr(0, brace));
                }
                const std::size_t stereotype = rest.find('<');
                if (stereotype != std::string_view::npos)
                {
                    rest = trim(rest.substr(0, stereotype));
                }
                model.addNode(unquote(rest));
                declScan = {};
                break;
            }
        }
        if (declScan.empty())
        {
            continue;
        }

        // Relationship: find a connector run (>=2 chars, contains - or .).
        std::size_t runStart = std::string_view::npos;
        std::size_t runEnd   = std::string_view::npos;
        for (std::size_t index = 0; index < line.size(); ++index)
        {
            if (isConnectorChar(line[index]))
            {
                std::size_t end = index;
                bool hasLine    = false;
                while (end < line.size() && isConnectorChar(line[end]))
                {
                    hasLine = hasLine || line[end] == '-' || line[end] == '.';
                    ++end;
                }
                if (hasLine && end - index >= 2)
                {
                    runStart = index;
                    runEnd   = end;
                    break;
                }
                index = end;
            }
        }
        if (runStart == std::string_view::npos)
        {
            continue;
        }

        const std::string_view connector = line.substr(runStart, runEnd - runStart);
        std::string_view leftSide        = line.substr(0, runStart);
        std::string_view rightSide       = line.substr(runEnd);
        std::string edgeLabel;
        const std::size_t colon = rightSide.find(':');
        if (colon != std::string_view::npos)
        {
            edgeLabel = std::string(trim(rightSide.substr(colon + 1)));
            rightSide = rightSide.substr(0, colon);
        }
        const bool headLeft  = connector.front() == '<' || connector.front() == '|' ||
                              connector.front() == '*' || connector.front() == 'o';
        const std::string left  = plantUmlEndpoint(leftSide, true);
        const std::string right = plantUmlEndpoint(rightSide, false);
        if (headLeft)
        {
            model.addEdge(right, left, edgeLabel);
        }
        else
        {
            model.addEdge(left, right, edgeLabel);
        }
    }

    if (model.nodes.empty())
    {
        return std::nullopt;
    }
    return model;
}

namespace
{
std::string jsonScalarText(const nlohmann::json &value)
{
    if (value.is_string())
    {
        return value.get<std::string>();
    }
    if (value.is_boolean())
    {
        return value.get<bool>() ? "true" : "false";
    }
    if (value.is_number_integer())
    {
        return std::to_string(value.get<long long>());
    }
    if (value.is_number_unsigned())
    {
        return std::to_string(value.get<unsigned long long>());
    }
    if (value.is_number_float())
    {
        return std::to_string(value.get<double>());
    }
    if (value.is_null())
    {
        return "null";
    }
    return {};
}

std::string truncateLabel(std::string text)
{
    constexpr std::size_t kMaxLabel = 28;
    if (text.size() > kMaxLabel)
    {
        text.resize(kMaxLabel - 2);
        text += "..";
    }
    return text;
}

constexpr int         kJsonMaxDepth = 12;
constexpr std::size_t kJsonMaxNodes = 256;

// Adds `value` as a node and recurses into its children, joining each by a
// parent->child edge. `counter` hands out unique node ids; the walk stops at
// the depth and node-count bounds so a huge document stays viewable.
void walkJson(DiagramModel &model, const nlohmann::json &value, const std::string &nodeId,
              const std::string &nodeLabel, int depth, std::size_t &counter)
{
    model.addNode(nodeId, nodeLabel);
    if (depth >= kJsonMaxDepth || counter >= kJsonMaxNodes)
    {
        return;
    }
    if (value.is_object())
    {
        for (auto member = value.begin(); member != value.end(); ++member)
        {
            if (counter >= kJsonMaxNodes)
            {
                break;
            }
            const std::string childId = "n" + std::to_string(counter++);
            std::string       childLabel = member.key();
            if (member.value().is_primitive())
            {
                childLabel += ": " + jsonScalarText(member.value());
            }
            walkJson(model, member.value(), childId, truncateLabel(childLabel), depth + 1, counter);
            model.addEdge(nodeId, childId);
        }
    }
    else if (value.is_array())
    {
        std::size_t index = 0;
        for (const auto &element : value)
        {
            if (counter >= kJsonMaxNodes)
            {
                break;
            }
            const std::string childId    = "n" + std::to_string(counter++);
            std::string       childLabel = "[" + std::to_string(index) + "]";
            if (element.is_primitive())
            {
                childLabel += " " + jsonScalarText(element);
            }
            walkJson(model, element, childId, truncateLabel(childLabel), depth + 1, counter);
            model.addEdge(nodeId, childId);
            ++index;
        }
    }
}
} // namespace

namespace
{
bool isIdentChar(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '.';
}

// First identifier run in `text` (skips leading punctuation, e.g. `[Foo!]!`
// yields `Foo`). Empty when none.
std::string firstIdentifier(std::string_view text)
{
    std::size_t start = 0;
    while (start < text.size() && !isIdentChar(text[start]))
    {
        ++start;
    }
    std::size_t end = start;
    while (end < text.size() && isIdentChar(text[end]))
    {
        ++end;
    }
    return std::string(text.substr(start, end - start));
}

std::string asciiLower(std::string_view text)
{
    std::string out(text);
    for (char &ch : out)
    {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return out;
}

std::size_t countChar(std::string_view text, char target)
{
    std::size_t total = 0;
    for (const char ch : text)
    {
        if (ch == target)
        {
            ++total;
        }
    }
    return total;
}

// True when `line` opens with `keyword` followed by whitespace.
bool startsWithKeyword(std::string_view line, std::string_view keyword)
{
    return line.rfind(keyword, 0) == 0 && line.size() > keyword.size() &&
           isSpace(line[keyword.size()]);
}
} // namespace

std::optional<DiagramModel> EdgeListImporter::import(std::string_view text) const
{
    DiagramModel model;
    for (const std::string_view raw : splitAny(text, "\n"))
    {
        const std::string_view line = trim(raw);
        if (line.empty() || line.front() == '#' || line.rfind("//", 0) == 0)
        {
            continue;
        }
        const std::vector<std::string_view> tokens = splitAny(line, " \t");
        if (tokens.empty())
        {
            continue;
        }
        const std::string source = unquote(tokens.front());
        if (tokens.size() == 1)
        {
            model.addNode(source);
            continue;
        }
        for (std::size_t index = 1; index < tokens.size(); ++index)
        {
            model.addEdge(source, unquote(tokens[index]));
        }
    }

    if (model.nodes.empty())
    {
        return std::nullopt;
    }
    return model;
}

std::optional<DiagramModel> JsonTreeImporter::import(std::string_view text) const
{
    const nlohmann::json root = nlohmann::json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (root.is_discarded())
    {
        return std::nullopt;
    }
    DiagramModel model;
    std::size_t  counter = 1; // root takes id n0
    std::string  rootLabel;
    if (root.is_object())
    {
        rootLabel = "object";
    }
    else if (root.is_array())
    {
        rootLabel = "array";
    }
    else
    {
        rootLabel = jsonScalarText(root);
    }
    walkJson(model, root, "n0", truncateLabel(rootLabel), 0, counter);

    if (model.nodes.empty())
    {
        return std::nullopt;
    }
    return model;
}

std::optional<DiagramModel> FlameGraphImporter::import(std::string_view text) const
{
    constexpr std::size_t kFlameMaxNodes = 512;
    DiagramModel          model;
    for (const std::string_view raw : splitAny(text, "\n"))
    {
        const std::string_view line = trim(raw);
        if (line.empty() || line.front() == '#')
        {
            continue;
        }
        // The trailing token is a sample count when it is all digits; drop it.
        std::string_view stack     = line;
        const std::size_t lastSpace = line.rfind(' ');
        if (lastSpace != std::string_view::npos)
        {
            const std::string_view tail = line.substr(lastSpace + 1);
            if (!tail.empty() &&
                tail.find_first_not_of("0123456789") == std::string_view::npos)
            {
                stack = trim(line.substr(0, lastSpace));
            }
        }
        std::string parentPath;
        std::string path;
        for (const std::string_view frame : splitAny(stack, ";"))
        {
            if (model.nodes.size() >= kFlameMaxNodes)
            {
                break;
            }
            const std::string framePath = path.empty() ? std::string(frame) : path + ";" + std::string(frame);
            model.addNode(framePath, std::string(frame)); // label = leaf frame name
            if (!parentPath.empty())
            {
                model.addEdge(parentPath, framePath);
            }
            parentPath = framePath;
            path       = framePath;
        }
    }

    if (model.nodes.empty())
    {
        return std::nullopt;
    }
    return model;
}

std::optional<DiagramModel> GraphQlImporter::import(std::string_view text) const
{
    const std::array<std::string_view, 6> kBlockKeywords = {"type", "interface", "input", "enum",
                                                            "union", "scalar"};
    const std::vector<std::string_view>   lines = splitAny(text, "\n");

    // Pass 1: every declaration is a node; remember the names so a field edge is
    // only raised between declared types (not to scalars like String/Int).
    DiagramModel          model;
    std::set<std::string> declared;
    for (const std::string_view raw : lines)
    {
        const std::string_view line = trim(raw);
        for (const std::string_view keyword : kBlockKeywords)
        {
            if (startsWithKeyword(line, keyword))
            {
                const std::string name = firstIdentifier(line.substr(keyword.size()));
                if (!name.empty())
                {
                    declared.insert(name);
                    model.addNode(name);
                }
                break;
            }
        }
    }

    // Pass 2: walk the blocks; a field's return type and an `implements`/`union`
    // member that names a declared type becomes an edge.
    std::string current;
    std::string pending;
    int         depth = 0;
    for (const std::string_view raw : lines)
    {
        const std::string_view line = trim(raw);
        if (depth == 0 && startsWithKeyword(line, "union"))
        {
            const std::string name = firstIdentifier(line.substr(5));
            const std::size_t eq   = line.find('=');
            if (!name.empty() && eq != std::string_view::npos)
            {
                for (const std::string_view member : splitAny(line.substr(eq + 1), "|"))
                {
                    const std::string target = firstIdentifier(member);
                    if (declared.count(target) != 0 && target != name)
                    {
                        model.addEdge(name, target);
                    }
                }
            }
            continue;
        }
        if (depth == 0)
        {
            static constexpr std::array<std::string_view, 4> kFieldBlockKeywords = {
                "type", "interface", "input", "enum"};
            for (const std::string_view keyword : kFieldBlockKeywords)
            {
                if (startsWithKeyword(line, keyword))
                {
                    pending = firstIdentifier(line.substr(keyword.size()));
                    const std::size_t impl = line.find("implements");
                    if (impl != std::string_view::npos)
                    {
                        const std::string base = firstIdentifier(line.substr(impl + 10));
                        if (declared.count(base) != 0 && base != pending)
                        {
                            model.addEdge(pending, base);
                        }
                    }
                    break;
                }
            }
        }
        const std::size_t opens    = countChar(line, '{');
        const std::size_t closes   = countChar(line, '}');
        const bool        entering = (depth == 0 && opens > 0);
        if (entering)
        {
            current = pending;
        }
        if ((depth > 0 || entering) && !current.empty())
        {
            // Every `: Type` on the line is a field whose type, when declared,
            // raises an edge -- handles one-field-per-line and inline bodies
            // like `type X { a: Y b: Z }`. Enum value lines carry no colon.
            std::size_t colon = 0;
            while ((colon = line.find(':', colon)) != std::string_view::npos)
            {
                const std::string fieldType = firstIdentifier(line.substr(colon + 1));
                if (declared.count(fieldType) != 0 && fieldType != current)
                {
                    model.addEdge(current, fieldType);
                }
                ++colon;
            }
        }
        depth += static_cast<int>(opens) - static_cast<int>(closes);
        if (depth <= 0)
        {
            depth   = 0;
            current.clear();
        }
    }

    if (model.nodes.empty())
    {
        return std::nullopt;
    }
    return model;
}

std::optional<DiagramModel> ProtobufImporter::import(std::string_view text) const
{
    const std::vector<std::string_view> lines = splitAny(text, "\n");

    // Pass 1: every message/enum (including nested) is a node.
    DiagramModel          model;
    std::set<std::string> declared;
    for (const std::string_view raw : lines)
    {
        const std::string_view line = trim(raw);
        if (startsWithKeyword(line, "message") || startsWithKeyword(line, "enum"))
        {
            const std::size_t keywordLen = startsWithKeyword(line, "message") ? 7 : 4;
            const std::string name       = firstIdentifier(line.substr(keywordLen));
            if (!name.empty())
            {
                declared.insert(name);
                model.addNode(name);
            }
        }
    }

    // The declared type a single field statement points at, or empty when the
    // field is a scalar or the statement is not a field. Handles `map<K,V>`
    // (value type) and a `[repeated|optional|required] Type name = N` field.
    const auto fieldTypeOf = [](std::string_view statement) -> std::string {
        statement              = trim(statement);
        const std::size_t mapPos = statement.find("map<");
        if (mapPos != std::string_view::npos)
        {
            const std::size_t comma = statement.find(',', mapPos);
            return comma == std::string_view::npos ? std::string{}
                                                   : firstIdentifier(statement.substr(comma + 1));
        }
        if (statement.find('=') == std::string_view::npos)
        {
            return {};
        }
        std::string_view scan = statement;
        static constexpr std::array<std::string_view, 3> kFieldModifiers = {"repeated", "optional",
                                                                            "required"};
        for (const std::string_view modifier : kFieldModifiers)
        {
            if (startsWithKeyword(scan, modifier))
            {
                scan = trim(scan.substr(modifier.size()));
                break;
            }
        }
        return firstIdentifier(scan);
    };

    // Pass 2: a field whose type names a declared message/enum is an edge from
    // the enclosing message. `current` follows the most recent top-level
    // message (nested declarations still render as nodes from pass 1). Field
    // statements are scanned per `;`, so an inline `message X { A a = 1; }` and
    // a one-field-per-line body both resolve.
    std::string current;
    int         depth = 0;
    for (const std::string_view raw : lines)
    {
        const std::string_view line      = trim(raw);
        const bool             isMessage = startsWithKeyword(line, "message");
        const bool             isEnum    = startsWithKeyword(line, "enum");
        const bool             entering  = depth == 0 && (isMessage || isEnum);
        if (entering)
        {
            current = firstIdentifier(line.substr(isMessage ? 7 : 4));
        }
        if (!current.empty() && (depth > 0 || entering))
        {
            // Scan the field body -- after the opening brace on a declaration
            // line, otherwise the whole line.
            std::string_view body = line;
            if (entering)
            {
                const std::size_t brace = line.find('{');
                body = brace == std::string_view::npos ? std::string_view{} : line.substr(brace + 1);
            }
            for (const std::string_view statement : splitAny(body, ";"))
            {
                const std::string fieldType = fieldTypeOf(statement);
                if (declared.count(fieldType) != 0 && fieldType != current)
                {
                    model.addEdge(current, fieldType);
                }
            }
        }
        depth += static_cast<int>(countChar(line, '{')) - static_cast<int>(countChar(line, '}'));
        if (depth <= 0)
        {
            depth = 0;
            current.clear();
        }
    }

    if (model.nodes.empty())
    {
        return std::nullopt;
    }
    return model;
}

std::optional<DiagramModel> SqlErdImporter::import(std::string_view text) const
{
    DiagramModel model;
    std::string  current;
    int          parenDepth = 0;
    for (const std::string_view raw : splitAny(text, "\n"))
    {
        const std::string_view line      = trim(raw);
        const std::string      lowerLine = asciiLower(line);

        if (parenDepth == 0)
        {
            const std::size_t create = lowerLine.find("create table");
            if (create != std::string_view::npos)
            {
                std::string_view rest = line.substr(create + 12);
                // Skip an optional `if not exists`.
                const std::string lowerRest = asciiLower(rest);
                if (lowerRest.find("if not exists") != std::string::npos)
                {
                    rest = rest.substr(asciiLower(rest).find("exists") + 6);
                }
                current = firstIdentifier(rest);
                model.addNode(current);
            }
        }
        if (!current.empty())
        {
            std::size_t refScan = 0;
            while ((refScan = lowerLine.find("references", refScan)) != std::string::npos)
            {
                const std::string target = firstIdentifier(line.substr(refScan + 10));
                if (!target.empty() && target != current)
                {
                    model.addEdge(current, target);
                }
                refScan += 10;
            }
        }
        parenDepth += static_cast<int>(countChar(line, '(')) - static_cast<int>(countChar(line, ')'));
        if (parenDepth <= 0)
        {
            parenDepth = 0;
            // A statement terminator closes the table scope.
            if (line.find(';') != std::string_view::npos)
            {
                current.clear();
            }
        }
    }

    if (model.nodes.empty())
    {
        return std::nullopt;
    }
    return model;
}

} // namespace vigine::format
