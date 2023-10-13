#include "collapsiblegroupbox.h"
#include <QMouseEvent>
#include <QDebug>
#include <QMenu>

CollapsibleGroupBox::CollapsibleGroupBox(const QString &title, QWidget *parent)
    : QgsCollapsibleGroupBox(title, parent)
{

    menu = new QMenu();
    acAddItem = menu->addAction(QStringLiteral("新建检查项"));
    acAddGroup = menu->addAction(QStringLiteral("新建检查组"));
    acRemove = menu->addAction(QStringLiteral("删除"));
    acRename = menu->addAction(QStringLiteral("重命名"));
    acRunAll = menu->addAction(QStringLiteral("一键检查项"));

    connect(acAddGroup, &QAction::triggered, this, &CollapsibleGroupBox::addGroup);
    connect(acAddItem, &QAction::triggered, this, &CollapsibleGroupBox::addItem);
    connect(acRename, &QAction::triggered, this, &CollapsibleGroupBox::rename);
    connect(acRemove, &QAction::triggered, this, &CollapsibleGroupBox::remove);
    connect(acRunAll, &QAction::triggered, this, &CollapsibleGroupBox::runAll);

    menu->hide();

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this,&CollapsibleGroupBox::customContextMenuRequested,this,&CollapsibleGroupBox::ShowcustomContextMenu);
}

CollapsibleGroupBox::~CollapsibleGroupBox()
{
    delete menu;
}

void CollapsibleGroupBox::ShowcustomContextMenu()
{
    menu->exec(QCursor::pos());
}
