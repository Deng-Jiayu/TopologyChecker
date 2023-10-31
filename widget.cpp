#include "widget.h"
#include <QMouseEvent>
#include <QDebug>

Widget::Widget(QWidget *parent) : QWidget(parent)
{
    menu = new QMenu(this);
    acAddGroup = menu->addAction(QStringLiteral("新建检查组"));
    acRunAll = menu->addAction(QStringLiteral("一键检查项"));

    connect(acAddGroup, &QAction::triggered, this, &Widget::addGroup);
    connect(acRunAll, &QAction::triggered, this, &Widget::runAll);

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this,&Widget::customContextMenuRequested,this,&Widget::ShowcustomContextMenu);
}

void Widget::ShowcustomContextMenu()
{
    menu->exec(QCursor::pos());
}
