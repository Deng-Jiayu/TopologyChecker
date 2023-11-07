#include "messagebox.h"
#include "ui_messagebox.h"
#include <QPushButton>

MessageBox::MessageBox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MessageBox)
{
    ui->setupUi(this);

    this->setWindowTitle(QStringLiteral("参数设置"));

    connect(ui->btnCancel, &QPushButton::clicked, this, [&](){ this->hide(); });
    connect(ui->btnOK, &QPushButton::clicked, this, &MessageBox::run);
}

MessageBox::~MessageBox()
{
    delete ui;
}

bool MessageBox::selectedOnly()
{
    return ui->checkBox->isChecked();
}

double MessageBox::tolerance()
{
    return ui->spinBoxTolerance->value();
}
