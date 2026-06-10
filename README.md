# VigineFormat

The file-format library of the Vigine family: it identifies what a file is and
parses it into a neutral model the engine can lay out and render.

Two layers:

1. **Identify** — content-first recognition (magic bytes and structural
   probes); the filename extension is only a hint that resolves what the bytes
   alone cannot. An extension can lie, the bytes cannot.
2. **Parse** — one importer per format, all emitting the same neutral
   `DiagramModel` (nodes + labelled directed/undirected edges). Today:
   Graphviz DOT, Mermaid flowcharts, GraphML, PlantUML class diagrams, JSON
   (as a containment tree), edge lists (git DAGs, build graphs, any adjacency
   dump), GraphQL SDL, Protobuf, SQL DDL (entity-relationship), and folded
   flame-graph stacks.

No third-party type appears in a public header, so backends can be swapped
without touching consumers. Heavier format backends (YAML, PDF, 3D meshes)
arrive as vendored submodules behind per-family CMake options, keeping the
default build light.

Like its siblings VigineCrypto and VigineConfig, the library is consumed as a
single submodule exposing the `VigineFormat::VigineFormat` target.
