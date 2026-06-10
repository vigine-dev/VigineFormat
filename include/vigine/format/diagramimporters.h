#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "vigine/format/diagrammodel.h"

namespace vigine::format
{
// Graphviz DOT importer: parses the common subset -- `a -> b` / `a -- b`
// edges (and chains), and node labels `id [label="..."]`. Graph/node/edge
// attribute statements are ignored.
class DotImporter final : public IDiagramImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "dot"; }
    [[nodiscard]] std::optional<DiagramModel> import(std::string_view text) const override;
};

// Mermaid flowchart importer: parses the common subset -- `A --> B` edges
// (and the dotted/thick variants), edge labels `A -- text --> B` and
// `A -->|text| B`, and node shapes `A[..]` / `A(..)` / `A{..}` / `A((..))`.
class MermaidImporter final : public IDiagramImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "mermaid"; }
    [[nodiscard]] std::optional<DiagramModel> import(std::string_view text) const override;
};

// GraphML importer: scans the `<node id="...">` and `<edge source=".."
// target="..">` elements of the GraphML XML and pulls the first `<data>` text
// inside a node element as its label. Parses the common subset without a full
// XML parser, the same approach the other importers take.
class GraphMlImporter final : public IDiagramImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "graphml"; }
    [[nodiscard]] std::optional<DiagramModel> import(std::string_view text) const override;
};

// Edge-list importer: each non-empty, non-comment line is `source target...`
// -- the first token is a node and every following token is a node joined by a
// source->target edge. Single-token lines are standalone nodes; `#` and `//`
// start comments. The generic shape behind a git commit DAG (`hash parents`),
// a build dependency list, or any adjacency dump, so one importer covers them.
class EdgeListImporter final : public IDiagramImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "edgelist"; }
    [[nodiscard]] std::optional<DiagramModel> import(std::string_view text) const override;
};

// JSON structured-data importer: parses a JSON document and turns it into a
// containment tree -- every object/array is a node, each member or element a
// child joined by a parent->child edge (objects label the child by key,
// arrays by index), and scalar leaves carry their value in the label. Bounded
// in depth and node count so a large document still yields a viewable tree.
class JsonTreeImporter final : public IDiagramImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "json"; }
    [[nodiscard]] std::optional<DiagramModel> import(std::string_view text) const override;
};

// PlantUML class-diagram importer: parses the common line-based subset --
// type declarations `class A` / `interface A` / `abstract class A` /
// `enum A`, and relationships `A --> B`, `A <|-- B`, `A *-- B`, `A o-- B`,
// `A ..> B` (and their dotted/headed variants) with an optional ` : label`.
// Member lines and `{ ... }` bodies are ignored.
class PlantUmlImporter final : public IDiagramImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "plantuml"; }
    [[nodiscard]] std::optional<DiagramModel> import(std::string_view text) const override;
};

// Flame-graph importer: folded/collapsed stack samples (`frameA;frameB;frameC
// count`, one stack per line) become a call-prefix tree -- each frame is a node
// keyed by its full path (so the same name under different parents stays
// distinct) labelled by the frame name, joined parent->child. The trailing
// sample count is ignored for the shape. Bounded in node count.
class FlameGraphImporter final : public IDiagramImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "flamegraph"; }
    [[nodiscard]] std::optional<DiagramModel> import(std::string_view text) const override;
};

// GraphQL SDL importer: each `type` / `interface` / `input` / `enum` / `union`
// / `scalar` declaration is a node, and a field whose type names another
// declared type becomes an edge (scalars like String/Int are not nodes, so
// they raise no edge). `union X = A | B` adds an edge to each member.
class GraphQlImporter final : public IDiagramImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "graphql"; }
    [[nodiscard]] std::optional<DiagramModel> import(std::string_view text) const override;
};

// Protobuf importer: each `message` / `enum` declaration is a node, and a field
// whose type names another declared message or enum becomes an edge (scalar
// field types raise no edge). `map<K,V>` adds an edge to a declared V.
class ProtobufImporter final : public IDiagramImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "protobuf"; }
    [[nodiscard]] std::optional<DiagramModel> import(std::string_view text) const override;
};

// SQL ERD importer: each `CREATE TABLE X` is a node and every `REFERENCES Y`
// (inline column reference or a table-level FOREIGN KEY) adds an edge X->Y, so
// a schema dump renders as an entity-relationship graph.
class SqlErdImporter final : public IDiagramImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "sql"; }
    [[nodiscard]] std::optional<DiagramModel> import(std::string_view text) const override;
};

} // namespace vigine::format
