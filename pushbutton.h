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
    void addGroup();
    void addItem();
    void runAll();

private:
    QMenu *menu;

    QAction *acRemove;
    QAction *acAddGroup;
    QAction *acAddItem;
    QAction *acRename;
    QAction *acRun;
    QAction *acRunAll;

private slots:
    void ShowcustomContextMenu();
};

#endif // PUSHBUTTON_H
