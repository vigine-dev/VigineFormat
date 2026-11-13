#include "vigine/format/jsoncodec.h"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vigine/unittest/unittest.h>

namespace
{
using namespace vigine::format;

VIGINE_TEST(JsonCodec, ParsesEveryKind)
{
    const auto codec = createJsonCodec();
    const DataValue root = codec->parse("{\"name\":\"app\",\"width\":800,\"ratio\":1.5,"
                                        "\"visible\":true,\"nothing\":null,\"tags\":[\"a\",\"b\"]}");

    VIGINE_ASSERT_TRUE(root.kind() == DataKind::Table);
    VIGINE_EXPECT_EQ(root.at("name").asString(), std::string("app"));
    VIGINE_ASSERT_TRUE(root.at("width").kind() == DataKind::Integer);
    VIGINE_EXPECT_EQ(root.at("width").asInteger(), std::int64_t{800});
    VIGINE_ASSERT_TRUE(root.at("ratio").kind() == DataKind::Float);
    VIGINE_EXPECT_NEAR(root.at("ratio").asFloating(), 1.5, 1e-12);
    VIGINE_EXPECT_TRUE(root.at("visible").asBoolean());
    VIGINE_EXPECT_TRUE(root.at("nothing").kind() == DataKind::Null);

    const DataValue &tags = root.at("tags");
    VIGINE_ASSERT_TRUE(tags.kind() == DataKind::Array);
    VIGINE_ASSERT_EQ(tags.elements().size(), 2u);
    VIGINE_EXPECT_EQ(tags.elements()[1].asString(), std::string("b"));
}

VIGINE_TEST(JsonCodec, RootMayBeAnyKind)
{
    const auto codec = createJsonCodec();
    VIGINE_EXPECT_TRUE(codec->parse("null").kind() == DataKind::Null);
    VIGINE_EXPECT_TRUE(codec->parse("[1,2]").kind() == DataKind::Array);
    VIGINE_EXPECT_EQ(codec->parse("\"text\"").asString(), std::string("text"));
    VIGINE_EXPECT_EQ(codec->serialize(DataValue::null()), std::string("null"));
    VIGINE_EXPECT_EQ(codec->serialize(DataValue::integer(7)), std::string("7"));
}

VIGINE_TEST(JsonCodec, EveryKindRoundTrips)
{
    const auto codec = createJsonCodec();
    DataValue inner = DataValue::table();
    inner.set("flag", DataValue::boolean(false));
    inner.set("none", DataValue::null());

    DataValue list = DataValue::array();
    list.append(DataValue::integer(-3));
    list.append(DataValue::floating(2.0));
    list.append(DataValue::string("quote \" and \n newline"));
    list.append(DataValue::null());
    list.append(inner);

    DataValue root = DataValue::table();
    root.set("text", DataValue::string("caf\xC3\xA9"));
    root.set("big", DataValue::integer(std::numeric_limits<std::int64_t>::max()));
    root.set("small", DataValue::integer(std::numeric_limits<std::int64_t>::min()));
    root.set("ratio", DataValue::floating(0.1));
    root.set("list", list);
    root.set("nested", inner);

    const DataValue again = codec->parse(codec->serialize(root));
    VIGINE_EXPECT_TRUE(again == root);
    VIGINE_EXPECT_TRUE(again.at("list").elements()[1].kind() == DataKind::Float);
}

VIGINE_TEST(JsonCodec, DeeplyNestedTablesAndArraysRoundTrip)
{
    const auto codec = createJsonCodec();
    DataValue value = DataValue::string("leaf");
    for (int depth = 0; depth < 20; ++depth)
    {
        DataValue wrapper = (depth % 2 == 0) ? DataValue::array() : DataValue::table();
        if (depth % 2 == 0)
        {
            wrapper.append(value);
        } else
        {
            wrapper.set("child", value);
        }
        value = wrapper;
    }
    VIGINE_EXPECT_TRUE(codec->parse(codec->serialize(value)) == value);
}

VIGINE_TEST(JsonCodec, KeysKeepTheirOrder)
{
    const auto codec = createJsonCodec();
    DataValue root = DataValue::table();
    root.set("zeta", DataValue::integer(1));
    root.set("alpha", DataValue::integer(2));
    VIGINE_EXPECT_EQ(codec->serialize(root), std::string("{\"zeta\":1,\"alpha\":2}"));
    VIGINE_EXPECT_EQ(codec->parse("{\"zeta\":1,\"alpha\":2}").keys().front(), std::string("zeta"));
}

VIGINE_TEST(JsonCodec, MalformedTextThrowsParseErrorWithPosition)
{
    const auto codec = createJsonCodec();
    bool thrown = false;
    try
    {
        (void)codec->parse("{\n  \"a\": 1,\n  \"b\": ,\n}");
    } catch (const JsonParseError &error)
    {
        thrown = true;
        VIGINE_EXPECT_EQ(error.line(), 3u);
        VIGINE_EXPECT_TRUE(error.column() > 0u);
    }
    VIGINE_EXPECT_TRUE(thrown);
}

VIGINE_TEST(JsonCodec, NonFiniteFloatCannotBeWritten)
{
    const auto codec = createJsonCodec();
    bool thrown = false;
    try
    {
        (void)codec->serialize(DataValue::floating(std::numeric_limits<double>::infinity()));
    } catch (const std::invalid_argument &)
    {
        thrown = true;
    }
    VIGINE_EXPECT_TRUE(thrown);
}

VIGINE_TEST(JsonCodec, ReplacingMethodGivesTheSameBytesForValidInput)
{
    const auto codec = createJsonCodec();
    DataValue root = DataValue::table();
    root.set("name", DataValue::string("caf\xC3\xA9 \"quoted\""));
    root.set("count", DataValue::integer(7));
    root.set("tags", DataValue::array());
    root.at("tags").append(DataValue::boolean(true));
    VIGINE_EXPECT_EQ(codec->serializeReplacingInvalidUtf8(root), codec->serialize(root));
}

VIGINE_TEST(JsonCodec, InvalidUtf8IsReplacedByTheReplacingMethodAndRefusedByTheStrictOne)
{
    const auto codec = createJsonCodec();
    const DataValue loneByte = DataValue::string(std::string("a\xFF" "b"));
    const DataValue cutSequence = DataValue::string(std::string("a\xE2\x82"));

    VIGINE_EXPECT_EQ(codec->serializeReplacingInvalidUtf8(loneByte), std::string("\"a\xEF\xBF\xBD" "b\""));
    VIGINE_EXPECT_EQ(codec->serializeReplacingInvalidUtf8(cutSequence), std::string("\"a\xEF\xBF\xBD\""));
    VIGINE_EXPECT_THROW((void)codec->serialize(loneByte), std::exception);
    VIGINE_EXPECT_THROW((void)codec->serialize(cutSequence), std::exception);
}
} // namespace
