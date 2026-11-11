#include "vigine/format/diagrammodel.h"

#include <algorithm>
#include <utility>

namespace vigine::format
{
DiagramNode::DiagramNode(std::string id, std::string label, std::uint32_t kind)
    : _id{std::move(id)}, _label{std::move(label)}, _kind{kind}
{
}

void DiagramNode::setLabel(std::string_view label)
{
    _label.assign(label);
}

DiagramEdge::DiagramEdge(std::string from, std::string to, std::string label, bool directed)
    : _from{std::move(from)}, _to{std::move(to)}, _label{std::move(label)}, _directed{directed}
{
}

void DiagramModel::addNode(std::string_view id, std::string_view label)
{
    if (id.empty())
    {
        return;
    }
    const auto existing = std::find_if(_nodes.begin(), _nodes.end(),
                                       [id](const DiagramNode &node) { return node.id() == id; });
    if (existing != _nodes.end())
    {
        if (existing->label().empty() && !label.empty())
        {
            existing->setLabel(label);
        }
        return;
    }
    // An empty label means "display the id"; a later labelled mention fills it
    // through the dedup path above.
    _nodes.emplace_back(std::string{id}, std::string{label});
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
    _edges.emplace_back(std::string{from}, std::string{to}, std::string{label}, directed);
}

} // namespace vigine::format
