#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include "vigine/format/diagramimporters.h"
#include "vigine/format/diagrammodel.h"

namespace
{
using namespace vigine::format;

const DiagramNode *findNode(const DiagramModel &model, const std::string &id)
{
    const auto it = std::find_if(model.nodes.begin(), model.nodes.end(),
                                 [&id](const DiagramNode &node) { return node.id == id; });
    return it == model.nodes.end() ? nullptr : &*it;
}

bool hasEdge(const DiagramModel &model, const std::string &from, const std::string &to)
{
    return std::any_of(model.edges.begin(), model.edges.end(),
                       [&](const DiagramEdge &edge) { return edge.from == from && edge.to == to; });
}

TEST(DiagramModel, DedupNodesAndAutoEdgeNodes)
{
    DiagramModel model;
    model.addNode("a");
    model.addNode("a", "Label A"); // fills the missing label, no duplicate
    ASSERT_EQ(model.nodes.size(), 1u);
    EXPECT_EQ(model.nodes.front().label, "Label A");
    model.addEdge("a", "b");
    EXPECT_EQ(model.nodes.size(), 2u); // b auto-created
    EXPECT_TRUE(hasEdge(model, "a", "b"));
}

TEST(DotImporter, EdgesAndLabels)
{
    DotImporter dot;
    const auto model = dot.import(R"(digraph G { a -> b; b -> c; a [label="Node A"]; })");
    ASSERT_TRUE(model.has_value());
    EXPECT_EQ(model->nodes.size(), 3u);
    ASSERT_NE(findNode(*model, "a"), nullptr);
    EXPECT_EQ(findNode(*model, "a")->label, "Node A");
    EXPECT_TRUE(hasEdge(*model, "a", "b"));
    EXPECT_TRUE(hasEdge(*model, "b", "c"));
    EXPECT_TRUE(model->edges.front().directed);
}

TEST(DotImporter, ChainAndUndirected)
{
    DotImporter dot;
    const auto chain = dot.import("digraph { x -> y -> z }");
    ASSERT_TRUE(chain.has_value());
    EXPECT_EQ(chain->edges.size(), 2u);
    EXPECT_TRUE(hasEdge(*chain, "x", "y"));
    EXPECT_TRUE(hasEdge(*chain, "y", "z"));

    const auto undirected = dot.import("graph { p -- q }");
    ASSERT_TRUE(undirected.has_value());
    ASSERT_EQ(undirected->edges.size(), 1u);
    EXPECT_FALSE(undirected->edges.front().directed);
}

TEST(MermaidImporter, EdgesNodesAndLabel)
{
    MermaidImporter mermaid;
    const auto model = mermaid.import("graph TD\n A-->B\n B-->C\n A[Start]");
    ASSERT_TRUE(model.has_value());
    EXPECT_EQ(model->nodes.size(), 3u);
    ASSERT_NE(findNode(*model, "A"), nullptr);
    EXPECT_EQ(findNode(*model, "A")->label, "Start");
    EXPECT_TRUE(hasEdge(*model, "A", "B"));
    EXPECT_TRUE(hasEdge(*model, "B", "C"));
}

TEST(MermaidImporter, InlineLabelsAndPipedEdgeLabel)
{
    MermaidImporter mermaid;
    const auto inlineModel = mermaid.import("flowchart LR\n A[Start] --> B[End]");
    ASSERT_TRUE(inlineModel.has_value());
    EXPECT_EQ(findNode(*inlineModel, "A")->label, "Start");
    EXPECT_EQ(findNode(*inlineModel, "B")->label, "End");
    EXPECT_TRUE(hasEdge(*inlineModel, "A", "B"));

    const auto piped = mermaid.import("graph TD\n A -->|yes| B");
    ASSERT_TRUE(piped.has_value());
    ASSERT_EQ(piped->edges.size(), 1u);
    EXPECT_EQ(piped->edges.front().label, "yes");
}

TEST(MermaidImporter, NodeShapes)
{
    MermaidImporter mermaid;
    const auto model = mermaid.import("graph TD\n A((Circle))\n B{Diamond}\n C(Round)");
    ASSERT_TRUE(model.has_value());
    EXPECT_EQ(findNode(*model, "A")->label, "Circle");
    EXPECT_EQ(findNode(*model, "B")->label, "Diamond");
    EXPECT_EQ(findNode(*model, "C")->label, "Round");
}

TEST(GraphMlImporter, NodesEdgesAndDataLabel)
{
    GraphMlImporter graphml;
    const auto model = graphml.import(R"(<?xml version="1.0"?>
<graphml><graph id="G" edgedefault="directed">
  <node id="n0"><data key="d0">Alpha</data></node>
  <node id="n1"/>
  <edge source="n0" target="n1"/>
</graph></graphml>)");
    ASSERT_TRUE(model.has_value());
    EXPECT_EQ(model->nodes.size(), 2u);
    ASSERT_NE(findNode(*model, "n0"), nullptr);
    EXPECT_EQ(findNode(*model, "n0")->label, "Alpha");
    EXPECT_TRUE(hasEdge(*model, "n0", "n1"));
}

TEST(PlantUmlImporter, DeclarationsAndRelationships)
{
    PlantUmlImporter uml;
    const auto model = uml.import(R"(@startuml
class Animal
class Dog
abstract class Base
Animal <|-- Dog
Dog --> Base : owns
@enduml)");
    ASSERT_TRUE(model.has_value());
    ASSERT_NE(findNode(*model, "Animal"), nullptr);
    ASSERT_NE(findNode(*model, "Dog"), nullptr);
    ASSERT_NE(findNode(*model, "Base"), nullptr);
    // `Animal <|-- Dog` puts the head on the left -> edge Dog -> Animal.
    EXPECT_TRUE(hasEdge(*model, "Dog", "Animal"));
    EXPECT_TRUE(hasEdge(*model, "Dog", "Base"));
    const auto owns = std::find_if(model->edges.begin(), model->edges.end(),
                                   [](const DiagramEdge &edge) { return edge.label == "owns"; });
    EXPECT_NE(owns, model->edges.end());
}

TEST(PlantUmlImporter, IgnoresClassBody)
{
    PlantUmlImporter uml;
    const auto model = uml.import(R"(@startuml
class Account {
  +balance : int
  +deposit()
}
Account --> Ledger
@enduml)");
    ASSERT_TRUE(model.has_value());
    EXPECT_EQ(model->nodes.size(), 2u); // Account + Ledger, no members
    EXPECT_TRUE(hasEdge(*model, "Account", "Ledger"));
}

TEST(JsonTreeImporter, ObjectArrayAndScalarTree)
{
    JsonTreeImporter json;
    const auto model = json.import(R"({"name":"svc","ports":[80,443],"tls":true})");
    ASSERT_TRUE(model.has_value());
    // root + name + svc-value(leaf is the name node itself) + ports + 2 elems + tls.
    ASSERT_NE(findNode(*model, "n0"), nullptr);
    EXPECT_EQ(findNode(*model, "n0")->label, "object");
    // Every child hangs off the root.
    int rootChildren = 0;
    for (const auto &edge : model->edges)
        if (edge.from == "n0")
            ++rootChildren;
    EXPECT_EQ(rootChildren, 3); // name, ports, tls
    // A primitive member carries "key: value"; an array element "[i] value".
    const bool hasNameLeaf = std::any_of(model->nodes.begin(), model->nodes.end(),
                                         [](const DiagramNode &n) { return n.label == "name: svc"; });
    const bool hasPortLeaf = std::any_of(model->nodes.begin(), model->nodes.end(),
                                         [](const DiagramNode &n) { return n.label == "[0] 80"; });
    EXPECT_TRUE(hasNameLeaf);
    EXPECT_TRUE(hasPortLeaf);
}

TEST(EdgeListImporter, AdjacencyAndMultiParent)
{
    EdgeListImporter edges;
    // git-style: a commit then its parents; a plain edge; a comment; a lone node.
    const auto model = edges.import("# history\nc3 c2 c1\nc2 c1\nc1\nlib utils");
    ASSERT_TRUE(model.has_value());
    EXPECT_TRUE(hasEdge(*model, "c3", "c2"));
    EXPECT_TRUE(hasEdge(*model, "c3", "c1")); // merge: second parent
    EXPECT_TRUE(hasEdge(*model, "c2", "c1"));
    EXPECT_TRUE(hasEdge(*model, "lib", "utils"));
    ASSERT_NE(findNode(*model, "c1"), nullptr); // lone node still present
}

TEST(GraphQlImporter, TypesFieldsAndUnion)
{
    GraphQlImporter graphql;
    const auto model = graphql.import(R"(
type User { id: ID! name: String posts: [Post!]! }
type Post { id: ID author: User }
union Content = Post | User
scalar DateTime
)");
    ASSERT_TRUE(model.has_value());
    ASSERT_NE(findNode(*model, "User"), nullptr);
    ASSERT_NE(findNode(*model, "Post"), nullptr);
    EXPECT_TRUE(hasEdge(*model, "User", "Post"));    // posts: [Post!]!
    EXPECT_TRUE(hasEdge(*model, "Post", "User"));    // author: User
    EXPECT_TRUE(hasEdge(*model, "Content", "Post")); // union member
    EXPECT_TRUE(hasEdge(*model, "Content", "User"));
    // Scalars never become edges.
    EXPECT_FALSE(hasEdge(*model, "User", "String"));
    EXPECT_EQ(findNode(*model, "String"), nullptr);
}

TEST(ProtobufImporter, MessagesFieldsAndMap)
{
    ProtobufImporter proto;
    const auto model = proto.import(R"(
message Address { string city = 1; }
message User {
  string name = 1;
  Address home = 2;
  repeated User friends = 3;
  map<string, Address> work = 4;
}
)");
    ASSERT_TRUE(model.has_value());
    ASSERT_NE(findNode(*model, "User"), nullptr);
    ASSERT_NE(findNode(*model, "Address"), nullptr);
    EXPECT_TRUE(hasEdge(*model, "User", "Address")); // home + map value
    EXPECT_FALSE(hasEdge(*model, "User", "string")); // scalar, not declared
    // self-reference (friends: User) is suppressed.
    EXPECT_FALSE(hasEdge(*model, "User", "User"));
}

TEST(ProtobufImporter, InlineMessageBody)
{
    ProtobufImporter proto;
    // Whole message on one line -- the field must still resolve.
    const auto model = proto.import("message A { string s = 1; }\nmessage B { A a = 1; }");
    ASSERT_TRUE(model.has_value());
    EXPECT_TRUE(hasEdge(*model, "B", "A"));
}

TEST(SqlErdImporter, TablesAndForeignKeys)
{
    SqlErdImporter sql;
    const auto model = sql.import(R"(
CREATE TABLE users (id INT PRIMARY KEY, name TEXT);
CREATE TABLE orders (
  id INT PRIMARY KEY,
  user_id INT REFERENCES users(id),
  FOREIGN KEY (coupon_id) REFERENCES coupons(id)
);
)");
    ASSERT_TRUE(model.has_value());
    ASSERT_NE(findNode(*model, "orders"), nullptr);
    ASSERT_NE(findNode(*model, "users"), nullptr);
    EXPECT_TRUE(hasEdge(*model, "orders", "users"));   // inline column REFERENCES
    EXPECT_TRUE(hasEdge(*model, "orders", "coupons")); // table-level FOREIGN KEY
}

TEST(FlameGraphImporter, FoldedStacksBuildPrefixTree)
{
    FlameGraphImporter flame;
    const auto model = flame.import("main;parse;lex 120\nmain;parse;ast 80\nmain;render 50");
    ASSERT_TRUE(model.has_value());
    // Shared prefix main->parse is one path; lex and ast diverge under parse.
    EXPECT_TRUE(hasEdge(*model, "main", "main;parse"));
    EXPECT_TRUE(hasEdge(*model, "main;parse", "main;parse;lex"));
    EXPECT_TRUE(hasEdge(*model, "main;parse", "main;parse;ast"));
    EXPECT_TRUE(hasEdge(*model, "main", "main;render"));
    // The node is keyed by path but labelled by the bare frame name.
    const DiagramNode *lex = findNode(*model, "main;parse;lex");
    ASSERT_NE(lex, nullptr);
    EXPECT_EQ(lex->label, "lex");
}

TEST(DiagramImporters, EmptyInputYieldsNoModel)
{
    DotImporter dot;
    MermaidImporter mermaid;
    GraphMlImporter graphml;
    PlantUmlImporter uml;
    JsonTreeImporter json;
    EdgeListImporter edges;
    GraphQlImporter graphql;
    ProtobufImporter proto;
    SqlErdImporter sql;
    FlameGraphImporter flame;
    EXPECT_FALSE(dot.import("digraph {}").has_value());
    EXPECT_FALSE(mermaid.import("graph TD").has_value());
    EXPECT_FALSE(graphml.import("<graphml></graphml>").has_value());
    EXPECT_FALSE(uml.import("@startuml\n@enduml").has_value());
    EXPECT_FALSE(json.import("{ not valid json").has_value());
    EXPECT_FALSE(edges.import("# only a comment\n   ").has_value());
    EXPECT_FALSE(graphql.import("# nothing here").has_value());
    EXPECT_FALSE(proto.import("syntax = \"proto3\";").has_value());
    EXPECT_FALSE(sql.import("SELECT 1;").has_value());
    EXPECT_FALSE(flame.import("# just a comment").has_value());
}

} // namespace
