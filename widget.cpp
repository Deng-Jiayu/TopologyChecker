#include "widget.h"
#include <QMouseEvent>
#include <QDebug>

Widget::Widget(QWidget *parent) : QWidget(parent)
{
    menu = new QMenu(this);
    acAddGroup = menu->addAction(QStringLiteral("�½������"));
    acRunAll = menu->addAction(QStringLiteral("һ�������"));

    connect(acAddGroup, &QAction::triggered, this, &Widget::addGroup);
    connect(acRunAll, &QAction::triggered, this, &Widget::runAll);

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this,&Widget::customContextMenuRequested,this,&Widget::ShowcustomContextMenu);
}

void Widget::ShowcustomContextMenu()
{
    menu->exec(QCursor::pos());
}
