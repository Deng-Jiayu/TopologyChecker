#include "checkitemdialog.h"
#include "ui_checkitemdialog.h"
#include <QDebug>
#include <QTimer>

CheckItemDialog::CheckItemDialog(CheckItem *item, QWidget *parent)
    : QDialog(parent), ui(new Ui::CheckItemDialog)
{
    ui->setupUi(this);


    this->item = item;
}

CheckItemDialog::~CheckItemDialog()
{
    qDebug()<<"CheckItemDialog::~CheckItemDialog()";
    delete ui;
}

void CheckItemDialog::set()
{
    //ui->groupBoxGeometry->setCollapsed(true);
    //ui->groupBoxGeometryProperties->setCollapsed(true);
}
