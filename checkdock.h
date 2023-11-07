#ifndef CHECKDOCK_H
#define CHECKDOCK_H

#include <QDockWidget>
#include <qgisinterface.h>
#include <QDialogButtonBox>
#include <QTabWidget>
#include "checker.h"

namespace Ui {
class CheckDock;
}

class CheckDock : public QDockWidget
{
    Q_OBJECT

public:
    CheckDock(QgisInterface *iface, QWidget *parent = nullptr);
    ~CheckDock();

private:
    Ui::CheckDock *ui;
    QgisInterface *mIface;
    QTabWidget *mTabWidget = nullptr;

    void closeEvent( QCloseEvent *ev ) override;

private slots:
    void onCheckerStarted( Checker *checker );
    void onCheckerFinished( bool successful );
};

#endif // CHECKDOCK_H
