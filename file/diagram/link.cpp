#include <QtWidgets>
#include <iostream>

#include "link.h"
#include "node.h"

namespace {

Node *find_node(const std::map<int, Node *> &nodeList, int index)
{
    auto iter = nodeList.find(index);
    if (iter == nodeList.end())
        return NULL;
    return iter->second;
}

}

Link::Link(Node *fromNode, Node *toNode)
{
    myFromNode = fromNode;
    myToNode = toNode;

    myFromNode->addLink(this);
    myToNode->addLink(this);

    setFlags(QGraphicsItem::ItemIsSelectable);
    setZValue(-1);

    setColor(Qt::darkRed);
    trackNodes();
}

Link::~Link()
{
    myFromNode->removeLink(this);
    myToNode->removeLink(this);
}

Node *Link::fromNode() const
{
    return myFromNode;
}

Node *Link::toNode() const
{
    return myToNode;
}

void Link::setColor(const QColor &color)
{
    setPen(QPen(color, 1.0));
}

QColor Link::color() const
{
    return pen().color();
}

void Link::trackNodes()
{
    setLine(QLineF(myFromNode->pos(), myToNode->pos()));
}

Link *Link::newFromJson(json11::Json json, const std::map<int, Node *> &nodeList)
{
    auto from = json["from"];
    if (!from.is_number()) {
        std::cerr << "invalid json of link, no from property\n";
        return NULL;
    }

    auto fromNode = find_node(nodeList, from.int_value());
    if (!fromNode) {
        std::cerr << "invalid link from index\n";
        return NULL;
    }

    auto to = json["to"];
    if (!to.is_number()) {
        std::cerr << "invalid json of link, no to property\n";
        return NULL;
    }

    auto toNode = find_node(nodeList, to.int_value());
    if (!toNode) {
        std::cerr << "invalid link to index\n";
        return NULL;
    }

    return new Link(fromNode, toNode);
}

json11::Json Link::toJson()
{
    using json11::Json;
    Json obj = Json::object({
        {"from", fromNode()->index()},
        {"to", toNode()->index()}
    });

    return obj;
}
