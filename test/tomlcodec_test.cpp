#include "vigine/format/tomlcodec.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vigine/unittest/unittest.h>

namespace
{
using namespace vigine::format;

VIGINE_TEST(TomlCodec, ParsesScalarsAndNesting)
{
    const auto codec = createTomlCodec();
    const DataValue root = codec->parse("title = \"app\"\n"
                                        "[window]\n"
                                        "width = 800\n"
                                        "ratio = 1.5\n"
                                        "visible = true\n"
                                        "tags = [\"a\", \"b\"]\n");

    VIGINE_ASSERT_TRUE(root.kind() == DataKind::Table);
    VIGINE_EXPECT_EQ(root.at("title").asString(), std::string("app"));

    const DataValue &window = root.at("window");
    VIGINE_ASSERT_TRUE(window.kind() == DataKind::Table);
    VIGINE_EXPECT_EQ(window.at("width").asInteger(), std::int64_t{800});
    VIGINE_EXPECT_NEAR(window.at("ratio").asFloating(), 1.5, 1e-12);
    VIGINE_EXPECT_TRUE(window.at("visible").asBoolean());

    const DataValue &tags = window.at("tags");
    VIGINE_ASSERT_TRUE(tags.kind() == DataKind::Array);
    VIGINE_ASSERT_EQ(tags.elements().size(), 2u);
    VIGINE_EXPECT_EQ(tags.elements()[0].asString(), std::string("a"));
    VIGINE_EXPECT_EQ(tags.elements()[1].asString(), std::string("b"));
}

VIGINE_TEST(TomlCodec, DatesAndTimesBecomeIsoStrings)
{
    const auto codec = createTomlCodec();
    const DataValue root = codec->parse("day = 1979-05-27\n"
                                        "clock = 07:32:00\n"
                                        "moment = 1979-05-27T07:32:00Z\n");

    VIGINE_ASSERT_TRUE(root.at("day").kind() == DataKind::String);
    VIGINE_EXPECT_EQ(root.at("day").asString(), std::string("1979-05-27"));
    VIGINE_EXPECT_EQ(root.at("clock").asString(), std::string("07:32:00"));
    VIGINE_EXPECT_EQ(root.at("moment").asString(), std::string("1979-05-27T07:32:00Z"));
}

VIGINE_TEST(TomlCodec, ArrayOfTablesAndMixedArray)
{
    const auto codec = createTomlCodec();
    const DataValue root = codec->parse("mixed = [1, \"two\", 3.5]\n"
                                        "[[step]]\n"
                                        "name = \"build\"\n"
                                        "[[step]]\n"
                                        "name = \"test\"\n");

    const DataValue &mixed = root.at("mixed");
    VIGINE_ASSERT_EQ(mixed.elements().size(), 3u);
    VIGINE_EXPECT_TRUE(mixed.elements()[0].kind() == DataKind::Integer);
    VIGINE_EXPECT_TRUE(mixed.elements()[1].kind() == DataKind::String);
    VIGINE_EXPECT_TRUE(mixed.elements()[2].kind() == DataKind::Float);

    const DataValue &steps = root.at("step");
    VIGINE_ASSERT_EQ(steps.elements().size(), 2u);
    VIGINE_EXPECT_EQ(steps.elements()[1].at("name").asString(), std::string("test"));
}

VIGINE_TEST(TomlCodec, RoundTripsToEqualTree)
{
    const auto codec = createTomlCodec();
    const DataValue original = codec->parse("a = 1\n"
                                            "b = \"x\"\n"
                                            "pi = 3.25\n"
                                            "day = 1979-05-27\n"
                                            "[section]\n"
                                            "flag = true\n"
                                            "list = [1, 2, 3]\n"
                                            "[[section.item]]\n"
                                            "id = 7\n");

    const std::string text = codec->serialize(original);
    const DataValue reparsed = codec->parse(text);
    VIGINE_EXPECT_TRUE(original == reparsed);
}

VIGINE_TEST(TomlCodec, SerializesATreeBuiltByHand)
{
    const auto codec = createTomlCodec();
    DataValue inner = DataValue::table();
    inner.set("on", DataValue::boolean(true));
    DataValue list = DataValue::array();
    list.append(DataValue::integer(1));
    list.append(DataValue::integer(2));
    DataValue root = DataValue::table();
    root.set("name", DataValue::string("demo"));
    root.set("count", DataValue::integer(-3));
    root.set("ratio", DataValue::floating(0.5));
    root.set("inner", inner);
    root.set("list", list);

    VIGINE_EXPECT_TRUE(codec->parse(codec->serialize(root)) == root);
}

VIGINE_TEST(TomlCodec, ThrowsOnMalformedInput)
{
    const auto codec = createTomlCodec();
    bool thrown = false;
    try
    {
        (void)codec->parse("[unterminated\n");
    } catch (const TomlParseError &error)
    {
        thrown = true;
        VIGINE_EXPECT_EQ(error.line(), 1u);
        VIGINE_EXPECT_TRUE(std::string(error.what()).find("TOML parse error") == 0);
    }
    VIGINE_EXPECT_TRUE(thrown);
}

VIGINE_TEST(TomlCodec, SerializeRejectsNonTableRoot)
{
    const auto codec = createTomlCodec();
    bool thrown = false;
    try
    {
        (void)codec->serialize(DataValue::integer(42));
    } catch (const std::invalid_argument &)
    {
        thrown = true;
    }
    VIGINE_EXPECT_TRUE(thrown);
}

VIGINE_TEST(TomlCodec, SerializeRefusesANullInATable)
{
    const auto codec = createTomlCodec();
    DataValue inner = DataValue::table();
    inner.set("gone", DataValue::null());
    DataValue root = DataValue::table();
    root.set("inner", inner);
    bool thrown = false;
    try
    {
        (void)codec->serialize(root);
    } catch (const std::invalid_argument &error)
    {
        thrown = true;
        VIGINE_EXPECT_TRUE(std::string(error.what()).find("\"gone\"") != std::string::npos);
        VIGINE_EXPECT_TRUE(std::string(error.what()).find("null") != std::string::npos);
    }
    VIGINE_EXPECT_TRUE(thrown);
}

VIGINE_TEST(TomlCodec, SerializeRefusesANullInAnArray)
{
    const auto codec = createTomlCodec();
    DataValue list = DataValue::array();
    list.append(DataValue::integer(1));
    list.append(DataValue::null());
    DataValue root = DataValue::table();
    root.set("list", list);
    bool thrown = false;
    try
    {
        (void)codec->serialize(root);
    } catch (const std::invalid_argument &error)
    {
        thrown = true;
        VIGINE_EXPECT_TRUE(std::string(error.what()).find("null") != std::string::npos);
    }
    VIGINE_EXPECT_TRUE(thrown);
}
} // namespace
