#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMenu>

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);

Q_SIGNALS:
    void addGroup();
    void runAll();

private:
    QMenu *menu;
    QAction *acAddGroup;
    QAction *acRunAll;

private slots:
    void ShowcustomContextMenu();
};

#endif // WIDGET_H
