#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vigine::format
{
// A neutral diagram: nodes and directed/undirected edges with labels. Every
// developer diagram format (Mermaid, DOT, UML, an ER diagram, a call graph,
// ...) imports into this one model, which then feeds the generic layout +
// renderer. The engine assigns no meaning to ids/labels/kinds -- a consumer
// maps them. ENCAP EXEMPT: pure value aggregates.
struct DiagramNode
{
    std::string id;
    std::string label;
    std::uint32_t kind{0};
};

struct DiagramEdge
{
    std::string from;
    std::string to;
    std::string label;
    bool directed{true};
};

struct DiagramModel
{
    std::vector<DiagramNode> nodes;
    std::vector<DiagramEdge> edges;

    // Adds a node if absent (or fills a missing label), returning nothing.
    void addNode(std::string_view id, std::string_view label = std::string_view{});
    void addEdge(std::string_view from, std::string_view to, std::string_view label = {},
                 bool directed = true);
};

// Parses one diagram text format into the neutral model. A per-format
// importer implements this; the engine never special-cases a format.
class IDiagramImporter
{
  public:
    virtual ~IDiagramImporter() = default;

    [[nodiscard]] virtual std::string formatName() const = 0;

    // Returns the parsed model, or no value when the text is not this format
    // / cannot be parsed at all.
    [[nodiscard]] virtual std::optional<DiagramModel> import(std::string_view text) const = 0;

  protected:
    IDiagramImporter() = default;

  public:
    IDiagramImporter(const IDiagramImporter &)            = delete;
    IDiagramImporter &operator=(const IDiagramImporter &) = delete;
    IDiagramImporter(IDiagramImporter &&)                 = delete;
    IDiagramImporter &operator=(IDiagramImporter &&)      = delete;
};

} // namespace vigine::format
