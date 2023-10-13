#ifndef COLLAPSIBLEGROUPBOX_H
#define COLLAPSIBLEGROUPBOX_H

#include <qgscollapsiblegroupbox.h>

class CollapsibleGroupBox : public QgsCollapsibleGroupBox
{
    Q_OBJECT

public:
    CollapsibleGroupBox(const QString &title,QWidget *parent = nullptr);
    ~CollapsibleGroupBox();

signals:
    void rename();
    void remove();
    void addGroup();
    void addItem();
    void runAll();

private:
    QMenu *menu;

    QAction *acRemove;
    QAction *acRename;
    QAction *acAddGroup;
    QAction *acAddItem;
    QAction *acRunAll;

private slots:
    void ShowcustomContextMenu();

};

#endif // COLLAPSIBLEGROUPBOX_H
