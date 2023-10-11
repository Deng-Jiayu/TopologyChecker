#ifndef SETUPTAB_H
#define SETUPTAB_H

#include <QWidget>
#include <qgisinterface.h>
#include <QPushButton>
#include <QMutex>
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
    vector<CheckList> mlists;
    QMap<QPushButton *, CheckItem *> btnToCheck;

    void initLists();
    void initUi();
    void save();
    void read();
};

#endif // SETUPTAB_H
