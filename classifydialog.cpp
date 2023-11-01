#include "classifydialog.h"
#include "ui_classifydialog.h"

ClassifyDialog::ClassifyDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ErrorTypeDialog)
{
    ui->setupUi(this);

    this->setWindowTitle(QStringLiteral("列表"));

    connect(ui->btnSelectAll, &QPushButton::clicked, this, &ClassifyDialog::selectAll);
    connect(ui->btnDeselectAll, &QPushButton::clicked, this, &ClassifyDialog::deselectAll);
    connect(ui->btnOK, &QPushButton::clicked, this, &ClassifyDialog::ClassifyDone);
    connect(ui->btnCancel, &QPushButton::clicked, this, [&]()
            { this->hide(); });
}

ClassifyDialog::~ClassifyDialog()
{
    delete ui;
}

QStringList ClassifyDialog::selectedType()
{
    QStringList ans;
    for (int row = 0, nRows = ui->listWidget->count(); row < nRows; ++row)
    {
        QListWidgetItem *item = ui->listWidget->item(row);
        if (item->checkState() == Qt::Checked)
        {
            ans.append(item->text());
        }
    }
    return ans;
}

void ClassifyDialog::initList(QList<Check *> checks)
{
    ui->listWidget->clear();
    this->types.clear();

    for (auto it : checks)
        types.insert(it->description());

    for (auto &it : std::as_const(types))
    {
        QListWidgetItem *item = new QListWidgetItem(it);
        item->setCheckState(Qt::Checked);
        ui->listWidget->addItem(item);
    }
}

void ClassifyDialog::selectAll()
{
    for (int row = 0, nRows = ui->listWidget->count(); row < nRows; ++row)
    {
        QListWidgetItem *item = ui->listWidget->item(row);
        item->setCheckState(Qt::Checked);
    }
}

void ClassifyDialog::deselectAll()
{
    for (int row = 0, nRows = ui->listWidget->count(); row < nRows; ++row)
    {
        QListWidgetItem *item = ui->listWidget->item(row);
        item->setCheckState(Qt::Unchecked);
    }
}
