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
    const TomlValue root = codec->parse("title = \"app\"\n"
                                        "[window]\n"
                                        "width = 800\n"
                                        "ratio = 1.5\n"
                                        "visible = true\n"
                                        "tags = [\"a\", \"b\"]\n");

    VIGINE_ASSERT_TRUE(root.kind() == TomlKind::Table);
    VIGINE_EXPECT_EQ(root.at("title").asString(), std::string("app"));

    const TomlValue &window = root.at("window");
    VIGINE_ASSERT_TRUE(window.kind() == TomlKind::Table);
    VIGINE_EXPECT_EQ(window.at("width").asInteger(), std::int64_t{800});
    VIGINE_EXPECT_NEAR(window.at("ratio").asFloating(), 1.5, 1e-12);
    VIGINE_EXPECT_TRUE(window.at("visible").asBoolean());

    const TomlValue &tags = window.at("tags");
    VIGINE_ASSERT_TRUE(tags.kind() == TomlKind::Array);
    VIGINE_ASSERT_EQ(tags.elements().size(), 2u);
    VIGINE_EXPECT_EQ(tags.elements()[0].asString(), std::string("a"));
    VIGINE_EXPECT_EQ(tags.elements()[1].asString(), std::string("b"));
}

VIGINE_TEST(TomlCodec, DatesAndTimesBecomeIsoStrings)
{
    const auto codec = createTomlCodec();
    const TomlValue root = codec->parse("day = 1979-05-27\n"
                                        "clock = 07:32:00\n"
                                        "moment = 1979-05-27T07:32:00Z\n");

    VIGINE_ASSERT_TRUE(root.at("day").kind() == TomlKind::String);
    VIGINE_EXPECT_EQ(root.at("day").asString(), std::string("1979-05-27"));
    VIGINE_EXPECT_EQ(root.at("clock").asString(), std::string("07:32:00"));
    VIGINE_EXPECT_EQ(root.at("moment").asString(), std::string("1979-05-27T07:32:00Z"));
}

VIGINE_TEST(TomlCodec, ArrayOfTablesAndMixedArray)
{
    const auto codec = createTomlCodec();
    const TomlValue root = codec->parse("mixed = [1, \"two\", 3.5]\n"
                                        "[[step]]\n"
                                        "name = \"build\"\n"
                                        "[[step]]\n"
                                        "name = \"test\"\n");

    const TomlValue &mixed = root.at("mixed");
    VIGINE_ASSERT_EQ(mixed.elements().size(), 3u);
    VIGINE_EXPECT_TRUE(mixed.elements()[0].kind() == TomlKind::Integer);
    VIGINE_EXPECT_TRUE(mixed.elements()[1].kind() == TomlKind::String);
    VIGINE_EXPECT_TRUE(mixed.elements()[2].kind() == TomlKind::Floating);

    const TomlValue &steps = root.at("step");
    VIGINE_ASSERT_EQ(steps.elements().size(), 2u);
    VIGINE_EXPECT_EQ(steps.elements()[1].at("name").asString(), std::string("test"));
}

VIGINE_TEST(TomlCodec, RoundTripsToEqualTree)
{
    const auto codec = createTomlCodec();
    const TomlValue original = codec->parse("a = 1\n"
                                            "b = \"x\"\n"
                                            "pi = 3.25\n"
                                            "day = 1979-05-27\n"
                                            "[section]\n"
                                            "flag = true\n"
                                            "list = [1, 2, 3]\n"
                                            "[[section.item]]\n"
                                            "id = 7\n");

    const std::string text = codec->serialize(original);
    const TomlValue reparsed = codec->parse(text);
    VIGINE_EXPECT_TRUE(original == reparsed);
}

VIGINE_TEST(TomlCodec, SerializesATreeBuiltByHand)
{
    const auto codec = createTomlCodec();
    TomlValue inner = TomlValue::table();
    inner.set("on", TomlValue::boolean(true));
    TomlValue list = TomlValue::array();
    list.append(TomlValue::integer(1));
    list.append(TomlValue::integer(2));
    TomlValue root = TomlValue::table();
    root.set("name", TomlValue::string("demo"));
    root.set("count", TomlValue::integer(-3));
    root.set("ratio", TomlValue::floating(0.5));
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
        (void)codec->serialize(TomlValue::integer(42));
    } catch (const std::invalid_argument &)
    {
        thrown = true;
    }
    VIGINE_EXPECT_TRUE(thrown);
}

VIGINE_TEST(TomlValue, AskingForTheWrongKindThrows)
{
    bool thrown = false;
    try
    {
        (void)TomlValue::string("text").asInteger();
    } catch (const std::logic_error &)
    {
        thrown = true;
    }
    VIGINE_EXPECT_TRUE(thrown);

    thrown = false;
    try
    {
        (void)TomlValue::table().at("missing");
    } catch (const std::out_of_range &)
    {
        thrown = true;
    }
    VIGINE_EXPECT_TRUE(thrown);
}

VIGINE_TEST(TomlValue, SettingAKeyAgainReplacesItAndTablesIgnoreKeyOrder)
{
    TomlValue first = TomlValue::table();
    first.set("a", TomlValue::integer(1));
    first.set("b", TomlValue::integer(2));
    first.set("a", TomlValue::integer(9));
    VIGINE_EXPECT_EQ(first.keys().size(), 2u);
    VIGINE_EXPECT_EQ(first.at("a").asInteger(), std::int64_t{9});

    TomlValue second = TomlValue::table();
    second.set("b", TomlValue::integer(2));
    second.set("a", TomlValue::integer(9));
    VIGINE_EXPECT_TRUE(first == second);
}
} // namespace
