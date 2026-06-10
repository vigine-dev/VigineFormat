#include "vigine/format/diagrammodel.h"

#include <algorithm>

namespace vigine::format
{
void DiagramModel::addNode(std::string_view id, std::string_view label)
{
    if (id.empty())
    {
        return;
    }
    const auto existing = std::find_if(nodes.begin(), nodes.end(),
                                       [id](const DiagramNode &node) { return node.id == id; });
    if (existing != nodes.end())
    {
        if (existing->label.empty() && !label.empty())
        {
            existing->label.assign(label);
        }
        return;
    }
    DiagramNode node;
    node.id.assign(id);
    node.label.assign(label); // empty label means "display the id" -- a later
                              // labelled mention fills it via the dedup path above
    nodes.push_back(std::move(node));
}

void DiagramModel::addEdge(std::string_view from, std::string_view to, std::string_view label,
                           bool directed)
{
    if (from.empty() || to.empty())
    {
        return;
    }
    addNode(from);
    addNode(to);
    DiagramEdge edge;
    edge.from.assign(from);
    edge.to.assign(to);
    edge.label.assign(label);
    edge.directed = directed;
    edges.push_back(std::move(edge));
}

} // namespace vigine::format
