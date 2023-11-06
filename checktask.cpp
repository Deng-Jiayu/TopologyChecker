#include "checktask.h"

CheckTask::CheckTask(Checker *checker)
    : QgsTask(QStringLiteral("数据检查"), QgsTask::CanCancel),
      checker(checker)
{
}

void CheckTask::cancel()
{
    emit canceled();
    QgsTask::cancel();
}

bool CheckTask::run()
{
    qDebug() << "run";
    // Run
    int maxSteps = 1;
    QEventLoop evLoop;
    QFutureWatcher<void> futureWatcher;
    //connect( checker, &Checker::progressValue, this, &CheckTask::setProgress );

    connect( checker, &Checker::progressValue, this, [&](int value){
        qDebug() << value;
        this->setProgress(value*100.0/maxSteps);
    } );


    connect( &futureWatcher, &QFutureWatcherBase::finished, &evLoop, &QEventLoop::quit );
    connect( this, &CheckTask::canceled, &futureWatcher, &QFutureWatcherBase::cancel );


    futureWatcher.setFuture( checker->execute( &maxSteps ) );
    evLoop.exec();

    qDebug() << "maxSteps"<< maxSteps;

    return true;
}

void CheckTask::finished( bool result )
{

}
