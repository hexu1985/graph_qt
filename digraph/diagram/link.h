#ifndef LINK_H
#define LINK_H

#include <QGraphicsLineItem>

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

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option, QWidget *widget);

    void turnRound();

private:
    Node *myFromNode;
    Node *myToNode;
};

#endif
