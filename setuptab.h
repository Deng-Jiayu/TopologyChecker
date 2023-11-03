#ifndef SETUPTAB_H
#define SETUPTAB_H

#include <QWidget>
#include <qgisinterface.h>
#include <QPushButton>
#include <QMutex>
#include "collapsiblegroupbox.h"
#include "pushbutton.h"
#include "checkset.h"
#include "checker.h"
#include "messagebox.h"

namespace Ui
{
    class SetupTab;
}

class SetupTab : public QWidget
{
    Q_OBJECT

public:
    SetupTab(QgisInterface *iface, QDockWidget *checkDock, QWidget *parent = nullptr);
    ~SetupTab() override;

signals:
    void checkerStarted(Checker *checker);
    void checkerFinished(bool);

private:
    Ui::SetupTab *ui;
    QgisInterface *mIface = nullptr;
    QDockWidget *mCheckDock;
    MessageBox *mMessageBox = nullptr;

    CheckList *curList = nullptr;
    QVector<CheckList> mlists;
    QMap<CollapsibleGroupBox *, CheckGroup *> boxToGroup;
    QMap<PushButton *, CheckItem *> btnToCheck;

    QVector<bool> boxCollapsed;
    QVector<CollapsibleGroupBox *> groupBoxs;
    QVector<PushButton *> btns;
    QSet<QgsVectorLayer *> layers;
    QPushButton *mAbortButton = nullptr;

    bool mIsRunningInBackground;

    void initLists();
    void initUi();
    void initConnection();
    void save();
    void read();
    void createList();
    void deleteList();

private slots:
    void updateLayers();
    void addGroup();
    void addItem();
    void remove();
    void rename();
    void runItem();
    void setPara();
    void run();

private:
    QList<Check *> getChecks(CheckContext *context);
    void initItemLayers();
    void setupItemLayers();
};

#endif // SETUPTAB_H
