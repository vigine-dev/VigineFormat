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

The library is shared and speaks only through interfaces: an importer is
obtained from a factory (`createDotImporter()`, `createPdfImporter()`,
`createObjImporter()`, ...) as `IDiagramImporter`, `IDocumentImporter` or
`IPointCloudImporter`, and the model types keep their data private behind
accessors. Only symbols marked `VIGINEFORMAT_API` are exported; the bundled
third-party code stays hidden inside on every system.

No third-party type appears in a public header, so backends can be swapped
without touching consumers. Heavier format backends (YAML, PDF, 3D meshes)
arrive as vendored submodules behind per-family CMake options, keeping the
default build light.

Like its siblings VigineCrypto and VigineConfig, the library is consumed as a
single submodule exposing the `VigineFormat::VigineFormat` target.

## JSON codec and the value through interfaces

A consumer that holds interfaces needs no non-virtual symbol of `vigine::format` except the two factories. Values are
`IDataValue`, built by `createDataValueFactory()` (`IDataValueFactory`); the codec from `createJsonCodec()` reads text
with `parseValue` and writes it with `serialize(const IDataValue &, const JsonWriteOptions &)`: `indent` (-1 compact,
>= 0 as nlohmann's `dump(indent)`), `sortKeys` (every table in byte order) and `replaceInvalidUtf8`. `DataValue` implements
`IDataValue` and keeps its API, so it can be given wherever an `IDataValue` is asked. `kindName` is not part of the
interface path: a consumer names the kinds from the `DataKind` enum itself.

## Build

The build goes by the recipe, vigine.build.toml. The tests come from
VigineUnitTest.
