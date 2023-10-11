#include "checkdock.h"
#include "ui_checkdock.h"
#include "setuptab.h"
#include "resulttab.h"

CheckDock::CheckDock(QgisInterface *iface, QWidget *parent)
    : QDockWidget(parent), ui(new Ui::CheckDock)
{
    mIface = iface;
    ui->setupUi(this);

    mTabWidget = new QTabWidget();
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Help, Qt::Horizontal);

    ui->verticalLayout->addWidget(mTabWidget);

    mTabWidget->addTab(new SetupTab(mIface, this, this), QStringLiteral("检查方案列表"));
    mTabWidget->addTab(new ResultTab(this), QStringLiteral("检查结果列表"));
}

CheckDock::~CheckDock()
{
    delete ui;
}
