#include "checktask.h"

CheckTask::CheckTask(Checker *checker)
    : QgsTask(QStringLiteral("数据检查")),
      checker(checker)
{
}

bool CheckTask::run()
{
    // Run
    int maxSteps = 1;
    QEventLoop evLoop;
    QFutureWatcher<void> futureWatcher;

    connect( checker, &Checker::progressValue, this, [&](int value){
        this->setProgress(value*100.0/maxSteps);
    } );

    connect( &futureWatcher, &QFutureWatcherBase::finished, &evLoop, &QEventLoop::quit );

    futureWatcher.setFuture( checker->execute( &maxSteps ) );
    evLoop.exec();

    return true;
}
