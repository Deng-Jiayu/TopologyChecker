#ifndef CHECKDOCK_H
#define CHECKDOCK_H

#include <QDockWidget>
#include <qgisinterface.h>
#include <QDialogButtonBox>
#include <QTabWidget>

namespace Ui {
class CheckDock;
}

class CheckDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit CheckDock(QgisInterface *iface, QWidget *parent = nullptr);
    ~CheckDock();

private:
    Ui::CheckDock *ui;
    QgisInterface *mIface;
    QDialogButtonBox *mButtonBox = nullptr;
    QTabWidget *mTabWidget = nullptr;

};

#endif // CHECKDOCK_H
