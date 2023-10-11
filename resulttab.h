#ifndef RESULTTAB_H
#define RESULTTAB_H

#include <QWidget>

namespace Ui {
class ResultTab;
}

class ResultTab : public QWidget
{
    Q_OBJECT

public:
    explicit ResultTab(QWidget *parent = nullptr);
    ~ResultTab();

private:
    Ui::ResultTab *ui;
};

#endif // RESULTTAB_H
