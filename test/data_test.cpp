#include "vigine/format/diagramimporters.h"
#include "vigine/format/identify.h"

#include <algorithm>
#include <string>
#include <vigine/unittest/unittest.h>

namespace
{
using namespace vigine::format;

const DiagramNode *findByLabel(const DiagramModel &model, const std::string &label)
{
    const auto it = std::find_if(model.nodes().begin(), model.nodes().end(),
                                 [&label](const DiagramNode &node) { return node.label() == label; });
    return it == model.nodes().end() ? nullptr : &*it;
}

VIGINE_TEST(YamlTreeImporter, MapAndSequenceBecomeTheContainmentTree)
{
    const auto yaml = createYamlTreeImporter();
    const auto model = yaml->import("name: svc\nports:\n  - 80\n  - 443\ntls: true\n");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_EXPECT_NE(findByLabel(*model, "name: svc"), nullptr);
    VIGINE_EXPECT_NE(findByLabel(*model, "[0] 80"), nullptr);
    VIGINE_EXPECT_NE(findByLabel(*model, "[1] 443"), nullptr);
    VIGINE_EXPECT_NE(findByLabel(*model, "tls: true"), nullptr);
}

VIGINE_TEST(TomlTreeImporter, TablesArraysAndScalars)
{
    const auto toml = createTomlTreeImporter();
    const auto model =
        toml->import("title = \"demo\"\n[server]\nhost = \"localhost\"\nports = [80, 443]\n");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_EXPECT_NE(findByLabel(*model, "title: demo"), nullptr);
    VIGINE_EXPECT_NE(findByLabel(*model, "server"), nullptr);
    VIGINE_EXPECT_NE(findByLabel(*model, "host: localhost"), nullptr);
    VIGINE_EXPECT_NE(findByLabel(*model, "[0] 80"), nullptr);
}

VIGINE_TEST(OpenApiImporter, JsonSpecBecomesPathsAndOperations)
{
    const auto openapi = createOpenApiImporter();
    const auto model = openapi->import(R"({
        "openapi": "3.0.0",
        "info": {"title": "Pet Store"},
        "paths": {
            "/pets": {"get": {"summary": "List pets"}, "post": {"operationId": "createPet"}},
            "/pets/{id}": {"get": {"summary": "Get a pet"}}
        }
    })");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_EXPECT_NE(findByLabel(*model, "Pet Store"), nullptr);
    VIGINE_EXPECT_NE(findByLabel(*model, "/pets"), nullptr);
    VIGINE_EXPECT_NE(findByLabel(*model, "GET List pets"), nullptr);
    VIGINE_EXPECT_NE(findByLabel(*model, "POST createPet"), nullptr);
}

VIGINE_TEST(OpenApiImporter, YamlSpecIsNormalised)
{
    const auto openapi = createOpenApiImporter();
    const auto model = openapi->import(
        "openapi: 3.0.0\ninfo:\n  title: Tiny\npaths:\n  /ping:\n    get:\n      summary: Ping\n");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_EXPECT_NE(findByLabel(*model, "Tiny"), nullptr);
    VIGINE_EXPECT_NE(findByLabel(*model, "GET Ping"), nullptr);
}

VIGINE_TEST(Identify, OpenApiBeatsItsCarrier)
{
    VIGINE_EXPECT_EQ(identifyContent("{\"openapi\":\"3.1.0\",\"paths\":{}}"), KnownFormat::OpenApi);
    VIGINE_EXPECT_EQ(identifyContent("openapi: 3.0.0\npaths: {}\n"), KnownFormat::OpenApi);
}

VIGINE_TEST(DataImporters, MalformedInputYieldsNoModel)
{
    VIGINE_EXPECT_FALSE(createYamlTreeImporter()->import("key: [unclosed").has_value());
    VIGINE_EXPECT_FALSE(createTomlTreeImporter()->import("= broken =").has_value());
    VIGINE_EXPECT_FALSE(createOpenApiImporter()->import("{\"no\":\"paths\"}").has_value());
}
} // namespace
