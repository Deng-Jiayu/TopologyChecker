#ifndef CHECKTASK_H
#define CHECKTASK_H

#include "qgstaskmanager.h"
#include "checker.h"

class CheckTask : public QgsTask
{
    Q_OBJECT
public:

    CheckTask( Checker * checker );


protected:

    bool run() override;


private:

    Checker * checker;

};

#endif // CHECKTASK_H
