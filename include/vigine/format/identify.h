#pragma once

#include <cstdint>
#include <string_view>

namespace vigine::format
{
// Every format the library can currently recognise. Identification is
// CONTENT-FIRST: magic bytes and structural probes decide, because a file
// extension is only a naming convention that can lie or be missing; the
// extension serves as a hint that resolves what content alone cannot.
enum class KnownFormat : std::uint8_t
{
    Unknown,
    Dot,
    Mermaid,
    GraphMl,
    PlantUml,
    Json,
    EdgeList,
    GraphQl,
    Protobuf,
    Sql,
    FlameGraph,
    Yaml,
    Toml,
    Xml,
    Pdf,
    OpenApi,
};

[[nodiscard]] std::string_view formatName(KnownFormat format) noexcept;

// Identify by content alone (magic bytes + structural probes). Unknown when
// nothing matches confidently.
[[nodiscard]] KnownFormat identifyContent(std::string_view content) noexcept;

// Identify by the filename extension alone. Unknown for unmapped extensions.
[[nodiscard]] KnownFormat identifyExtension(std::string_view fileName) noexcept;

// Content first, extension as the tie-breaker: when the probes recognise the
// bytes, that verdict stands; only an inconclusive probe falls back to the
// extension hint.
[[nodiscard]] KnownFormat identify(std::string_view fileName, std::string_view content) noexcept;

} // namespace vigine::format
