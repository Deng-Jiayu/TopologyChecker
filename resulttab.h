#ifndef RESULTTAB_H
#define RESULTTAB_H

#include <QWidget>
#include <QTabWidget>
#include <qgisinterface.h>
#include <qgsrubberband.h>
#include "checker.h"
#include "classifydialog.h"
#include <QItemSelection>
#include <QKeyEvent>

namespace Ui {
class ResultTab;
}

class ResultTab : public QWidget
{
    Q_OBJECT

public:
    ResultTab(QgisInterface *iface, Checker *checker, QTabWidget *tabWidget, QWidget *parent = nullptr);
    ~ResultTab();
    void finalize();
    bool isCloseable() const { return mCloseable; }

    static QString sSettingsGroup;

protected:
    void keyReleaseEvent(QKeyEvent *event);

private:
    Ui::ResultTab *ui;

    QTabWidget *mTabWidget = nullptr;
    QgisInterface *mIface = nullptr;
    ClassifyDialog *mClassifyDialog = nullptr;
    Checker *mChecker = nullptr;
    QList<QgsRubberBand *> mCurrentRubberBands;
    QMap<CheckError *, QPersistentModelIndex> mErrorMap;
    QMap<QString, QPointer<QDialog>> mAttribTableDialogs;
    int mErrorCount;
    int mFixedCount;

    bool mCloseable = true;

    void setRowStatus( int row, const QColor &color, const QString &message, bool selectable );
    bool exportErrorsDo( const QString &file );
    void updateComboBox();

    bool enableKeyboard = false;
    int mCurrentRow = -1;

private slots:
    void addError( CheckError *error );
    void updateError( CheckError *error, bool statusChanged );
    void onSelectionChanged( const QItemSelection &newSel, const QItemSelection & /*oldSel*/ );
    void highlightErrors( bool current = false );
    void openAttributeTable();
    void checkRemovedLayer( const QStringList &ids );
    void exportErrors();
    void fixCurrentError();
    void fixErrorsWithDefault();
    void setDefaultResolutionMethods();
    void storeDefaultResolutionMethod(int) const;
    void classify();
    void doClassify();
    void switchByKey();

};

#endif // RESULTTAB_H
