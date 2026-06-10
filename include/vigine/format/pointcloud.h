#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vigine::format
{
// ENCAP EXEMPT: pure value aggregates.
struct Vec3f
{
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

// The neutral output of every 3D importer: positions plus optional wireframe
// edges (index pairs into `points`). A renderer that can draw dots and lines
// can show any of these formats; nothing here depends on a math or loader
// library.
struct PointCloud
{
    std::vector<Vec3f>                                points;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> edges;
};

// One 3D format parsed into the neutral point cloud. `data` is the raw file
// content -- text or binary -- and nullopt means "not this format / broken".
class IPointCloudImporter
{
  public:
    virtual ~IPointCloudImporter() = default;
    [[nodiscard]] virtual std::string formatName() const = 0;
    [[nodiscard]] virtual std::optional<PointCloud> import(std::string_view data) const = 0;
};

// Wavefront OBJ: `v x y z` vertices and `f a b c ...` faces (1-based indices,
// `a/b/c` attribute slashes tolerated); faces contribute wireframe edges.
class ObjImporter final : public IPointCloudImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "obj"; }
    [[nodiscard]] std::optional<PointCloud> import(std::string_view data) const override;
};

// STL, both flavours: ASCII (`solid` / `facet` / `vertex`) and binary
// (80-byte header, uint32 triangle count, 50-byte records). Triangles
// contribute their three corner points and wireframe edges.
class StlImporter final : public IPointCloudImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "stl"; }
    [[nodiscard]] std::optional<PointCloud> import(std::string_view data) const override;
};

// ASCII PLY: the header names the vertex/face elements and property order;
// vertex rows supply points, face rows (index lists) supply wireframe edges.
class PlyImporter final : public IPointCloudImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "ply"; }
    [[nodiscard]] std::optional<PointCloud> import(std::string_view data) const override;
};

// Plain XYZ point lists: one `x y z [anything]` per line, `#` comments.
class XyzImporter final : public IPointCloudImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "xyz"; }
    [[nodiscard]] std::optional<PointCloud> import(std::string_view data) const override;
};

// MagicaVoxel .vox: the chunk stream's XYZI payloads become one point per
// voxel (multiple models accumulate).
class VoxImporter final : public IPointCloudImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "vox"; }
    [[nodiscard]] std::optional<PointCloud> import(std::string_view data) const override;
};

} // namespace vigine::format
