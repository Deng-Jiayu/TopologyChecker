#ifndef CHECKITEMDIALOG_H
#define CHECKITEMDIALOG_H

#include <QDialog>
#include <qgisinterface.h>
#include <qgsdoublespinbox.h>
#include <QComboBox>
#include <QCheckBox>

#include "checkset.h"

namespace Ui {
class CheckItemDialog;
}

class CheckItemDialog : public QDialog
{
    Q_OBJECT

public:
    CheckItemDialog(QgisInterface *iface, CheckItem *item, QWidget *parent = nullptr);
    ~CheckItemDialog();

    void set();

private:
    Ui::CheckItemDialog *ui;
    QgisInterface *iface = nullptr;
    CheckItem *mcheckItem = nullptr;

    QMap<int, CheckSet*> mCheckMap;


    void initTable();
    void initParaTable();

    QPushButton* layerA;
    QPushButton* layerB;
    QPushButton* layerC;
    QgsDoubleSpinBox* doubleSpinBoxMax;
    QgsDoubleSpinBox* doubleSpinBoxMin;
    QComboBox* comboBoxLayerMode;
    QgsDoubleSpinBox *doubleSpinBoxAngle;
    QCheckBox *excludeEndpoint;

};

#endif // CHECKITEMDIALOG_H
