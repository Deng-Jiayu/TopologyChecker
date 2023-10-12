#include "collapsiblegroupbox.h"
#include <QMouseEvent>
#include <QDebug>

CollapsibleGroupBox::CollapsibleGroupBox(const QString &title, QWidget *parent)
    : QgsCollapsibleGroupBox(title, parent)
{
}

void CollapsibleGroupBox::mouseReleaseEvent(QMouseEvent *event)
{
    QgsCollapsibleGroupBox::mouseReleaseEvent(event);

    if (event->button() == Qt::RightButton)
    {
        qDebug() << QStringLiteral("右键");
    }
}
