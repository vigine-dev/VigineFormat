#include "vigine/format/identify.h"
#include "vigine/format/pointcloud.h"

#include <cstring>
#include <string>
#include <vigine/unittest/unittest.h>

namespace
{
using namespace vigine::format;

VIGINE_TEST(ObjImporter, VerticesAndFaceEdges)
{
    const auto obj = createObjImporter();
    const auto cloud = obj->import("# cube corner\nv 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
    VIGINE_ASSERT_TRUE(cloud.has_value());
    VIGINE_EXPECT_EQ(cloud->points().size(), 3u);
    VIGINE_EXPECT_EQ(cloud->edges().size(), 3u); // triangle wireframe
    VIGINE_EXPECT_FLOAT_EQ(cloud->points()[1].x(), 1.0f);
}

VIGINE_TEST(ObjImporter, SlashedFaceTokensResolve)
{
    const auto obj = createObjImporter();
    const auto cloud = obj->import("v 0 0 0\nv 1 1 1\nv 2 2 2\nf 1/1/1 2/2/2 3/3/3\n");
    VIGINE_ASSERT_TRUE(cloud.has_value());
    VIGINE_EXPECT_EQ(cloud->edges().size(), 3u);
}

VIGINE_TEST(StlImporter, AsciiTriangles)
{
    const auto stl = createStlImporter();
    const auto cloud = stl->import(
        "solid demo\n facet normal 0 0 1\n  outer loop\n   vertex 0 0 0\n   vertex 1 0 0\n"
        "   vertex 0 1 0\n  endloop\n endfacet\nendsolid demo\n");
    VIGINE_ASSERT_TRUE(cloud.has_value());
    VIGINE_EXPECT_EQ(cloud->points().size(), 3u);
    VIGINE_EXPECT_EQ(cloud->edges().size(), 3u);
}

VIGINE_TEST(StlImporter, BinaryTriangles)
{
    std::string blob(84, '\0');
    const std::uint32_t count = 1;
    std::memcpy(blob.data() + 80, &count, 4);
    float record[12] = {0, 0, 1, /*v0*/ 0, 0, 0, /*v1*/ 1, 0, 0, /*v2*/ 0, 1, 0};
    std::string triangle(50, '\0');
    std::memcpy(triangle.data(), record, sizeof(record));
    blob += triangle;

    const auto stl = createStlImporter();
    const auto cloud = stl->import(blob);
    VIGINE_ASSERT_TRUE(cloud.has_value());
    VIGINE_EXPECT_EQ(cloud->points().size(), 3u);
    VIGINE_EXPECT_FLOAT_EQ(cloud->points()[1].x(), 1.0f);
}

VIGINE_TEST(PlyImporter, AsciiVerticesAndFaces)
{
    const auto ply = createPlyImporter();
    const auto cloud =
        ply->import("ply\nformat ascii 1.0\nelement vertex 3\nproperty float x\nproperty float y\n"
                   "property float z\nelement face 1\nproperty list uchar int vertex_indices\n"
                   "end_header\n0 0 0\n1 0 0\n0 1 0\n3 0 1 2\n");
    VIGINE_ASSERT_TRUE(cloud.has_value());
    VIGINE_EXPECT_EQ(cloud->points().size(), 3u);
    VIGINE_EXPECT_EQ(cloud->edges().size(), 3u);
}

VIGINE_TEST(XyzImporter, PlainPoints)
{
    const auto xyz = createXyzImporter();
    const auto cloud = xyz->import("# scan\n0 0 0\n1.5 2.5 3.5 255 0 0\n");
    VIGINE_ASSERT_TRUE(cloud.has_value());
    VIGINE_EXPECT_EQ(cloud->points().size(), 2u);
    VIGINE_EXPECT_FLOAT_EQ(cloud->points()[1].z(), 3.5f);
}

VIGINE_TEST(VoxImporter, ChunkStreamVoxels)
{
    // Minimal VOX: magic+version, MAIN (children only), SIZE, XYZI with 2 voxels.
    std::string blob    = "VOX ";
    const auto append32 = [&blob](std::uint32_t value) {
        char bytes[4];
        std::memcpy(bytes, &value, 4);
        blob.append(bytes, 4);
    };
    append32(150); // version
    blob += "MAIN";
    append32(0);
    append32(52); // children: SIZE(24) + XYZI(28)
    blob += "SIZE";
    append32(12);
    append32(0);
    append32(2);
    append32(2);
    append32(1);
    blob += "XYZI";
    append32(12);
    append32(0);
    append32(2);
    blob.push_back(0);
    blob.push_back(0);
    blob.push_back(0);
    blob.push_back(1); // voxel A
    blob.push_back(1);
    blob.push_back(1);
    blob.push_back(0);
    blob.push_back(2); // voxel B

    const auto vox = createVoxImporter();
    const auto cloud = vox->import(blob);
    VIGINE_ASSERT_TRUE(cloud.has_value());
    VIGINE_EXPECT_EQ(cloud->points().size(), 2u);
}

VIGINE_TEST(Identify, ThreeDMagicAndExtensions)
{
    VIGINE_EXPECT_EQ(identifyContent("ply\nformat ascii 1.0\nend_header\n"), KnownFormat::Ply);
    VIGINE_EXPECT_EQ(identifyContent("solid x\nfacet normal 0 0 1\n"), KnownFormat::Stl);
    VIGINE_EXPECT_EQ(identifyContent(std::string("VOX \x96\x00\x00\x00", 8)), KnownFormat::Vox);
    VIGINE_EXPECT_EQ(identifyExtension("model.obj"), KnownFormat::Obj);
    VIGINE_EXPECT_EQ(identifyExtension("scan.xyz"), KnownFormat::Xyz);
}
} // namespace
