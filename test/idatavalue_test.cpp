#include "vigine/format/idatavalue.h"
#include "vigine/format/idatavaluefactory.h"
#include "vigine/format/jsoncodec.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vigine/unittest/unittest.h>

namespace
{
using namespace vigine::format;

VIGINE_TEST(DataValueInterface, BuildsAndReadsThroughTheInterfaceOnly)
{
    const auto factory = createDataValueFactory();
    const auto codec = createJsonCodec();

    auto root = factory->table();
    root->set("name", factory->string("app"));
    root->set("width", factory->integer(800));
    root->set("ratio", factory->floating(1.5));
    root->set("visible", factory->boolean(true));
    root->set("nothing", factory->null());
    auto tags = factory->array();
    tags->append(factory->string("a"));
    tags->append(factory->string("b"));
    root->set("tags", std::move(tags));

    VIGINE_ASSERT_TRUE(root->kind() == DataKind::Table);
    VIGINE_ASSERT_EQ(root->size(), 6u);
    VIGINE_EXPECT_EQ(root->keyAt(0), std::string_view("name"));
    VIGINE_EXPECT_EQ(root->keyAt(5), std::string_view("tags"));
    VIGINE_EXPECT_EQ(root->at("name").asString(), std::string("app"));
    VIGINE_EXPECT_EQ(root->at("width").asInteger(), std::int64_t{800});
    VIGINE_EXPECT_NEAR(root->at("ratio").asFloating(), 1.5, 1e-12);
    VIGINE_EXPECT_TRUE(root->at("visible").asBoolean());
    VIGINE_EXPECT_TRUE(root->at("nothing").kind() == DataKind::Null);
    VIGINE_EXPECT_TRUE(root->find("absent") == nullptr);
    VIGINE_ASSERT_EQ(root->at("tags").size(), 2u);
    VIGINE_EXPECT_EQ(root->at("tags").elementAt(1).asString(), std::string("b"));

    const std::string text = codec->serialize(*root, JsonWriteOptions{});
    const auto parsed = codec->parseValue(text);
    VIGINE_EXPECT_TRUE(parsed->equals(*root));
    VIGINE_EXPECT_EQ(codec->serialize(*parsed, JsonWriteOptions{}), text);
}

VIGINE_TEST(DataValueInterface, ChangesInPlaceAndRoundTripsThroughTheCodecInterface)
{
    const auto factory = createDataValueFactory();
    const auto codec = createJsonCodec();

    auto root = factory->table();
    root->set("list", factory->array());
    root->at("list").append(factory->integer(1));
    root->at("list").append(factory->integer(2));
    root->set("list", factory->array());
    root->find("list")->append(factory->boolean(false));
    root->set("inner", factory->table());
    root->at("inner").set("key", factory->string("v"));

    const std::string text = codec->serialize(*root, JsonWriteOptions{});
    VIGINE_EXPECT_EQ(text, std::string("{\"list\":[false],\"inner\":{\"key\":\"v\"}}"));

    const auto parsed = codec->parseValue(text);
    VIGINE_EXPECT_TRUE(parsed->equals(*root));
    const auto copy = root->clone();
    VIGINE_EXPECT_TRUE(copy->equals(*root));
    copy->at("inner").set("key", factory->string("changed"));
    VIGINE_EXPECT_FALSE(copy->equals(*root));
    VIGINE_EXPECT_EQ(root->at("inner").at("key").asString(), std::string("v"));
}

VIGINE_TEST(DataValueInterface, WrongKindAndMissingKeyThrow)
{
    const auto factory = createDataValueFactory();
    auto root = factory->table();
    root->set("number", factory->integer(1));

    VIGINE_EXPECT_THROW((void)root->at("absent"), std::out_of_range);
    VIGINE_EXPECT_THROW((void)root->at("number").asString(), std::logic_error);
    VIGINE_EXPECT_THROW((void)root->asInteger(), std::logic_error);
    VIGINE_EXPECT_THROW((void)root->at("number").size(), std::logic_error);
    VIGINE_EXPECT_THROW((void)root->at("number").find("x"), std::logic_error);
    VIGINE_EXPECT_THROW((void)root->elementAt(0), std::logic_error);
    VIGINE_EXPECT_THROW((void)root->keyAt(1), std::out_of_range);
    auto list = factory->array();
    VIGINE_EXPECT_THROW((void)list->elementAt(0), std::out_of_range);
    VIGINE_EXPECT_THROW(list->set("k", factory->null()), std::logic_error);
    VIGINE_EXPECT_THROW(root->append(factory->null()), std::logic_error);
}
} // namespace
