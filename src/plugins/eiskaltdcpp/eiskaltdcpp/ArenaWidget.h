#ifndef ARENAWIDGET_H
#define ARENAWIDGET_H

#include <QWidget>
#include <QMenu>
#include <QAction>
#include <QPixmap>
#include <QToolBar>

#include <interfaces/imultitabs.h>

class ArenaWidget: public IMultiTabsWidget
{
    Q_INTERFACES(IMultiTabsWidget)
public:
    ArenaWidget();
    virtual ~ArenaWidget();

    virtual QWidget *getWidget() = 0;
    virtual QString getArenaTitle() = 0;
    virtual QString getArenaShortTitle() = 0;
    virtual QMenu *getMenu() = 0;
    virtual QAction *toolButton() { return toolBtn; }
    virtual void  setToolButton(QAction *btn) { if (btn) toolBtn = btn; }
    virtual const QPixmap &getPixmap(){ return _pxmap; }

    virtual void Remove () { /** Do nothing */ }
    virtual QToolBar* GetToolBar () const { return NULL; }
    virtual void NewTabRequested () { /** Do nothing */ }
    virtual QObject* ParentMultiTabs () const { return NULL; }
    virtual QList<QAction*> GetTabBarContextMenuActions () const { return QList<QAction*>(); }

    virtual void setUnload(bool b){ _arenaUnload = b; }
    virtual bool isUnload() const { return _arenaUnload; }

private:
    bool _arenaUnload;
    QAction *toolBtn;
    QPixmap _pxmap;
};

Q_DECLARE_INTERFACE (ArenaWidget, "org.negativ.EiskaltDCPP.ArenaWidget/1.0");

#endif // ARENAWIDGET_H
