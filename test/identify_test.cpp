#include <gtest/gtest.h>

#include "vigine/format/identify.h"

namespace
{
using vigine::format::identify;
using vigine::format::identifyContent;
using vigine::format::identifyExtension;
using vigine::format::KnownFormat;

TEST(Identify, ContentProbesRecogniseTheCoreFormats)
{
    EXPECT_EQ(identifyContent("%PDF-1.7 ..."), KnownFormat::Pdf);
    EXPECT_EQ(identifyContent("  {\"a\":1}"), KnownFormat::Json);
    EXPECT_EQ(identifyContent("@startuml\nclass A\n@enduml"), KnownFormat::PlantUml);
    EXPECT_EQ(identifyContent("<graphml><graph/></graphml>"), KnownFormat::GraphMl);
    EXPECT_EQ(identifyContent("<?xml version=\"1.0\"?><svg/>"), KnownFormat::Xml);
    EXPECT_EQ(identifyContent("--- \nkey: value"), KnownFormat::Yaml);
    EXPECT_EQ(identifyContent("CREATE TABLE users (id INT);"), KnownFormat::Sql);
    EXPECT_EQ(identifyContent("syntax = \"proto3\";\nmessage A {}"), KnownFormat::Protobuf);
    EXPECT_EQ(identifyContent("type Query { me: User }\nscalar Date"), KnownFormat::GraphQl);
    EXPECT_EQ(identifyContent("digraph G { a -> b }"), KnownFormat::Dot);
    EXPECT_EQ(identifyContent("graph TD\n A-->B"), KnownFormat::Mermaid);
}

TEST(Identify, ExtensionHintMapsKnownSuffixes)
{
    EXPECT_EQ(identifyExtension("deps.gv"), KnownFormat::Dot);
    EXPECT_EQ(identifyExtension("flow.mmd"), KnownFormat::Mermaid);
    EXPECT_EQ(identifyExtension("model.puml"), KnownFormat::PlantUml);
    EXPECT_EQ(identifyExtension("notebook.ipynb"), KnownFormat::Json);
    EXPECT_EQ(identifyExtension("api.graphql"), KnownFormat::GraphQl);
    EXPECT_EQ(identifyExtension("schema.PROTO"), KnownFormat::Protobuf);
    EXPECT_EQ(identifyExtension("stacks.folded"), KnownFormat::FlameGraph);
    EXPECT_EQ(identifyExtension("config.toml"), KnownFormat::Toml);
    EXPECT_EQ(identifyExtension("noextension"), KnownFormat::Unknown);
}

TEST(Identify, ContentWinsOverExtension)
{
    // A lying extension does not fool the probes: bytes first.
    EXPECT_EQ(identify("data.txt", "{\"actually\":\"json\"}"), KnownFormat::Json);
    EXPECT_EQ(identify("diagram.json", "@startuml\n@enduml"), KnownFormat::PlantUml);
}

TEST(Identify, ExtensionResolvesInconclusiveContent)
{
    // Plain numbers tell the probes nothing; the extension decides.
    EXPECT_EQ(identify("samples.folded", "0.5 0.1 0.9"), KnownFormat::FlameGraph);
    EXPECT_EQ(identify("settings.toml", "answer = 42"), KnownFormat::Toml);
    EXPECT_EQ(identify("mystery.bin", "\x00\x01\x02"), KnownFormat::Unknown);
}
} // namespace
