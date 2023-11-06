#ifndef CHECKTASK_H
#define CHECKTASK_H

#include "qgstaskmanager.h"
#include "checker.h"

class CheckTask : public QgsTask
{
    Q_OBJECT
public:

    CheckTask( Checker * checker );

    void cancel() override;

signals:
    void canceled();

protected:

    bool run() override;
    void finished( bool result ) override;

private:

    Checker * checker;

};

#endif // CHECKTASK_H
