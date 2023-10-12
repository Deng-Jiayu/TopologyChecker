#ifndef PUSHBUTTON_H
#define PUSHBUTTON_H

#include <QPushButton>

class PushButton : public QPushButton
{
    Q_OBJECT
public:
    PushButton(QWidget *parent = nullptr);

signals:
    void run();
    void runSelected();
    void rename();
    void remove();
    void add();
    void runAll();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QMenu *menu;

    QAction *acRemove;
    QAction *acAdd;
    QAction *acRename;
    QAction *acRun;
    QAction *acRunSelected;
    QAction *acRunAll;
};

#endif // PUSHBUTTON_H
