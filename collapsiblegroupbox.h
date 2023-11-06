#ifndef COLLAPSIBLEGROUPBOX_H
#define COLLAPSIBLEGROUPBOX_H

#include <QGroupBox>
#include <QPointer>
#include <QToolButton>

#include "qgis_sip.h"
#include "qgssettings.h"

class QMouseEvent;
class QToolButton;
class QScrollArea;


class GroupBoxCollapseButton : public QToolButton
{
    Q_OBJECT

public:
    GroupBoxCollapseButton( QWidget *parent SIP_TRANSFERTHIS = nullptr )
        : QToolButton( parent )
    {}

    bool altDown() const { return mAltDown; }
    void setAltDown( bool updown ) { mAltDown = updown; }

    bool shiftDown() const { return mShiftDown; }
    void setShiftDown( bool shiftdown ) { mShiftDown = shiftdown; }

protected:
    void mouseReleaseEvent( QMouseEvent *event ) override;

private:
    bool mAltDown = false;
    bool mShiftDown = false;
};

class CollapsibleGroupBoxBasic : public QGroupBox
{
    Q_OBJECT

    /**
     * The collapsed state of this group box. If it is set to TRUE, all content is hidden
     * if it is set to FALSE all content is shown.
     */
    Q_PROPERTY( bool collapsed READ isCollapsed WRITE setCollapsed USER true )

    /**
     * An optional group to be collapsed and uncollapsed in sync with this group box if the Alt-modifier
     * is pressed while collapsing / uncollapsing.
     */
    Q_PROPERTY( QString syncGroup READ syncGroup WRITE setSyncGroup )

    /**
     * If this property is set to TRUE, a parent scroll area will try to make sure that the whole
     * group box is visible when uncollapsing it.
     */
    Q_PROPERTY( bool scrollOnExpand READ scrollOnExpand WRITE setScrollOnExpand )

public:
    CollapsibleGroupBoxBasic( QWidget *parent SIP_TRANSFERTHIS = nullptr );
    CollapsibleGroupBoxBasic( const QString &title, QWidget *parent SIP_TRANSFERTHIS = nullptr );

    /**
     * Returns the current collapsed state of this group box
     */
    bool isCollapsed() const { return mCollapsed; }

    /**
     * Collapse or uncollapse this groupbox
     *
     * \param collapse Will collapse on TRUE and uncollapse on FALSE
     */
    void setCollapsed( bool collapse );

    /**
     * Named group which synchronizes collapsing action when triangle is clicked while holding alt modifier key
     */
    QString syncGroup() const { return mSyncGroup; }

    /**
     * Named group which synchronizes collapsing action when triangle is clicked while holding alt modifier key
     */
    void setSyncGroup( const QString &grp );

    //! Sets this to FALSE to not automatically scroll parent QScrollArea to this widget's contents when expanded
    void setScrollOnExpand( bool scroll ) { mScrollOnExpand = scroll; }

    //! If this is set to FALSE the parent QScrollArea will not be automatically scrolled to this widget's contents when expanded
    bool scrollOnExpand() {return mScrollOnExpand;}

signals:
    //! Signal emitted when groupbox collapsed/expanded state is changed, and when first shown
    void collapsedStateChanged( bool collapsed );

public slots:
    void checkToggled( bool ckd );
    void checkClicked( bool ckd );
    void toggleCollapsed();

protected:
    void init();

    //! Visual fixes for when group box is collapsed/expanded
    void collapseExpandFixes();

    void showEvent( QShowEvent *event ) override;
    void mousePressEvent( QMouseEvent *event ) override;
    void mouseReleaseEvent( QMouseEvent *event ) override;
    void changeEvent( QEvent *event ) override;

    void updateStyle();
    QRect titleRect() const;
    void clearModifiers();

    bool mCollapsed;
    bool mInitFlat;
    bool mInitFlatChecked;
    bool mScrollOnExpand;
    bool mShown;
    QScrollArea *mParentScrollArea = nullptr;
    GroupBoxCollapseButton *mCollapseButton = nullptr;
    QWidget *mSyncParent = nullptr;
    QString mSyncGroup;
    bool mAltDown;
    bool mShiftDown;
    bool mTitleClicked;

    QIcon mCollapseIcon;
    QIcon mExpandIcon;
};

class CollapsibleGroupBox : public CollapsibleGroupBoxBasic
{
    Q_OBJECT

    /**
     * Shall the collapsed state of this group box be saved and loaded persistently in QgsSettings
     */
    Q_PROPERTY( bool saveCollapsedState READ saveCollapsedState WRITE setSaveCollapsedState )

    /**
     * Shall the checked state of this group box be saved and loaded persistently in QgsSettings
     */
    Q_PROPERTY( bool saveCheckedState READ saveCheckedState WRITE setSaveCheckedState )

public:
    CollapsibleGroupBox( QWidget *parent SIP_TRANSFERTHIS = nullptr, QgsSettings *settings = nullptr );
    CollapsibleGroupBox( const QString &title, QWidget *parent SIP_TRANSFERTHIS = nullptr, QgsSettings *settings = nullptr );
    ~CollapsibleGroupBox() override;

    // set custom QgsSettings pointer if group box was already created from QtDesigner promotion
    void setSettings( QgsSettings *settings );

    //! Sets this to FALSE to not save/restore collapsed state
    void setSaveCollapsedState( bool save ) { mSaveCollapsedState = save; }

    /**
     * Set this to TRUE to save/restore checked state
     * \note only turn on mSaveCheckedState for groupboxes NOT used
     * in multiple places or used as options for different parent objects
    */
    void setSaveCheckedState( bool save ) { mSaveCheckedState = save; }
    bool saveCollapsedState() { return mSaveCollapsedState; }
    bool saveCheckedState() { return mSaveCheckedState; }

    //! Sets this to a defined string to share save/restore states across different parent dialogs
    void setSettingGroup( const QString &group ) { mSettingGroup = group; }
    //! Returns the name of the setting group in which the collapsed state will be saved
    QString settingGroup() const { return mSettingGroup; }

signals:
    void rename();
    void remove();
    void addGroup();
    void addItem();
    void runAll();

protected slots:

    /**
     * Will load the collapsed and checked state
     *
     * The configuration path from which it is loaded is defined by
     *
     * - The object name
     * - The settingGroup
     */
    void loadState();

    /**
     * Will save the collapsed and checked state
     *
     * The configuration path to which it is saved is defined by
     *
     * - The object name
     * - The settingGroup
     */
    void saveState() const;

protected:
    void init();
    void showEvent( QShowEvent *event ) override;
    QString saveKey() const;

    // pointer to app or custom, external QgsSettings
    // QPointer in case custom settings obj gets deleted while groupbox's dialog is open
    QPointer<QgsSettings> mSettings;
    bool mDelSettings;

    bool mSaveCollapsedState;
    bool mSaveCheckedState;
    QString mSettingGroup;

private:
    QMenu *menu;

    QAction *acRemove;
    QAction *acRename;
    QAction *acAddGroup;
    QAction *acAddItem;
    QAction *acRunAll;

private slots:
    void ShowcustomContextMenu();

};

#endif // COLLAPSIBLEGROUPBOX_H
