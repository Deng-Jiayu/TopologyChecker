#ifndef CHECKITEMDIALOG_H
#define CHECKITEMDIALOG_H

#include <QDialog>
#include <qgisinterface.h>
#include <qgsdoublespinbox.h>
#include <QComboBox>
#include <QCheckBox>
#include "layerselectiondialog.h"

#include "checker.h"
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

signals:
    void checkerStarted( Checker *checker );
    void checkerFinished( bool );

private:
    Ui::CheckItemDialog *ui;
    QgisInterface *iface = nullptr;
    CheckItem *mcheckItem = nullptr;
    CheckSet *mCheckSet = nullptr;
    LayerSelectionDialog* layerSelectDialog = nullptr;
    QMap<int, CheckSet *> mCheckMap;
    bool mIsRunningInBackground;

    void initTable();
    void initParaTable();
    void initConnection();
    void setBtnText(QPushButton* btn, QVector<QgsVectorLayer*> vec);
    void setParaUi(QPushButton *btn);
    void setText(QPushButton* btn);

    QPushButton *layerA;
    QPushButton *layerB;
    QPushButton *layerC;
    QgsDoubleSpinBox *doubleSpinBoxMax;
    QgsDoubleSpinBox *doubleSpinBoxMin;
    QComboBox *comboBoxLayerMode;
    QgsDoubleSpinBox *doubleSpinBoxAngle;
    QCheckBox *excludeEndpoint;

private slots:
    void onSelectionChanged(const QItemSelection& newSel, const QItemSelection& /*oldSel*/);
    void selectLayerA();
    void selectLayerB();
    void saveEdit();
    void deleteCheck();
    void run();

private:
    QList<Check *> getChecks(CheckContext *context);
    void initPointOnLineUi();
    void initPointDuplicateUi();

};

#endif // CHECKITEMDIALOG_H
