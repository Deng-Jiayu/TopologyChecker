#include "pushbutton.h"
#include <QMouseEvent>
#include <QDebug>
#include <QMenu>

PushButton::PushButton(QWidget *parent) : QPushButton(parent)
{

    menu = new QMenu(this);
    acRun = menu->addAction(QStringLiteral("执行检查项（对全图）"));
    acRunSelected = menu->addAction(QStringLiteral("执行检查项（对选中地物）"));
    acAddItem = menu->addAction(QStringLiteral("新建检查项"));
    acAddGroup = menu->addAction(QStringLiteral("新建检查组"));
    acRemove = menu->addAction(QStringLiteral("删除项"));
    acRename = menu->addAction(QStringLiteral("重命名项"));
    acRunAll = menu->addAction(QStringLiteral("一键检查项"));

    connect(acRun, &QAction::triggered, this, &PushButton::run);
    connect(acRunSelected, &QAction::triggered, this, &PushButton::runSelected);
    connect(acAddGroup, &QAction::triggered, this, &PushButton::addGroup);
    connect(acAddItem, &QAction::triggered, this, &PushButton::addItem);
    connect(acRename, &QAction::triggered, this, &PushButton::rename);
    connect(acRemove, &QAction::triggered, this, &PushButton::remove);
    connect(acRunAll, &QAction::triggered, this, &PushButton::runAll);

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this,&PushButton::customContextMenuRequested,this,&PushButton::ShowcustomContextMenu);
}

void PushButton::ShowcustomContextMenu()
{
    menu->exec(QCursor::pos());
}
