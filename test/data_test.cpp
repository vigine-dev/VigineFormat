#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include "vigine/format/diagramimporters.h"
#include "vigine/format/identify.h"

namespace
{
using namespace vigine::format;

const DiagramNode *findByLabel(const DiagramModel &model, const std::string &label)
{
    const auto it = std::find_if(model.nodes.begin(), model.nodes.end(),
                                 [&label](const DiagramNode &node) { return node.label == label; });
    return it == model.nodes.end() ? nullptr : &*it;
}

TEST(YamlTreeImporter, MapAndSequenceBecomeTheContainmentTree)
{
    YamlTreeImporter yaml;
    const auto model = yaml.import("name: svc\nports:\n  - 80\n  - 443\ntls: true\n");
    ASSERT_TRUE(model.has_value());
    EXPECT_NE(findByLabel(*model, "name: svc"), nullptr);
    EXPECT_NE(findByLabel(*model, "[0] 80"), nullptr);
    EXPECT_NE(findByLabel(*model, "[1] 443"), nullptr);
    EXPECT_NE(findByLabel(*model, "tls: true"), nullptr);
}

TEST(TomlTreeImporter, TablesArraysAndScalars)
{
    TomlTreeImporter toml;
    const auto model = toml.import("title = \"demo\"\n[server]\nhost = \"localhost\"\nports = [80, 443]\n");
    ASSERT_TRUE(model.has_value());
    EXPECT_NE(findByLabel(*model, "title: demo"), nullptr);
    EXPECT_NE(findByLabel(*model, "server"), nullptr);
    EXPECT_NE(findByLabel(*model, "host: localhost"), nullptr);
    EXPECT_NE(findByLabel(*model, "[0] 80"), nullptr);
}

TEST(OpenApiImporter, JsonSpecBecomesPathsAndOperations)
{
    OpenApiImporter openapi;
    const auto model = openapi.import(R"({
        "openapi": "3.0.0",
        "info": {"title": "Pet Store"},
        "paths": {
            "/pets": {"get": {"summary": "List pets"}, "post": {"operationId": "createPet"}},
            "/pets/{id}": {"get": {"summary": "Get a pet"}}
        }
    })");
    ASSERT_TRUE(model.has_value());
    EXPECT_NE(findByLabel(*model, "Pet Store"), nullptr);
    EXPECT_NE(findByLabel(*model, "/pets"), nullptr);
    EXPECT_NE(findByLabel(*model, "GET List pets"), nullptr);
    EXPECT_NE(findByLabel(*model, "POST createPet"), nullptr);
}

TEST(OpenApiImporter, YamlSpecIsNormalised)
{
    OpenApiImporter openapi;
    const auto model = openapi.import(
        "openapi: 3.0.0\ninfo:\n  title: Tiny\npaths:\n  /ping:\n    get:\n      summary: Ping\n");
    ASSERT_TRUE(model.has_value());
    EXPECT_NE(findByLabel(*model, "Tiny"), nullptr);
    EXPECT_NE(findByLabel(*model, "GET Ping"), nullptr);
}

TEST(Identify, OpenApiBeatsItsCarrier)
{
    EXPECT_EQ(identifyContent("{\"openapi\":\"3.1.0\",\"paths\":{}}"), KnownFormat::OpenApi);
    EXPECT_EQ(identifyContent("openapi: 3.0.0\npaths: {}\n"), KnownFormat::OpenApi);
}

TEST(DataImporters, MalformedInputYieldsNoModel)
{
    EXPECT_FALSE(YamlTreeImporter{}.import("key: [unclosed").has_value());
    EXPECT_FALSE(TomlTreeImporter{}.import("= broken =").has_value());
    EXPECT_FALSE(OpenApiImporter{}.import("{\"no\":\"paths\"}").has_value());
}
} // namespace
