#include "vigine/format/identify.h"
#include <vigine/unittest/unittest.h>


namespace
{
using vigine::format::identify;
using vigine::format::identifyContent;
using vigine::format::identifyExtension;
using vigine::format::KnownFormat;

VIGINE_TEST(Identify, ContentProbesRecogniseTheCoreFormats)
{
    VIGINE_EXPECT_EQ(identifyContent("%PDF-1.7 ..."), KnownFormat::Pdf);
    VIGINE_EXPECT_EQ(identifyContent("  {\"a\":1}"), KnownFormat::Json);
    VIGINE_EXPECT_EQ(identifyContent("@startuml\nclass A\n@enduml"), KnownFormat::PlantUml);
    VIGINE_EXPECT_EQ(identifyContent("<graphml><graph/></graphml>"), KnownFormat::GraphMl);
    VIGINE_EXPECT_EQ(identifyContent("<?xml version=\"1.0\"?><svg/>"), KnownFormat::Xml);
    VIGINE_EXPECT_EQ(identifyContent("--- \nkey: value"), KnownFormat::Yaml);
    VIGINE_EXPECT_EQ(identifyContent("CREATE TABLE users (id INT);"), KnownFormat::Sql);
    VIGINE_EXPECT_EQ(identifyContent("syntax = \"proto3\";\nmessage A {}"), KnownFormat::Protobuf);
    VIGINE_EXPECT_EQ(identifyContent("type Query { me: User }\nscalar Date"), KnownFormat::GraphQl);
    VIGINE_EXPECT_EQ(identifyContent("digraph G { a -> b }"), KnownFormat::Dot);
    VIGINE_EXPECT_EQ(identifyContent("graph TD\n A-->B"), KnownFormat::Mermaid);
}

VIGINE_TEST(Identify, ExtensionHintMapsKnownSuffixes)
{
    VIGINE_EXPECT_EQ(identifyExtension("deps.gv"), KnownFormat::Dot);
    VIGINE_EXPECT_EQ(identifyExtension("flow.mmd"), KnownFormat::Mermaid);
    VIGINE_EXPECT_EQ(identifyExtension("model.puml"), KnownFormat::PlantUml);
    VIGINE_EXPECT_EQ(identifyExtension("notebook.ipynb"), KnownFormat::Json);
    VIGINE_EXPECT_EQ(identifyExtension("api.graphql"), KnownFormat::GraphQl);
    VIGINE_EXPECT_EQ(identifyExtension("schema.PROTO"), KnownFormat::Protobuf);
    VIGINE_EXPECT_EQ(identifyExtension("stacks.folded"), KnownFormat::FlameGraph);
    VIGINE_EXPECT_EQ(identifyExtension("config.toml"), KnownFormat::Toml);
    VIGINE_EXPECT_EQ(identifyExtension("noextension"), KnownFormat::Unknown);
}

VIGINE_TEST(Identify, ContentWinsOverExtension)
{
    // A lying extension does not fool the probes: bytes first.
    VIGINE_EXPECT_EQ(identify("data.txt", "{\"actually\":\"json\"}"), KnownFormat::Json);
    VIGINE_EXPECT_EQ(identify("diagram.json", "@startuml\n@enduml"), KnownFormat::PlantUml);
}

VIGINE_TEST(Identify, ExtensionResolvesInconclusiveContent)
{
    // Plain numbers tell the probes nothing; the extension decides.
    VIGINE_EXPECT_EQ(identify("samples.folded", "0.5 0.1 0.9"), KnownFormat::FlameGraph);
    VIGINE_EXPECT_EQ(identify("settings.toml", "answer = 42"), KnownFormat::Toml);
    VIGINE_EXPECT_EQ(identify("mystery.bin", "\x00\x01\x02"), KnownFormat::Unknown);
}
} // namespace
