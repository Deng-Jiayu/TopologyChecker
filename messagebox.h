#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QDialog>

namespace Ui {
class MessageBox;
}

class MessageBox : public QDialog
{
    Q_OBJECT

public:
    explicit MessageBox(QWidget *parent = nullptr);
    ~MessageBox();

    bool selectedOnly();
    double tolerance();

signals:
    void run();

private:
    Ui::MessageBox *ui;
};

#endif // MESSAGEBOX_H
