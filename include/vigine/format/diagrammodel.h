#pragma once

#include "vigine/format/vigineformat_export.h"

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
// maps them.
class VIGINEFORMAT_API DiagramNode
{
  public:
    DiagramNode() = default;
    DiagramNode(std::string id, std::string label, std::uint32_t kind = 0);

    [[nodiscard]] const std::string &id() const noexcept { return _id; }
    [[nodiscard]] const std::string &label() const noexcept { return _label; }
    [[nodiscard]] std::uint32_t kind() const noexcept { return _kind; }

    void setLabel(std::string_view label);
    void setKind(std::uint32_t kind) noexcept { _kind = kind; }

  private:
    std::string _id;
    std::string _label;
    std::uint32_t _kind{0};
};

class VIGINEFORMAT_API DiagramEdge
{
  public:
    DiagramEdge() = default;
    DiagramEdge(std::string from, std::string to, std::string label, bool directed = true);

    [[nodiscard]] const std::string &from() const noexcept { return _from; }
    [[nodiscard]] const std::string &to() const noexcept { return _to; }
    [[nodiscard]] const std::string &label() const noexcept { return _label; }
    [[nodiscard]] bool directed() const noexcept { return _directed; }

  private:
    std::string _from;
    std::string _to;
    std::string _label;
    bool _directed{true};
};

class VIGINEFORMAT_API DiagramModel
{
  public:
    [[nodiscard]] const std::vector<DiagramNode> &nodes() const noexcept { return _nodes; }
    [[nodiscard]] const std::vector<DiagramEdge> &edges() const noexcept { return _edges; }

    // Adds a node if absent (or fills a missing label), returning nothing.
    void addNode(std::string_view id, std::string_view label = std::string_view{});
    void addEdge(std::string_view from, std::string_view to, std::string_view label = {},
                 bool directed = true);

  private:
    std::vector<DiagramNode> _nodes;
    std::vector<DiagramEdge> _edges;
};

// Parses one diagram text format into the neutral model. A per-format
// importer implements this; the engine never special-cases a format.
class VIGINEFORMAT_API IDiagramImporter
{
  public:
    virtual ~IDiagramImporter();

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
