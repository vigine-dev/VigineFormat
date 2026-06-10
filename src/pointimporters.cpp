#include "vigine/format/pointcloud.h"

#include <array>
#include <cctype>
#include <charconv>
#include <cstring>
#include <sstream>
#include <string>

namespace vigine::format
{
namespace
{
constexpr std::size_t kMaxPoints = 2'000'000;
constexpr std::size_t kMaxEdges  = 4'000'000;

std::string_view trimView(std::string_view text)
{
    std::size_t begin = 0;
    std::size_t end   = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0)
        ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0)
        --end;
    return text.substr(begin, end - begin);
}

// Walks `text` line by line without allocating.
template <typename Fn> void forEachLine(std::string_view text, Fn &&fn)
{
    std::size_t start = 0;
    while (start <= text.size())
    {
        std::size_t end = text.find('\n', start);
        if (end == std::string_view::npos)
            end = text.size();
        fn(trimView(text.substr(start, end - start)));
        start = end + 1;
        if (end == text.size())
            break;
    }
}

std::vector<std::string_view> splitWhitespace(std::string_view line)
{
    std::vector<std::string_view> tokens;
    std::size_t                   index = 0;
    while (index < line.size())
    {
        while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) != 0)
            ++index;
        const std::size_t start = index;
        while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) == 0)
            ++index;
        if (index > start)
            tokens.push_back(line.substr(start, index - start));
    }
    return tokens;
}

bool parseFloat(std::string_view token, float &out)
{
    // std::from_chars<float> is still spotty on some toolchains; strtof on a
    // bounded copy is portable and exact enough here.
    char buffer[64];
    const std::size_t length = std::min(token.size(), sizeof(buffer) - 1);
    std::memcpy(buffer, token.data(), length);
    buffer[length] = '\0';
    char *parseEnd = nullptr;
    out            = std::strtof(buffer, &parseEnd);
    return parseEnd != buffer;
}

bool parseIndex(std::string_view token, long &out)
{
    // OBJ face tokens may carry attribute slashes: `7/1/3` -> 7.
    const std::size_t slash = token.find('/');
    if (slash != std::string_view::npos)
        token = token.substr(0, slash);
    const auto result = std::from_chars(token.data(), token.data() + token.size(), out);
    return result.ec == std::errc{};
}

void addPolygonEdges(PointCloud &cloud, const std::vector<std::uint32_t> &polygon)
{
    if (polygon.size() < 2 || cloud.edges.size() >= kMaxEdges)
        return;
    for (std::size_t index = 0; index + 1 < polygon.size(); ++index)
        cloud.edges.emplace_back(polygon[index], polygon[index + 1]);
    if (polygon.size() > 2)
        cloud.edges.emplace_back(polygon.back(), polygon.front());
}

std::uint32_t readU32LE(const unsigned char *bytes)
{
    return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

float readF32LE(const unsigned char *bytes)
{
    const std::uint32_t raw = readU32LE(bytes);
    float               out;
    std::memcpy(&out, &raw, sizeof(out));
    return out;
}
} // namespace

std::optional<PointCloud> ObjImporter::import(std::string_view data) const
{
    PointCloud cloud;
    forEachLine(data, [&](std::string_view line) {
        if (line.empty() || line.front() == '#' || cloud.points.size() >= kMaxPoints)
            return;
        const auto tokens = splitWhitespace(line);
        if (tokens.size() >= 4 && tokens[0] == "v")
        {
            Vec3f point;
            if (parseFloat(tokens[1], point.x) && parseFloat(tokens[2], point.y) &&
                parseFloat(tokens[3], point.z))
                cloud.points.push_back(point);
        }
        else if (tokens.size() >= 3 && tokens[0] == "f")
        {
            std::vector<std::uint32_t> polygon;
            polygon.reserve(tokens.size() - 1);
            for (std::size_t index = 1; index < tokens.size(); ++index)
            {
                long vertex = 0;
                if (!parseIndex(tokens[index], vertex))
                    return;
                // OBJ indices are 1-based; negatives count back from the end.
                const long resolved =
                    vertex > 0 ? vertex - 1 : static_cast<long>(cloud.points.size()) + vertex;
                if (resolved < 0 || resolved >= static_cast<long>(cloud.points.size()))
                    return;
                polygon.push_back(static_cast<std::uint32_t>(resolved));
            }
            addPolygonEdges(cloud, polygon);
        }
    });
    if (cloud.points.empty())
        return std::nullopt;
    return cloud;
}

std::optional<PointCloud> StlImporter::import(std::string_view data) const
{
    // ASCII flavour: "solid" plus at least one "facet".
    if (data.rfind("solid", 0) == 0 && data.find("facet") != std::string_view::npos)
    {
        PointCloud                 cloud;
        std::vector<std::uint32_t> triangle;
        forEachLine(data, [&](std::string_view line) {
            if (cloud.points.size() >= kMaxPoints)
                return;
            const auto tokens = splitWhitespace(line);
            if (tokens.size() >= 4 && tokens[0] == "vertex")
            {
                Vec3f point;
                if (parseFloat(tokens[1], point.x) && parseFloat(tokens[2], point.y) &&
                    parseFloat(tokens[3], point.z))
                {
                    triangle.push_back(static_cast<std::uint32_t>(cloud.points.size()));
                    cloud.points.push_back(point);
                    if (triangle.size() == 3)
                    {
                        addPolygonEdges(cloud, triangle);
                        triangle.clear();
                    }
                }
            }
        });
        if (cloud.points.empty())
            return std::nullopt;
        return cloud;
    }

    // Binary flavour: 80-byte header, uint32 count, 50 bytes per triangle.
    if (data.size() < 84)
        return std::nullopt;
    const auto *bytes = reinterpret_cast<const unsigned char *>(data.data());
    const std::uint32_t triangleCount = readU32LE(bytes + 80);
    if (triangleCount == 0 ||
        data.size() < 84 + static_cast<std::size_t>(triangleCount) * 50)
        return std::nullopt;

    PointCloud cloud;
    const std::uint32_t bounded =
        static_cast<std::uint32_t>(std::min<std::size_t>(triangleCount, kMaxPoints / 3));
    cloud.points.reserve(static_cast<std::size_t>(bounded) * 3);
    for (std::uint32_t index = 0; index < bounded; ++index)
    {
        const unsigned char *record = bytes + 84 + static_cast<std::size_t>(index) * 50;
        std::vector<std::uint32_t> triangle;
        for (int corner = 0; corner < 3; ++corner)
        {
            const unsigned char *vertex = record + 12 + corner * 12; // skip the normal
            triangle.push_back(static_cast<std::uint32_t>(cloud.points.size()));
            cloud.points.push_back(
                Vec3f{readF32LE(vertex), readF32LE(vertex + 4), readF32LE(vertex + 8)});
        }
        addPolygonEdges(cloud, triangle);
    }
    if (cloud.points.empty())
        return std::nullopt;
    return cloud;
}

std::optional<PointCloud> PlyImporter::import(std::string_view data) const
{
    if (data.rfind("ply", 0) != 0)
        return std::nullopt;
    const std::size_t headerEnd = data.find("end_header");
    if (headerEnd == std::string_view::npos)
        return std::nullopt;
    const std::string_view header = data.substr(0, headerEnd);
    if (header.find("format ascii") == std::string_view::npos)
        return std::nullopt; // binary PLY stays out of the v1 scope

    // Parse the header: element order, counts, and where x/y/z sit among the
    // vertex properties.
    std::size_t vertexCount = 0;
    std::size_t faceCount   = 0;
    int         xSlot = -1, ySlot = -1, zSlot = -1;
    int         propertySlot   = 0;
    bool        inVertexElement = false;
    bool        vertexFirst     = true;
    bool        sawFaceElement  = false;
    forEachLine(header, [&](std::string_view line) {
        const auto tokens = splitWhitespace(line);
        if (tokens.size() >= 3 && tokens[0] == "element")
        {
            inVertexElement = tokens[1] == "vertex";
            if (inVertexElement)
            {
                long parsed = 0;
                (void)std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), parsed);
                vertexCount = static_cast<std::size_t>(std::max(0L, parsed));
                vertexFirst = !sawFaceElement;
            }
            else if (tokens[1] == "face")
            {
                long parsed = 0;
                (void)std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), parsed);
                faceCount      = static_cast<std::size_t>(std::max(0L, parsed));
                sawFaceElement = true;
            }
            propertySlot = 0;
        }
        else if (inVertexElement && tokens.size() >= 3 && tokens[0] == "property" &&
                 tokens[1] != "list")
        {
            if (tokens[2] == "x")
                xSlot = propertySlot;
            else if (tokens[2] == "y")
                ySlot = propertySlot;
            else if (tokens[2] == "z")
                zSlot = propertySlot;
            ++propertySlot;
        }
    });
    if (vertexCount == 0 || xSlot < 0 || ySlot < 0 || zSlot < 0 || !vertexFirst)
        return std::nullopt;

    PointCloud  cloud;
    std::size_t consumedVertices = 0;
    std::size_t consumedFaces    = 0;
    const std::string_view body  = data.substr(data.find('\n', headerEnd) + 1);
    forEachLine(body, [&](std::string_view line) {
        if (line.empty())
            return;
        const auto tokens = splitWhitespace(line);
        if (consumedVertices < vertexCount)
        {
            const int maxSlot = std::max({xSlot, ySlot, zSlot});
            if (static_cast<int>(tokens.size()) <= maxSlot)
                return;
            Vec3f point;
            if (parseFloat(tokens[static_cast<std::size_t>(xSlot)], point.x) &&
                parseFloat(tokens[static_cast<std::size_t>(ySlot)], point.y) &&
                parseFloat(tokens[static_cast<std::size_t>(zSlot)], point.z) &&
                cloud.points.size() < kMaxPoints)
                cloud.points.push_back(point);
            ++consumedVertices;
        }
        else if (consumedFaces < faceCount)
        {
            if (tokens.empty())
                return;
            std::vector<std::uint32_t> polygon;
            for (std::size_t index = 1; index < tokens.size(); ++index)
            {
                long vertex = 0;
                if (!parseIndex(tokens[index], vertex) || vertex < 0 ||
                    vertex >= static_cast<long>(cloud.points.size()))
                    return;
                polygon.push_back(static_cast<std::uint32_t>(vertex));
            }
            addPolygonEdges(cloud, polygon);
            ++consumedFaces;
        }
    });
    if (cloud.points.empty())
        return std::nullopt;
    return cloud;
}

std::optional<PointCloud> XyzImporter::import(std::string_view data) const
{
    PointCloud cloud;
    forEachLine(data, [&](std::string_view line) {
        if (line.empty() || line.front() == '#' || cloud.points.size() >= kMaxPoints)
            return;
        const auto tokens = splitWhitespace(line);
        if (tokens.size() < 3)
            return;
        Vec3f point;
        if (parseFloat(tokens[0], point.x) && parseFloat(tokens[1], point.y) &&
            parseFloat(tokens[2], point.z))
            cloud.points.push_back(point);
    });
    if (cloud.points.empty())
        return std::nullopt;
    return cloud;
}

std::optional<PointCloud> VoxImporter::import(std::string_view data) const
{
    if (data.size() < 8 || data.rfind("VOX ", 0) != 0)
        return std::nullopt;
    const auto *bytes = reinterpret_cast<const unsigned char *>(data.data());

    PointCloud cloud;
    // Walk the chunk stream: every chunk is id(4) + contentSize(4) +
    // childrenSize(4) + content. Children follow their parent's content
    // immediately, so advancing by content size alone naturally descends.
    std::size_t offset = 8; // 'VOX ' + version
    while (offset + 12 <= data.size())
    {
        const std::string_view chunkId     = data.substr(offset, 4);
        const std::uint32_t    contentSize = readU32LE(bytes + offset + 4);
        offset += 12;
        if (offset + contentSize > data.size())
            break;
        if (chunkId == "XYZI" && contentSize >= 4)
        {
            const std::uint32_t voxelCount = readU32LE(bytes + offset);
            const std::uint32_t bounded    = static_cast<std::uint32_t>(std::min<std::size_t>(
                voxelCount, std::min<std::size_t>((contentSize - 4) / 4,
                                                  kMaxPoints - cloud.points.size())));
            for (std::uint32_t index = 0; index < bounded; ++index)
            {
                const unsigned char *voxel = bytes + offset + 4 + static_cast<std::size_t>(index) * 4;
                cloud.points.push_back(Vec3f{static_cast<float>(voxel[0]),
                                             static_cast<float>(voxel[2]),
                                             static_cast<float>(voxel[1])});
            }
            offset += contentSize;
        }
        else if (chunkId == "MAIN")
        {
            offset += contentSize; // descend straight into the children
        }
        else
        {
            offset += contentSize; // skip unrelated chunk content
        }
        if (cloud.points.size() >= kMaxPoints)
            break;
    }
    if (cloud.points.empty())
        return std::nullopt;
    return cloud;
}

} // namespace vigine::format
