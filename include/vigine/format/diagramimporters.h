#pragma once

#include "vigine/format/diagrammodel.h"
#include "vigine/format/vigineformat_export.h"

#include <memory>

namespace vigine::format
{
// The only way to obtain a diagram importer: each factory returns the importer
// of one text format behind the IDiagramImporter interface.

// Graphviz DOT: the common subset -- `a -> b` / `a -- b` edges (and chains) and node labels `id [label="..."]`; attribute statements are ignored.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createDotImporter();

// Mermaid flowchart: `A --> B` edges (and the dotted/thick variants), edge labels, and the node shapes `A[..]` / `A(..)` / `A{..}` / `A((..))`.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createMermaidImporter();

// GraphML: the `<node id>` and `<edge source target>` elements; the first `<data>` text inside a node is its label.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createGraphMlImporter();

// Edge list: each non-empty, non-comment line is `source target...`; the first token is a node and every following token is a node joined by a source->target edge.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createEdgeListImporter();

// JSON: a containment tree -- every object/array a node, each member or element a child; scalar leaves carry their value in the label. Bounded in depth and node count.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createJsonTreeImporter();

// PlantUML class diagram: type declarations and relationships of the common line-based subset.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createPlantUmlImporter();

// Flame graph: folded stack samples become a call-prefix tree. Bounded in node count.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createFlameGraphImporter();

// GraphQL SDL: declarations are nodes, a field naming another declared type is an edge.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createGraphQlImporter();

// Protobuf: messages and enums are nodes, a field naming another declared type is an edge.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createProtobufImporter();

// SQL ERD: every `CREATE TABLE` is a node, every `REFERENCES` an edge.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createSqlErdImporter();

// YAML: the same containment tree as the JSON importer builds.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createYamlTreeImporter();

// TOML: tables and arrays become nodes, entries their children -- the same tree shape as JSON and YAML.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createTomlTreeImporter();

// OpenAPI: the API surface as a graph -- title at the root, a node per path, a child per operation. Accepts the JSON and the YAML flavour.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDiagramImporter> createOpenApiImporter();

} // namespace vigine::format
