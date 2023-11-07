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
    ui->verticalLayout->addWidget(mTabWidget);

    mTabWidget->addTab( new SetupTab(mIface, this, this), QStringLiteral("检查方案列表") );
    mTabWidget->addTab( new QWidget(), QStringLiteral("检查结果列表") );
    mTabWidget->setTabEnabled( 1, false );

    connect( dynamic_cast< SetupTab * >( mTabWidget->widget( 0 ) ), &SetupTab::checkerStarted, this, &CheckDock::onCheckerStarted );
    connect( dynamic_cast< SetupTab * >( mTabWidget->widget( 0 ) ), &SetupTab::checkerFinished, this, &CheckDock::onCheckerFinished );
}

CheckDock::~CheckDock()
{
    delete ui;
}
#include <QCloseEvent>
void CheckDock::closeEvent(QCloseEvent *ev)
{
    if ( SetupTab *setupTab = qobject_cast<SetupTab *>( mTabWidget->widget( 0 ) ) )
    {
        // do not allow closing the dialog - this would delete the geometry checker and crash
        if ( setupTab->mIsRunningInBackground )
        {
            ev->ignore();
            return;
        }
    }

    if ( qobject_cast<ResultTab *>( mTabWidget->widget( 1 ) ) &&
        !static_cast<ResultTab *>( mTabWidget->widget( 1 ) )->isCloseable() )
    {
        ev->ignore();
    }
    else
    {
        delete mTabWidget->widget( 1 );
        mTabWidget->removeTab( 1 );
        mTabWidget->addTab( new QWidget(), QStringLiteral( "检查结果列表" ) );
        mTabWidget->setTabEnabled( 1, false );

        QDockWidget::closeEvent( ev );
    }
}

void CheckDock::onCheckerStarted( Checker *checker )
{
    delete mTabWidget->widget( 1 );
    mTabWidget->removeTab( 1 );
    mTabWidget->addTab( new ResultTab( mIface, checker, mTabWidget ), QStringLiteral("检查结果列表") );
    mTabWidget->setTabEnabled( 1, false );
}

void CheckDock::onCheckerFinished( bool successful )
{
    if ( successful )
    {
        mTabWidget->setTabEnabled( 1, true );
        mTabWidget->setCurrentIndex( 1 );
        static_cast<ResultTab *>( mTabWidget->widget( 1 ) )->finalize();
    }
}
