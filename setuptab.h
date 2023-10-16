#ifndef SETUPTAB_H
#define SETUPTAB_H

#include <QWidget>
#include <qgisinterface.h>
#include <QPushButton>
#include <QMutex>
#include "collapsiblegroupbox.h"
#include "pushbutton.h"
#include "checkset.h"

namespace Ui {
class SetupTab;
}

class SetupTab : public QWidget
{
    Q_OBJECT

public:
    SetupTab( QgisInterface *iface, QDockWidget *checkDock, QWidget *parent = nullptr );
    ~SetupTab() override;

private:
    Ui::SetupTab *ui;
    QgisInterface *mIface = nullptr;
    QDockWidget *mCheckDock;

    CheckList *curList = nullptr;
    QVector<CheckList> mlists;
    QMap<CollapsibleGroupBox *, CheckGroup *> boxToGroup;
    QMap<PushButton *, CheckItem *> btnToCheck;

    QVector<CollapsibleGroupBox*> groupBoxs;
    QVector<PushButton*> btns;

    void initLists();
    void initUi();
    void initConnection();
    void save();
    void read();
    void createList();
    void deleteList();

private slots:
    void addGroup();
    void addItem();
    void deleteGroup();
    void rename();
};

#endif // SETUPTAB_H
