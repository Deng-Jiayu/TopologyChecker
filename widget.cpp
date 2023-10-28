#include "widget.h"
#include <QMouseEvent>
#include <QDebug>

Widget::Widget(QWidget *parent) : QWidget(parent)
{
    menu = new QMenu(this);
    acAddGroup = menu->addAction(QStringLiteral("�½������"));

    connect(acAddGroup, &QAction::triggered, this, &Widget::addGroup);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this,&Widget::customContextMenuRequested,this,&Widget::ShowcustomContextMenu);
}

void Widget::ShowcustomContextMenu()
{
    menu->exec(QCursor::pos());
}
