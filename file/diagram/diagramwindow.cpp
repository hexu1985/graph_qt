#include <QtWidgets>
#include <fstream>
#include <string>
#include <iterator>
#include <iostream>

#include "diagramwindow.h"
#include "link.h"
#include "node.h"
#include "propertiesdialog.h"

namespace {
const bool AUTO_POS = true;
const bool NON_AUTO_POS = false;
}

DiagramWindow::DiagramWindow()
{
    scene = new QGraphicsScene(0, 0, 600, 500);

    view = new QGraphicsView;
    view->setScene(scene);
    view->setDragMode(QGraphicsView::RubberBandDrag);
    view->setRenderHints(QPainter::Antialiasing
                         | QPainter::TextAntialiasing);
    view->setContextMenuPolicy(Qt::ActionsContextMenu);
    setCentralWidget(view);

    minZ = 0;
    maxZ = 0;
    seqNumber = 0;

    createActions();
    createMenus();
    createToolBars();

    connect(scene, SIGNAL(selectionChanged()),
            this, SLOT(updateActions()));

    setWindowTitle(tr("Diagram"));
    updateActions();
    setCurrentFile("");
}

void DiagramWindow::closeEvent(QCloseEvent *event)
{
    if (okToContinue()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void DiagramWindow::clear()
{
    for (auto link: linkList)
        delete link;
    linkList.clear();

    for (auto node: nodeList)
        delete node.second;
    nodeList.clear();

    minZ = 0;
    maxZ = 0;
    seqNumber = 0;

    setWindowTitle(tr("Diagram"));
    updateActions();
    setCurrentFile("");
}

void DiagramWindow::newFile()
{
    if (okToContinue()) {
        clear();
    }
}

void DiagramWindow::open()
{
    if (okToContinue()) {
        clear();
        QString fileName = QFileDialog::getOpenFileName(this,
                                   tr("Open Diagram"), ".",
                                   tr("Diagram files (*.diag)"));
        if (!fileName.isEmpty())
            loadFile(fileName);
    }
}

bool DiagramWindow::save()
{
    if (curFile.isEmpty()) {
        return saveAs();
    } else {
        return saveFile(curFile);
    }
}

bool DiagramWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                    tr("Save diagram"), ".",
                                    tr("Diagram files (*.diag)"));
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

void DiagramWindow::addNode()
{
    int index = seqNumber + 1;
    Node *node = new Node(index);
    node->setText(tr("Node %1").arg(index));
    setupNode(node, AUTO_POS);
    setWindowModified(true);
}

void DiagramWindow::addLink()
{
    NodePair nodes = selectedNodePair();
    if (nodes == NodePair())
        return;

    Link *link = new Link(nodes.first, nodes.second);
    setupLink(link);
    setWindowModified(true);
}

void DiagramWindow::del()
{
    QList<QGraphicsItem *> items = scene->selectedItems();
    QMutableListIterator<QGraphicsItem *> i(items);
    while (i.hasNext()) {
        QGraphicsItem *item = i.next();
        Link *link = dynamic_cast<Link *>(item);
        if (link) {
            delete link;
            i.remove();
        }
        Node *node = dynamic_cast<Node *>(item);
        if (node) {
            nodeList.erase(node->index());
        }
    }

    qDeleteAll(items);
    setWindowModified(true);
}

void DiagramWindow::cut()
{
    Node *node = selectedNode();
    if (!node)
        return;

    copy();
    delete node;
}

void DiagramWindow::copy()
{
    Node *node = selectedNode();
    if (!node)
        return;

    QString str = QString("Node %1 %2 %3 %4")
                  .arg(node->textColor().name())
                  .arg(node->outlineColor().name())
                  .arg(node->backgroundColor().name())
                  .arg(node->text());
    QApplication::clipboard()->setText(str);
}

void DiagramWindow::paste()
{
    QString str = QApplication::clipboard()->text();
    QStringList parts = str.split(" ");

    if (parts.count() >= 5 && parts.first() == "Node") {
        int index = seqNumber + 1;
        Node *node = new Node(index);
        node->setText(QStringList(parts.mid(4)).join(" "));
        node->setTextColor(QColor(parts[1]));
        node->setOutlineColor(QColor(parts[2]));
        node->setBackgroundColor(QColor(parts[3]));
        setupNode(node, AUTO_POS);
    }
    setWindowModified(true);
}

void DiagramWindow::bringToFront()
{
    ++maxZ;
    setZValue(maxZ);
}

void DiagramWindow::sendToBack()
{
    --minZ;
    setZValue(minZ);
}

void DiagramWindow::properties()
{
    Node *node = selectedNode();
    Link *link = selectedLink();

    if (node) {
        PropertiesDialog dialog(node, this);
        dialog.exec();
    } else if (link) {
        QColor color = QColorDialog::getColor(link->color(), this);
        if (color.isValid())
            link->setColor(color);
    }
}

void DiagramWindow::updateActions()
{
    bool hasSelection = !scene->selectedItems().isEmpty();
    bool isNode = (selectedNode() != 0);
    bool isNodePair = (selectedNodePair() != NodePair());

    cutAction->setEnabled(isNode);
    copyAction->setEnabled(isNode);
    addLinkAction->setEnabled(isNodePair);
    deleteAction->setEnabled(hasSelection);
    bringToFrontAction->setEnabled(isNode);
    sendToBackAction->setEnabled(isNode);
    propertiesAction->setEnabled(isNode);

    foreach (QAction *action, view->actions())
        view->removeAction(action);

    foreach (QAction *action, editMenu->actions()) {
        if (action->isEnabled())
            view->addAction(action);
    }
}

void DiagramWindow::createActions()
{
    newAction = new QAction(tr("&New"), this);
    newAction->setIcon(QIcon(":/images/new.png"));
    newAction->setShortcut(QKeySequence::New);
    newAction->setStatusTip(tr("Create a new spreadsheet file"));
    connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

    openAction = new QAction(tr("&Open..."), this);
    openAction->setIcon(QIcon(":/images/open.png"));
    openAction->setShortcut(QKeySequence::Open);
    openAction->setStatusTip(tr("Open an existing spreadsheet file"));
    connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

    saveAction = new QAction(tr("&Save"), this);
    saveAction->setIcon(QIcon(":/images/save.png"));
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setStatusTip(tr("Save the spreadsheet to disk"));
    connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAction = new QAction(tr("Save &As..."), this);
    saveAsAction->setStatusTip(tr("Save the spreadsheet under a new "
                                  "name"));
    connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(tr("Ctrl+Q"));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    addNodeAction = new QAction(tr("Add &Node"), this);
    addNodeAction->setIcon(QIcon(":/images/node.png"));
    addNodeAction->setShortcut(tr("Ctrl+N"));
    connect(addNodeAction, SIGNAL(triggered()), this, SLOT(addNode()));

    addLinkAction = new QAction(tr("Add &Link"), this);
    addLinkAction->setIcon(QIcon(":/images/link.png"));
    addLinkAction->setShortcut(tr("Ctrl+L"));
    connect(addLinkAction, SIGNAL(triggered()), this, SLOT(addLink()));

    deleteAction = new QAction(tr("&Delete"), this);
    deleteAction->setIcon(QIcon(":/images/delete.png"));
    deleteAction->setShortcut(tr("Del"));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(del()));

    cutAction = new QAction(tr("Cu&t"), this);
    cutAction->setIcon(QIcon(":/images/cut.png"));
    cutAction->setShortcut(tr("Ctrl+X"));
    connect(cutAction, SIGNAL(triggered()), this, SLOT(cut()));

    copyAction = new QAction(tr("&Copy"), this);
    copyAction->setIcon(QIcon(":/images/copy.png"));
    copyAction->setShortcut(tr("Ctrl+C"));
    connect(copyAction, SIGNAL(triggered()), this, SLOT(copy()));

    pasteAction = new QAction(tr("&Paste"), this);
    pasteAction->setIcon(QIcon(":/images/paste.png"));
    pasteAction->setShortcut(tr("Ctrl+V"));
    connect(pasteAction, SIGNAL(triggered()), this, SLOT(paste()));

    bringToFrontAction = new QAction(tr("Bring to &Front"), this);
    bringToFrontAction->setIcon(QIcon(":/images/bringtofront.png"));
    connect(bringToFrontAction, SIGNAL(triggered()),
            this, SLOT(bringToFront()));

    sendToBackAction = new QAction(tr("&Send to Back"), this);
    sendToBackAction->setIcon(QIcon(":/images/sendtoback.png"));
    connect(sendToBackAction, SIGNAL(triggered()),
            this, SLOT(sendToBack()));

    propertiesAction = new QAction(tr("P&roperties..."), this);
    connect(propertiesAction, SIGNAL(triggered()),
            this, SLOT(properties()));
}

void DiagramWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAction);
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addAction(exitAction);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(addNodeAction);
    editMenu->addAction(addLinkAction);
    editMenu->addAction(deleteAction);
    editMenu->addSeparator();
    editMenu->addAction(cutAction);
    editMenu->addAction(copyAction);
    editMenu->addAction(pasteAction);
    editMenu->addSeparator();
    editMenu->addAction(bringToFrontAction);
    editMenu->addAction(sendToBackAction);
    editMenu->addSeparator();
    editMenu->addAction(propertiesAction);
}

void DiagramWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("&File"));
    fileToolBar->addAction(newAction);
    fileToolBar->addAction(openAction);
    fileToolBar->addAction(saveAction);

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(addNodeAction);
    editToolBar->addAction(addLinkAction);
    editToolBar->addAction(deleteAction);
    editToolBar->addSeparator();
    editToolBar->addAction(cutAction);
    editToolBar->addAction(copyAction);
    editToolBar->addAction(pasteAction);
    editToolBar->addSeparator();
    editToolBar->addAction(bringToFrontAction);
    editToolBar->addAction(sendToBackAction);
}

void DiagramWindow::setZValue(int z)
{
    Node *node = selectedNode();
    if (node)
        node->setZValue(z);
}

void DiagramWindow::setupLink(Link *link)
{
    scene->addItem(link);
    linkList.insert(link);
}

void DiagramWindow::setupNode(Node *node, bool autoPos)
{
    if (autoPos) {
        node->setPos(QPoint(80 + (100 * (seqNumber % 5)),
                    80 + (50 * ((seqNumber / 5) % 7))));
    }
    scene->addItem(node);
    if (seqNumber < node->index())
        seqNumber = node->index();

    scene->clearSelection();
    node->setSelected(true);
    bringToFront();
    nodeList[node->index()] = node;
}

Node *DiagramWindow::selectedNode() const
{
    QList<QGraphicsItem *> items = scene->selectedItems();
    if (items.count() == 1) {
        return dynamic_cast<Node *>(items.first());
    } else {
        return 0;
    }
}

Link *DiagramWindow::selectedLink() const
{
    QList<QGraphicsItem *> items = scene->selectedItems();
    if (items.count() == 1) {
        return dynamic_cast<Link *>(items.first());
    } else {
        return 0;
    }
}

DiagramWindow::NodePair DiagramWindow::selectedNodePair() const
{
    QList<QGraphicsItem *> items = scene->selectedItems();
    if (items.count() == 2) {
        Node *first = dynamic_cast<Node *>(items.first());
        Node *second = dynamic_cast<Node *>(items.last());
        if (first && second)
            return NodePair(first, second);
    }
    return NodePair();
}

bool DiagramWindow::deserializeFromJson(const std::string &str)
{
    using json11::Json;
    std::string err;
    auto json = Json::parse(str, err);
    
    if (!err.empty()) {
        std::cerr << "parse json error: " << err << '\n';
        return false;
    }

    auto nodes = json["nodes"];
    if (nodes.is_array()) {
        for (auto nodeJson: nodes.array_items()) {
            auto node = Node::newFromJson(nodeJson);
            if (!node)
                continue;
            setupNode(node, NON_AUTO_POS);
        }
        setWindowModified(true);
    }

    auto links = json["links"];
    if (links.is_array()) {
        for (auto linkJson: links.array_items()) {
            auto link = Link::newFromJson(linkJson, nodeList);
            if (!link)
                continue;
            setupLink(link);
        }
        setWindowModified(true);
    }

    return true;
}

bool DiagramWindow::loadFile(const QString &fileName)
{
    std::ifstream ifile(fileName.toStdString());
    if (!ifile) {
        QMessageBox::information(this, "Error", "open file fail!");
        return false;
    }

    std::istreambuf_iterator<char>  beg(ifile),  end;
    std::string str(beg,  end);

    if (!deserializeFromJson(str)) {
        QMessageBox::information(this, "Error", "parse file fail!");
        return false;
    }

    setCurrentFile(fileName);

    return true;
}

json11::Json DiagramWindow::serializeToJson()
{
    using json11::Json;
    Json::array nodes;
    for (auto node: nodeList) {
        auto obj = node.second->toJson();
        nodes.push_back(obj);
    }

    Json::array links;
    for (auto link: linkList) {
        auto obj = link->toJson();
        links.push_back(obj);
    }

    Json json = Json::object({
            {"nodes", nodes},
            {"links", links}
            });

    return json;
}

bool DiagramWindow::saveFile(const QString &fileName)
{
    std::ofstream ofile(fileName.toStdString());
    if (!ofile) {
        QMessageBox::information(this, "Error", "save file fail!");
        return false;
    }

    auto json = serializeToJson();
    ofile << json.dump() << std::endl;

    setCurrentFile(fileName);

    return true;
}

void DiagramWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    setWindowModified(false);

    QString shownName = tr("Untitled");
    if (!curFile.isEmpty()) {
        shownName = strippedName(curFile);
    }

    setWindowTitle(tr("%1 - %2[*]").arg(tr("Diagram"))
                                   .arg(shownName));
}

QString DiagramWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

bool DiagramWindow::okToContinue()
{
    if (isWindowModified()) {
        int r = QMessageBox::warning(this, tr("Diagram"),
                tr("The document has been modified.\n"
                    "Do you want to save your changes?"),
                QMessageBox::Yes | QMessageBox::No
                | QMessageBox::Cancel);
        if (r == QMessageBox::Yes) {
            return save();
        } else if (r == QMessageBox::Cancel) {
            return false;
        }
    }
    return true;
}
