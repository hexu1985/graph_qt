#include <QtWidgets>
#include <iostream>

#include "link.h"
#include "node.h"

Node::Node(int index)
{
    myIndex = index;
    myTextColor = Qt::darkGreen;
    myOutlineColor = Qt::darkBlue;
    myBackgroundColor = Qt::white;

    setFlags(ItemIsMovable | ItemIsSelectable);
}

Node::~Node()
{
    foreach (Link *link, myLinks)
        delete link;
}

void Node::setText(const QString &text)
{
    prepareGeometryChange();
    myText = text;
    update();
}

int Node::index() const
{
    return myIndex;
}

QString Node::text() const
{
    return myText;
}

void Node::setTextColor(const QColor &color)
{
    myTextColor = color;
    update();
}

QColor Node::textColor() const
{
    return myTextColor;
}

void Node::setOutlineColor(const QColor &color)
{
    myOutlineColor = color;
    update();
}

QColor Node::outlineColor() const
{
    return myOutlineColor;
}

void Node::setBackgroundColor(const QColor &color)
{
    myBackgroundColor = color;
    update();
}

QColor Node::backgroundColor() const
{
    return myBackgroundColor;
}

void Node::addLink(Link *link)
{
    myLinks.insert(link);
}

void Node::removeLink(Link *link)
{
    myLinks.remove(link);
}

QRectF Node::boundingRect() const
{
    const int Margin = 1;
    return outlineRect().adjusted(-Margin, -Margin, +Margin, +Margin);
}

QPainterPath Node::shape() const
{
    QRectF rect = outlineRect();

    QPainterPath path;
    path.addRoundRect(rect, roundness(rect.width()),
                      roundness(rect.height()));
    return path;
}

void Node::paint(QPainter *painter,
                 const QStyleOptionGraphicsItem *option,
                 QWidget * /* widget */)
{
    QPen pen(myOutlineColor);
    if (option->state & QStyle::State_Selected) {
        pen.setStyle(Qt::DotLine);
        pen.setWidth(2);
    }
    painter->setPen(pen);
    painter->setBrush(myBackgroundColor);

    QRectF rect = outlineRect();
    painter->drawRoundRect(rect, roundness(rect.width()),
                           roundness(rect.height()));

    painter->setPen(myTextColor);
    painter->drawText(rect, Qt::AlignCenter, myText);
}

void Node::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QString text = QInputDialog::getText(event->widget(),
                           tr("Edit Text"), tr("Enter new text:"),
                           QLineEdit::Normal, myText);
    if (!text.isEmpty())
        setText(text);
}

QVariant Node::itemChange(GraphicsItemChange change,
                          const QVariant &value)
{
    foreach (Link *link, myLinks)
        link->trackNodes();
    return QGraphicsItem::itemChange(change, value);
}

QRectF Node::outlineRect() const
{
    const int Padding = 8;
    //QFontMetricsF metrics = qApp->font();
    QFontMetricsF metrics(qApp->font());
    QRectF rect = metrics.boundingRect(myText);
    rect.adjust(-Padding, -Padding, +Padding, +Padding);
    rect.translate(-rect.center());
    return rect;
}

int Node::roundness(double size) const
{
    const int Diameter = 12;
    return 100 * Diameter / int(size);
}

json11::Json Node::toJson()
{
    using json11::Json;
    Json obj = Json::object({
        {"index", myIndex},
        {"text", myText.toStdString()},
        {"x", (int) this->x()},
        {"y", (int) this->y()}
    });

    return obj;
}

Node *Node::newFromJson(json11::Json json)
{
    auto index = json["index"];
    if (!index.is_number()) {
        std::cerr << "invalid json of node, no index property\n";
        return NULL;
    }

    auto text = json["text"];
    if (!text.is_string()) {
        std::cerr << "invalid json of node, no string property\n";
        return NULL;
    }

    auto xPos = json["x"];
    if (!xPos.is_number()) {
        std::cerr << "invalid json of node, no x property\n";
        return NULL;
    }

    auto yPos = json["y"];
    if (!yPos.is_number()) {
        std::cerr << "invalid json of node, no y property\n";
        return NULL;
    }

    auto node = new Node(index.int_value());
    node->setText(QString(text.string_value().c_str()));
    node->setX(xPos.int_value());
    node->setY(yPos.int_value());

    return node;
}
