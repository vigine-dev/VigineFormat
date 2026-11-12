#include "vigine/format/datavalue.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vigine/unittest/unittest.h>

namespace
{
using namespace vigine::format;

VIGINE_TEST(DataValue, EveryFactoryMakesItsKind)
{
    VIGINE_EXPECT_TRUE(DataValue::table().kind() == DataKind::Table);
    VIGINE_EXPECT_TRUE(DataValue::array().kind() == DataKind::Array);
    VIGINE_EXPECT_TRUE(DataValue::string("x").kind() == DataKind::String);
    VIGINE_EXPECT_TRUE(DataValue::integer(1).kind() == DataKind::Integer);
    VIGINE_EXPECT_TRUE(DataValue::floating(1.0).kind() == DataKind::Float);
    VIGINE_EXPECT_TRUE(DataValue::boolean(false).kind() == DataKind::Boolean);
    VIGINE_EXPECT_TRUE(DataValue::null().kind() == DataKind::Null);
}

VIGINE_TEST(DataValue, ScalarsHoldTheirValue)
{
    VIGINE_EXPECT_EQ(DataValue::string("text").asString(), std::string("text"));
    VIGINE_EXPECT_EQ(DataValue::integer(-7).asInteger(), std::int64_t{-7});
    VIGINE_EXPECT_NEAR(DataValue::floating(2.5).asFloating(), 2.5, 1e-12);
    VIGINE_EXPECT_TRUE(DataValue::boolean(true).asBoolean());
    VIGINE_EXPECT_FALSE(DataValue::boolean(false).asBoolean());
}

VIGINE_TEST(DataValue, TableKeepsInsertionOrderAndFindsKeys)
{
    DataValue table = DataValue::table();
    table.set("second", DataValue::integer(2));
    table.set("first", DataValue::null());
    VIGINE_ASSERT_EQ(table.keys().size(), 2u);
    VIGINE_EXPECT_EQ(table.keys()[0], std::string("second"));
    VIGINE_EXPECT_EQ(table.keys()[1], std::string("first"));
    VIGINE_EXPECT_TRUE(table.find("absent") == nullptr);
    VIGINE_ASSERT_TRUE(table.find("first") != nullptr);
    VIGINE_EXPECT_TRUE(table.at("first").kind() == DataKind::Null);
}

VIGINE_TEST(DataValue, ArrayKeepsElementsInOrder)
{
    DataValue list = DataValue::array();
    list.append(DataValue::integer(1));
    list.append(DataValue::null());
    list.append(DataValue::string("end"));
    VIGINE_ASSERT_EQ(list.elements().size(), 3u);
    VIGINE_EXPECT_TRUE(list.elements()[1].kind() == DataKind::Null);
    VIGINE_EXPECT_EQ(list.elements()[2].asString(), std::string("end"));
}

VIGINE_TEST(DataValue, NullHoldsNothingAndAsksForNothing)
{
    bool thrown = false;
    try
    {
        (void)DataValue::null().asString();
    } catch (const std::logic_error &error)
    {
        thrown = true;
        VIGINE_EXPECT_TRUE(std::string(error.what()).find("null") != std::string::npos);
    }
    VIGINE_EXPECT_TRUE(thrown);
}

VIGINE_TEST(DataValue, EqualityComparesKindAndValue)
{
    VIGINE_EXPECT_TRUE(DataValue::null() == DataValue::null());
    VIGINE_EXPECT_FALSE(DataValue::null() == DataValue::boolean(false));
    VIGINE_EXPECT_FALSE(DataValue::integer(1) == DataValue::floating(1.0));
    VIGINE_EXPECT_TRUE(DataValue::string("a") == DataValue::string("a"));
    VIGINE_EXPECT_FALSE(DataValue::string("a") == DataValue::string("b"));
    VIGINE_EXPECT_TRUE(DataValue::integer(4) == DataValue::integer(4));
    VIGINE_EXPECT_FALSE(DataValue::integer(4) == DataValue::integer(5));
    VIGINE_EXPECT_TRUE(DataValue::floating(0.5) == DataValue::floating(0.5));
    VIGINE_EXPECT_FALSE(DataValue::boolean(true) == DataValue::boolean(false));

    DataValue left = DataValue::array();
    left.append(DataValue::null());
    DataValue right = DataValue::array();
    right.append(DataValue::null());
    VIGINE_EXPECT_TRUE(left == right);
    right.append(DataValue::integer(1));
    VIGINE_EXPECT_FALSE(left == right);
}

VIGINE_TEST(DataValue, AskingForTheWrongKindThrows)
{
    bool thrown = false;
    try
    {
        (void)DataValue::string("text").asInteger();
    } catch (const std::logic_error &)
    {
        thrown = true;
    }
    VIGINE_EXPECT_TRUE(thrown);

    thrown = false;
    try
    {
        (void)DataValue::table().at("missing");
    } catch (const std::out_of_range &)
    {
        thrown = true;
    }
    VIGINE_EXPECT_TRUE(thrown);
}

VIGINE_TEST(DataValue, SettingAKeyAgainReplacesItAndTablesIgnoreKeyOrder)
{
    DataValue first = DataValue::table();
    first.set("a", DataValue::integer(1));
    first.set("b", DataValue::integer(2));
    first.set("a", DataValue::integer(9));
    VIGINE_EXPECT_EQ(first.keys().size(), 2u);
    VIGINE_EXPECT_EQ(first.at("a").asInteger(), std::int64_t{9});

    DataValue second = DataValue::table();
    second.set("b", DataValue::integer(2));
    second.set("a", DataValue::integer(9));
    VIGINE_EXPECT_TRUE(first == second);
}
} // namespace
