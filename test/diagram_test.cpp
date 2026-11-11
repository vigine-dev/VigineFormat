#include "vigine/format/diagramimporters.h"
#include "vigine/format/diagrammodel.h"

#include <algorithm>
#include <string>
#include <vigine/unittest/unittest.h>

namespace
{
using namespace vigine::format;

const DiagramNode *findNode(const DiagramModel &model, const std::string &id)
{
    const auto it = std::find_if(model.nodes().begin(), model.nodes().end(),
                                 [&id](const DiagramNode &node) { return node.id() == id; });
    return it == model.nodes().end() ? nullptr : &*it;
}

bool hasEdge(const DiagramModel &model, const std::string &from, const std::string &to)
{
    return std::any_of(model.edges().begin(), model.edges().end(),
                       [&](const DiagramEdge &edge) { return edge.from() == from && edge.to() == to; });
}

VIGINE_TEST(DiagramModel, DedupNodesAndAutoEdgeNodes)
{
    DiagramModel model;
    model.addNode("a");
    model.addNode("a", "Label A"); // fills the missing label, no duplicate
    VIGINE_ASSERT_EQ(model.nodes().size(), 1u);
    VIGINE_EXPECT_EQ(model.nodes().front().label(), "Label A");
    model.addEdge("a", "b");
    VIGINE_EXPECT_EQ(model.nodes().size(), 2u); // b auto-created
    VIGINE_EXPECT_TRUE(hasEdge(model, "a", "b"));
}

VIGINE_TEST(DotImporter, EdgesAndLabels)
{
    const auto dot = createDotImporter();
    const auto model = dot->import(R"(digraph G { a -> b; b -> c; a [label="Node A"]; })");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_EXPECT_EQ(model->nodes().size(), 3u);
    VIGINE_ASSERT_NE(findNode(*model, "a"), nullptr);
    VIGINE_EXPECT_EQ(findNode(*model, "a")->label(), "Node A");
    VIGINE_EXPECT_TRUE(hasEdge(*model, "a", "b"));
    VIGINE_EXPECT_TRUE(hasEdge(*model, "b", "c"));
    VIGINE_EXPECT_TRUE(model->edges().front().directed());
}

VIGINE_TEST(DotImporter, ChainAndUndirected)
{
    const auto dot = createDotImporter();
    const auto chain = dot->import("digraph { x -> y -> z }");
    VIGINE_ASSERT_TRUE(chain.has_value());
    VIGINE_EXPECT_EQ(chain->edges().size(), 2u);
    VIGINE_EXPECT_TRUE(hasEdge(*chain, "x", "y"));
    VIGINE_EXPECT_TRUE(hasEdge(*chain, "y", "z"));

    const auto undirected = dot->import("graph { p -- q }");
    VIGINE_ASSERT_TRUE(undirected.has_value());
    VIGINE_ASSERT_EQ(undirected->edges().size(), 1u);
    VIGINE_EXPECT_FALSE(undirected->edges().front().directed());
}

VIGINE_TEST(MermaidImporter, EdgesNodesAndLabel)
{
    const auto mermaid = createMermaidImporter();
    const auto model = mermaid->import("graph TD\n A-->B\n B-->C\n A[Start]");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_EXPECT_EQ(model->nodes().size(), 3u);
    VIGINE_ASSERT_NE(findNode(*model, "A"), nullptr);
    VIGINE_EXPECT_EQ(findNode(*model, "A")->label(), "Start");
    VIGINE_EXPECT_TRUE(hasEdge(*model, "A", "B"));
    VIGINE_EXPECT_TRUE(hasEdge(*model, "B", "C"));
}

VIGINE_TEST(MermaidImporter, InlineLabelsAndPipedEdgeLabel)
{
    const auto mermaid = createMermaidImporter();
    const auto inlineModel = mermaid->import("flowchart LR\n A[Start] --> B[End]");
    VIGINE_ASSERT_TRUE(inlineModel.has_value());
    VIGINE_EXPECT_EQ(findNode(*inlineModel, "A")->label(), "Start");
    VIGINE_EXPECT_EQ(findNode(*inlineModel, "B")->label(), "End");
    VIGINE_EXPECT_TRUE(hasEdge(*inlineModel, "A", "B"));

    const auto piped = mermaid->import("graph TD\n A -->|yes| B");
    VIGINE_ASSERT_TRUE(piped.has_value());
    VIGINE_ASSERT_EQ(piped->edges().size(), 1u);
    VIGINE_EXPECT_EQ(piped->edges().front().label(), "yes");
}

VIGINE_TEST(MermaidImporter, NodeShapes)
{
    const auto mermaid = createMermaidImporter();
    const auto model = mermaid->import("graph TD\n A((Circle))\n B{Diamond}\n C(Round)");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_EXPECT_EQ(findNode(*model, "A")->label(), "Circle");
    VIGINE_EXPECT_EQ(findNode(*model, "B")->label(), "Diamond");
    VIGINE_EXPECT_EQ(findNode(*model, "C")->label(), "Round");
}

VIGINE_TEST(GraphMlImporter, NodesEdgesAndDataLabel)
{
    const auto graphml = createGraphMlImporter();
    const auto model = graphml->import(R"(<?xml version="1.0"?>
<graphml><graph id="G" edgedefault="directed">
  <node id="n0"><data key="d0">Alpha</data></node>
  <node id="n1"/>
  <edge source="n0" target="n1"/>
</graph></graphml>)");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_EXPECT_EQ(model->nodes().size(), 2u);
    VIGINE_ASSERT_NE(findNode(*model, "n0"), nullptr);
    VIGINE_EXPECT_EQ(findNode(*model, "n0")->label(), "Alpha");
    VIGINE_EXPECT_TRUE(hasEdge(*model, "n0", "n1"));
}

VIGINE_TEST(PlantUmlImporter, DeclarationsAndRelationships)
{
    const auto uml = createPlantUmlImporter();
    const auto model = uml->import(R"(@startuml
class Animal
class Dog
abstract class Base
Animal <|-- Dog
Dog --> Base : owns
@enduml)");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_ASSERT_NE(findNode(*model, "Animal"), nullptr);
    VIGINE_ASSERT_NE(findNode(*model, "Dog"), nullptr);
    VIGINE_ASSERT_NE(findNode(*model, "Base"), nullptr);
    // `Animal <|-- Dog` puts the head on the left -> edge Dog -> Animal.
    VIGINE_EXPECT_TRUE(hasEdge(*model, "Dog", "Animal"));
    VIGINE_EXPECT_TRUE(hasEdge(*model, "Dog", "Base"));
    const auto owns = std::find_if(model->edges().begin(), model->edges().end(),
                                   [](const DiagramEdge &edge) { return edge.label() == "owns"; });
    VIGINE_EXPECT_NE(owns, model->edges().end());
}

VIGINE_TEST(PlantUmlImporter, IgnoresClassBody)
{
    const auto uml = createPlantUmlImporter();
    const auto model = uml->import(R"(@startuml
class Account {
  +balance : int
  +deposit()
}
Account --> Ledger
@enduml)");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_EXPECT_EQ(model->nodes().size(), 2u); // Account + Ledger, no members
    VIGINE_EXPECT_TRUE(hasEdge(*model, "Account", "Ledger"));
}

VIGINE_TEST(JsonTreeImporter, ObjectArrayAndScalarTree)
{
    const auto json = createJsonTreeImporter();
    const auto model = json->import(R"({"name":"svc","ports":[80,443],"tls":true})");
    VIGINE_ASSERT_TRUE(model.has_value());
    // root + name + svc-value(leaf is the name node itself) + ports + 2 elems + tls.
    VIGINE_ASSERT_NE(findNode(*model, "n0"), nullptr);
    VIGINE_EXPECT_EQ(findNode(*model, "n0")->label(), "object");
    // Every child hangs off the root.
    int rootChildren = 0;
    for (const auto &edge : model->edges())
        if (edge.from() == "n0")
            ++rootChildren;
    VIGINE_EXPECT_EQ(rootChildren, 3); // name, ports, tls
    // A primitive member carries "key: value"; an array element "[i] value".
    const bool hasNameLeaf =
        std::any_of(model->nodes().begin(), model->nodes().end(),
                    [](const DiagramNode &n) { return n.label() == "name: svc"; });
    const bool hasPortLeaf = std::any_of(model->nodes().begin(), model->nodes().end(),
                                         [](const DiagramNode &n) { return n.label() == "[0] 80"; });
    VIGINE_EXPECT_TRUE(hasNameLeaf);
    VIGINE_EXPECT_TRUE(hasPortLeaf);
}

VIGINE_TEST(EdgeListImporter, AdjacencyAndMultiParent)
{
    const auto edges = createEdgeListImporter();
    // git-style: a commit then its parents; a plain edge; a comment; a lone node.
    const auto model = edges->import("# history\nc3 c2 c1\nc2 c1\nc1\nlib utils");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_EXPECT_TRUE(hasEdge(*model, "c3", "c2"));
    VIGINE_EXPECT_TRUE(hasEdge(*model, "c3", "c1")); // merge: second parent
    VIGINE_EXPECT_TRUE(hasEdge(*model, "c2", "c1"));
    VIGINE_EXPECT_TRUE(hasEdge(*model, "lib", "utils"));
    VIGINE_ASSERT_NE(findNode(*model, "c1"), nullptr); // lone node still present
}

VIGINE_TEST(GraphQlImporter, TypesFieldsAndUnion)
{
    const auto graphql = createGraphQlImporter();
    const auto model = graphql->import(R"(
type User { id: ID! name: String posts: [Post!]! }
type Post { id: ID author: User }
union Content = Post | User
scalar DateTime
)");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_ASSERT_NE(findNode(*model, "User"), nullptr);
    VIGINE_ASSERT_NE(findNode(*model, "Post"), nullptr);
    VIGINE_EXPECT_TRUE(hasEdge(*model, "User", "Post"));    // posts: [Post!]!
    VIGINE_EXPECT_TRUE(hasEdge(*model, "Post", "User"));    // author: User
    VIGINE_EXPECT_TRUE(hasEdge(*model, "Content", "Post")); // union member
    VIGINE_EXPECT_TRUE(hasEdge(*model, "Content", "User"));
    // Scalars never become edges.
    VIGINE_EXPECT_FALSE(hasEdge(*model, "User", "String"));
    VIGINE_EXPECT_EQ(findNode(*model, "String"), nullptr);
}

VIGINE_TEST(ProtobufImporter, MessagesFieldsAndMap)
{
    const auto proto = createProtobufImporter();
    const auto model = proto->import(R"(
message Address { string city = 1; }
message User {
  string name = 1;
  Address home = 2;
  repeated User friends = 3;
  map<string, Address> work = 4;
}
)");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_ASSERT_NE(findNode(*model, "User"), nullptr);
    VIGINE_ASSERT_NE(findNode(*model, "Address"), nullptr);
    VIGINE_EXPECT_TRUE(hasEdge(*model, "User", "Address")); // home + map value
    VIGINE_EXPECT_FALSE(hasEdge(*model, "User", "string")); // scalar, not declared
    // self-reference (friends: User) is suppressed.
    VIGINE_EXPECT_FALSE(hasEdge(*model, "User", "User"));
}

VIGINE_TEST(ProtobufImporter, InlineMessageBody)
{
    const auto proto = createProtobufImporter();
    // Whole message on one line -- the field must still resolve.
    const auto model = proto->import("message A { string s = 1; }\nmessage B { A a = 1; }");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_EXPECT_TRUE(hasEdge(*model, "B", "A"));
}

VIGINE_TEST(SqlErdImporter, TablesAndForeignKeys)
{
    const auto sql = createSqlErdImporter();
    const auto model = sql->import(R"(
CREATE TABLE users (id INT PRIMARY KEY, name TEXT);
CREATE TABLE orders (
  id INT PRIMARY KEY,
  user_id INT REFERENCES users(id),
  FOREIGN KEY (coupon_id) REFERENCES coupons(id)
);
)");
    VIGINE_ASSERT_TRUE(model.has_value());
    VIGINE_ASSERT_NE(findNode(*model, "orders"), nullptr);
    VIGINE_ASSERT_NE(findNode(*model, "users"), nullptr);
    VIGINE_EXPECT_TRUE(hasEdge(*model, "orders", "users"));   // inline column REFERENCES
    VIGINE_EXPECT_TRUE(hasEdge(*model, "orders", "coupons")); // table-level FOREIGN KEY
}

VIGINE_TEST(FlameGraphImporter, FoldedStacksBuildPrefixTree)
{
    const auto flame = createFlameGraphImporter();
    const auto model = flame->import("main;parse;lex 120\nmain;parse;ast 80\nmain;render 50");
    VIGINE_ASSERT_TRUE(model.has_value());
    // Shared prefix main->parse is one path; lex and ast diverge under parse.
    VIGINE_EXPECT_TRUE(hasEdge(*model, "main", "main;parse"));
    VIGINE_EXPECT_TRUE(hasEdge(*model, "main;parse", "main;parse;lex"));
    VIGINE_EXPECT_TRUE(hasEdge(*model, "main;parse", "main;parse;ast"));
    VIGINE_EXPECT_TRUE(hasEdge(*model, "main", "main;render"));
    // The node is keyed by path but labelled by the bare frame name.
    const DiagramNode *lex = findNode(*model, "main;parse;lex");
    VIGINE_ASSERT_NE(lex, nullptr);
    VIGINE_EXPECT_EQ(lex->label(), "lex");
}

VIGINE_TEST(DiagramImporters, EmptyInputYieldsNoModel)
{
    const auto dot = createDotImporter();
    const auto mermaid = createMermaidImporter();
    const auto graphml = createGraphMlImporter();
    const auto uml = createPlantUmlImporter();
    const auto json = createJsonTreeImporter();
    const auto edges = createEdgeListImporter();
    const auto graphql = createGraphQlImporter();
    const auto proto = createProtobufImporter();
    const auto sql = createSqlErdImporter();
    const auto flame = createFlameGraphImporter();
    VIGINE_EXPECT_FALSE(dot->import("digraph {}").has_value());
    VIGINE_EXPECT_FALSE(mermaid->import("graph TD").has_value());
    VIGINE_EXPECT_FALSE(graphml->import("<graphml></graphml>").has_value());
    VIGINE_EXPECT_FALSE(uml->import("@startuml\n@enduml").has_value());
    VIGINE_EXPECT_FALSE(json->import("{ not valid json").has_value());
    VIGINE_EXPECT_FALSE(edges->import("# only a comment\n   ").has_value());
    VIGINE_EXPECT_FALSE(graphql->import("# nothing here").has_value());
    VIGINE_EXPECT_FALSE(proto->import("syntax = \"proto3\";").has_value());
    VIGINE_EXPECT_FALSE(sql->import("SELECT 1;").has_value());
    VIGINE_EXPECT_FALSE(flame->import("# just a comment").has_value());
}

} // namespace
