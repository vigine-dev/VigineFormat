#pragma once

#include "vigine/format/vigineformat_export.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vigine::format
{
class VIGINEFORMAT_API Vec3f
{
  public:
    Vec3f() = default;
    Vec3f(float x, float y, float z) noexcept : _x{x}, _y{y}, _z{z} {}

    [[nodiscard]] float x() const noexcept { return _x; }
    [[nodiscard]] float y() const noexcept { return _y; }
    [[nodiscard]] float z() const noexcept { return _z; }

  private:
    float _x{0.0f};
    float _y{0.0f};
    float _z{0.0f};
};

// The neutral output of every 3D importer: positions plus optional wireframe
// edges (index pairs into `points`). A renderer that can draw dots and lines
// can show any of these formats; nothing here depends on a math or loader
// library.
class VIGINEFORMAT_API PointCloud
{
  public:
    using Edge = std::pair<std::uint32_t, std::uint32_t>;

    [[nodiscard]] const std::vector<Vec3f> &points() const noexcept { return _points; }
    [[nodiscard]] const std::vector<Edge> &edges() const noexcept { return _edges; }

    void addPoint(const Vec3f &point) { _points.push_back(point); }
    void addEdge(std::uint32_t from, std::uint32_t to) { _edges.emplace_back(from, to); }
    void reservePoints(std::size_t count) { _points.reserve(count); }

  private:
    std::vector<Vec3f> _points;
    std::vector<Edge> _edges;
};

// One 3D format parsed into the neutral point cloud. `data` is the raw file
// content -- text or binary -- and nullopt means "not this format / broken".
class VIGINEFORMAT_API IPointCloudImporter
{
  public:
    virtual ~IPointCloudImporter()                                                      = default;
    [[nodiscard]] virtual std::string formatName() const                                = 0;
    [[nodiscard]] virtual std::optional<PointCloud> import(std::string_view data) const = 0;

  protected:
    IPointCloudImporter() = default;

  public:
    IPointCloudImporter(const IPointCloudImporter &)            = delete;
    IPointCloudImporter &operator=(const IPointCloudImporter &) = delete;
    IPointCloudImporter(IPointCloudImporter &&)                 = delete;
    IPointCloudImporter &operator=(IPointCloudImporter &&)      = delete;
};

// The only way to obtain a 3D importer.

// Wavefront OBJ: `v x y z` vertices and `f a b c ...` faces (1-based indices,
// `a/b/c` attribute slashes tolerated); faces contribute wireframe edges.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IPointCloudImporter> createObjImporter();

// STL, both flavours: ASCII and binary; triangles contribute their corner
// points and wireframe edges.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IPointCloudImporter> createStlImporter();

// ASCII PLY: the header names the vertex/face elements and property order.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IPointCloudImporter> createPlyImporter();

// Plain XYZ point lists: one `x y z [anything]` per line, `#` comments.
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IPointCloudImporter> createXyzImporter();

// MagicaVoxel .vox: one point per voxel (multiple models accumulate).
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IPointCloudImporter> createVoxImporter();

} // namespace vigine::format
