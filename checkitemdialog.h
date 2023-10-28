#ifndef CHECKITEMDIALOG_H
#define CHECKITEMDIALOG_H

#include <QDialog>
#include <qgisinterface.h>
#include <qgsdoublespinbox.h>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
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
    void hideBtnList();

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
    QMap<QString, QPushButton*> nameToBtn;
    bool mIsRunningInBackground;

    void initTable();
    void initParaTable();
    void initConnection();
    void setBtnText(QPushButton* btn, QSet<QgsVectorLayer*> vec);
    void setLayerModeText();
    void setAttrText();
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
    QLineEdit *attr;


private slots:
    void onSelectionChanged(const QItemSelection& newSel, const QItemSelection& /*oldSel*/);
    void selectLayerA();
    void selectLayerB();
    void setLayerMode();
    void saveEdit();
    void deleteCheck();
    void run();

public:
    QList<Check *> getChecks(CheckContext *context);
    void initPointOnLineUi();
    void initPointDuplicateUi();

};

#endif // CHECKITEMDIALOG_H
