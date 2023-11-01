#ifndef CLASSIFYDIALOG_H
#define CLASSIFYDIALOG_H

#include <QDialog>
#include "check.h"

namespace Ui {
class ErrorTypeDialog;
}

class ClassifyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ClassifyDialog(QWidget *parent = nullptr);
    ~ClassifyDialog();

    QStringList selectedType();
    void initList(QList<Check *> checks);

signals:
    void ClassifyDone();

private:
    Ui::ErrorTypeDialog *ui;

    QSet<QString> types;

    void selectAll();
    void deselectAll();
};

#endif // CLASSIFYDIALOG_H
