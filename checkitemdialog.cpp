#include "checkitemdialog.h"
#include "ui_checkitemdialog.h"
#include <QDebug>

CheckItemDialog::CheckItemDialog(QgisInterface *iface, CheckItem *item, QWidget *parent)
    : QDialog(parent), ui(new Ui::CheckItemDialog), iface(iface), mcheckItem(item)
{
    ui->setupUi(this);
    this->resize(900, 650);



    ui->textShortHelp->setAcceptRichText(true);
    ui->textShortHelp->setText(QStringLiteral("<html><body><span style=\" font-size:16pt; font-weight:700;\">线与线无相交</span><br><br><span style=\" font-size:13pt;\">检查线数据集中是否存在与参考线数据集的线相交的线对象，即两个线数据集中的所有线对象必须相互分离。</span><br><br></body></html>"));

    initTable();
    initParaTable();

}

CheckItemDialog::~CheckItemDialog()
{
    qDebug() << "CheckItemDialog::~CheckItemDialog()";
    delete ui;
}

void CheckItemDialog::set()
{
    ui->groupBoxPoint->setCollapsed(true);
    ui->groupBoxLine->setCollapsed(true);
    ui->groupBoxPolygon->setCollapsed(true);
    ui->groupBoxProperties->setCollapsed(true);
}

void CheckItemDialog::initTable()
{
    while (ui->tableWidget->rowCount() > 0)
    {
        ui->tableWidget->removeRow(0);
    }
    mCheckMap.clear();

    for (int i = 0; i < mcheckItem->sets.size(); ++i)
    {
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);

        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(mcheckItem->sets[i].name));
        mCheckMap[row] = &mcheckItem->sets[i];
    }
}

void CheckItemDialog::initParaTable()
{

    while (ui->tableWidgetPara->rowCount() > 0)
        ui->tableWidgetPara->removeRow(0);

    ui->tableWidgetPara->setRowCount(8);

    ui->tableWidgetPara->setItem(0, 0, new QTableWidgetItem(QStringLiteral("点图层")));
    layerA = new QPushButton(QStringLiteral("..."), this);
    ui->tableWidgetPara->setCellWidget(0, 1, layerA);
    connect(layerA, &QPushButton::clicked, this, &CheckItemDialog::selectLayerA);

    ui->tableWidgetPara->setItem(1, 0, new QTableWidgetItem(QStringLiteral("线图层")));
    layerB = new QPushButton(QStringLiteral("..."), this);
    ui->tableWidgetPara->setCellWidget(1, 1, layerB);
    connect(layerB, &QPushButton::clicked, this, &CheckItemDialog::selectLayerB);

    ui->tableWidgetPara->setItem(2, 0, new QTableWidgetItem(QStringLiteral("其他图层")));
    layerC = new QPushButton(QStringLiteral("..."), this);
    ui->tableWidgetPara->setCellWidget(2, 1, layerC);

    ui->tableWidgetPara->setItem(3, 0, new QTableWidgetItem(QStringLiteral("长度上限")));
    doubleSpinBoxMax = new QgsDoubleSpinBox(this);
    doubleSpinBoxMax->setDecimals(6);
    doubleSpinBoxMax->setMaximum(999999999.000000000000000);
    ui->tableWidgetPara->setCellWidget(3, 1, doubleSpinBoxMax);
    ui->tableWidgetPara->setItem(4, 0, new QTableWidgetItem(QStringLiteral("长度下限")));
    doubleSpinBoxMin = new QgsDoubleSpinBox(this);
    doubleSpinBoxMin->setDecimals(6);
    doubleSpinBoxMin->setMaximum(999999999.000000000000000);
    ui->tableWidgetPara->setCellWidget(4, 1, doubleSpinBoxMin);

    ui->tableWidgetPara->setItem(5, 0, new QTableWidgetItem(QStringLiteral("图层对应模式")));
    comboBoxLayerMode = new QComboBox(this);
    comboBoxLayerMode->addItem(QStringLiteral("一对一"));
    comboBoxLayerMode->addItem(QStringLiteral("一对多"));
    ui->tableWidgetPara->setCellWidget(5, 1, comboBoxLayerMode);
    connect(comboBoxLayerMode, &QComboBox::currentTextChanged, this, &CheckItemDialog::setLayerMode);


    ui->tableWidgetPara->setItem(6, 0, new QTableWidgetItem(QStringLiteral("角度")));
    doubleSpinBoxAngle = new QgsDoubleSpinBox(this);
    doubleSpinBoxAngle->setObjectName(QString::fromUtf8("doubleSpinBoxAngle"));
    doubleSpinBoxAngle->setDecimals(6);
    doubleSpinBoxAngle->setMaximum(360.000000000000000);
    ui->tableWidgetPara->setCellWidget(6, 1, doubleSpinBoxAngle);

    ui->tableWidgetPara->setItem(7, 0, new QTableWidgetItem(QStringLiteral("排除端点")));
    excludeEndpoint = new QCheckBox(this);
    excludeEndpoint->setObjectName(QString::fromUtf8("excludeEndpoint"));
    ui->tableWidgetPara->setCellWidget(7, 1, excludeEndpoint);


    for (int i = 0; i < ui->tableWidgetPara->rowCount(); ++i)
    {
        ui->tableWidgetPara->hideRow(i);
    }

}

