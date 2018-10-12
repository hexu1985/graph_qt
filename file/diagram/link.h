#ifndef LINK_H
#define LINK_H

#include <QGraphicsLineItem>
#include "json11.hpp"

class Node;

class Link : public QGraphicsLineItem
{
public:
    Link(Node *fromNode, Node *toNode);
    ~Link();

    Node *fromNode() const;
    Node *toNode() const;

    void setColor(const QColor &color);
    QColor color() const;

    void trackNodes();

    static Link *newFromJson(json11::Json json, const std::map<int, Node *> &nodeList);
    json11::Json toJson();

private:
    Node *myFromNode;
    Node *myToNode;
};

#endif
