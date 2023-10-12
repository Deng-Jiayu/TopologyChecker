#include "pushbutton.h"
#include <QMouseEvent>
#include <QDebug>
#include <QMenu>

PushButton::PushButton(QWidget *parent) : QPushButton(parent)
{

    menu = new QMenu(this);
    acRun = menu->addAction(QStringLiteral("执行检查项（对全图）"));
    acRunSelected = menu->addAction(QStringLiteral("执行检查项（对选中地物）"));
    acAdd = menu->addAction(QStringLiteral("新建检查项"));
    acRemove = menu->addAction(QStringLiteral("删除"));
    acRename = menu->addAction(QStringLiteral("重命名"));
    acRunAll = menu->addAction(QStringLiteral("一键检查项"));

    connect(acRun, &QAction::triggered, this, &PushButton::run);
    connect(acRename, &QAction::triggered, this, &PushButton::rename);
    connect(acRemove, &QAction::triggered, this, &PushButton::remove);
}

void PushButton::mouseReleaseEvent(QMouseEvent *event)
{
    QPushButton::mouseReleaseEvent(event);

    if (event->button() == Qt::RightButton)
    {
        menu->exec(QCursor::pos());
    }
}
