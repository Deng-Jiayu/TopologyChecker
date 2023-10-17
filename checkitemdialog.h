#ifndef CHECKITEMDIALOG_H
#define CHECKITEMDIALOG_H

#include <QDialog>

#include "checkset.h"

namespace Ui {
class CheckItemDialog;
}

class CheckItemDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CheckItemDialog(CheckItem*item,QWidget *parent = nullptr);
    ~CheckItemDialog();

    void set();

private:
    Ui::CheckItemDialog *ui;

    CheckItem *item = nullptr;

};

#endif // CHECKITEMDIALOG_H
