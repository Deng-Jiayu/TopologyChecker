#ifndef COLLAPSIBLEGROUPBOX_H
#define COLLAPSIBLEGROUPBOX_H

#include <qgscollapsiblegroupbox.h>

class CollapsibleGroupBox : public QgsCollapsibleGroupBox
{
    Q_OBJECT

public:
    CollapsibleGroupBox(const QString &title,QWidget *parent = nullptr);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

};


#endif // COLLAPSIBLEGROUPBOX_H
