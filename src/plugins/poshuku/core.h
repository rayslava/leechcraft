/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#pragma once

#include <memory>
#include <vector>
#include <QObject>
#include <QUrl>
#include <QTimer>
#include <util/tags/tagscompletionmodel.h>
#include <interfaces/structures.h>
#include <interfaces/ihavetabs.h>
#include <interfaces/ihaveshortcuts.h>
#include <interfaces/ihaverecoverabletabs.h>
#include "favoritesmodel.h"
#include "historymodel.h"
#include "storagebackend.h"
#include "urlcompletionmodel.h"
#include "pluginmanager.h"
#include "browserwidgetsettings.h"

class QString;
class QWidget;
class QIcon;
class QWebView;
class QAbstractItemModel;
class QNetworkReply;
class QNetworkAccessManager;
class IWebWidget;
class IShortcutProxy;

namespace LeechCraft
{
namespace Poshuku
{
	class CustomWebView;
	class BrowserWidget;
	class WebPluginFactory;

	class Core : public QObject
	{
		Q_OBJECT

		typedef std::vector<BrowserWidget*> widgets_t;
		widgets_t Widgets_;

		PluginManager * const PluginManager_;
		URLCompletionModel * const URLCompletionModel_;
		HistoryModel * const HistoryModel_;
		FavoritesModel * const FavoritesModel_;

		std::shared_ptr<StorageBackend> StorageBackend_;
		QNetworkAccessManager *NetworkAccessManager_ = nullptr;
		WebPluginFactory *WebPluginFactory_ = nullptr;

		IShortcutProxy *ShortcutProxy_ = nullptr;

		ICoreProxy_ptr Proxy_;

		bool Initialized_ = false;

		TabClassInfo TabClass_;

		Core ();
	public:
		enum FormRememberType
		{
			FRTRemember_,
			FRTNotNow_,
			FRTNever_
		};

		struct UncloseData
		{
			QUrl URL_;
			QPoint SPoint_;
			QByteArray History_;
		};

		static Core& Instance ();
		void Init ();
		void SecondInit ();
		void Release ();
		void SetProxy (ICoreProxy_ptr);
		ICoreProxy_ptr GetProxy () const;
		TabClassInfo GetTabClass () const;

		bool CouldHandle (const Entity&) const;
		void Handle (Entity);

		WebPluginFactory* GetWebPluginFactory ();

		QSet<QByteArray> GetExpectedPluginClasses () const;
		void AddPlugin (QObject*);

		QUrl MakeURL (QString);
		BrowserWidget* NewURL (const QUrl&, bool = false, const DynPropertiesList_t& = DynPropertiesList_t ());
		BrowserWidget* NewURL (const QString&, bool = false);
		IWebWidget* GetWidget ();
		CustomWebView* MakeWebView (bool = false);
		void Unregister (BrowserWidget*);
		/** Sets up the connections between widget's signals
			* and our signals/slots only useful in own mode.
			*
			* Calls to SetupConnections internally as well.
			*/
		void ConnectSignals (BrowserWidget *widget);

		void CheckFavorites ();
		void ReloadAll ();

		FavoritesModel* GetFavoritesModel () const;
		HistoryModel* GetHistoryModel () const;
		URLCompletionModel* GetURLCompletionModel () const;
		QNetworkAccessManager* GetNetworkAccessManager () const;
		StorageBackend* GetStorageBackend () const;
		PluginManager* GetPluginManager () const;
		void SetShortcut (const QString& name, const QKeySequences_t& shortcut);
		IShortcutProxy* GetShortcutProxy () const;

		QIcon GetIcon (const QUrl&) const;
		QString GetUserAgent (const QUrl&, const QWebPage* = 0) const;

		bool IsUrlInFavourites (const QString&);
		void RemoveFromFavorites (const QString&);
	private:
		void HandleHistory (CustomWebView*);
		/** Sets up the connections between widget's signals
			* and our signals/slots that are always useful, both in own
			* and deown mode.
			*/
		void SetupConnections (BrowserWidget *widget);
		void HandleSearchRequest (const QString&);
	public slots:
		void importXbel ();
		void exportXbel ();
	private slots:
		void handleTitleChanged (const QString&);
		void handleURLChanged (const QString&);
		void handleIconChanged (const QIcon&);
		void handleNeedToClose ();
		void handleAddToFavorites (QString, QString);
		void handleStatusBarChanged (const QString&);
		void handleTooltipChanged (QWidget*);
		void favoriteTagsUpdated (const QStringList&);
	signals:
		void addNewTab (const QString&, QWidget*);
		void removeTab (QWidget*);
		void changeTabName (QWidget*, const QString&);
		void changeTabIcon (QWidget*, const QIcon&);
		void changeTooltip (QWidget*, QWidget*);
		void raiseTab (QWidget*);
		void error (const QString&) const;
		void statusBarChanged (QWidget*, const QString&);
		void gotEntity (const LeechCraft::Entity&);
		void delegateEntity (const LeechCraft::Entity&, int*, QObject**);
		void couldHandle (const LeechCraft::Entity&, bool*);
		void bookmarkAdded (const QString&);
		void bookmarkRemoved (const QString&);
		void browserWidgetCreated (BrowserWidget*);

		// Hook support signals
		void hookAddToFavoritesRequested (LeechCraft::IHookProxy_ptr,
				QString title, QString url);
		void hookIconRequested (LeechCraft::IHookProxy_ptr,
				const QUrl& url) const;
		void hookTabAdded (LeechCraft::IHookProxy_ptr,
				QObject *browserWidget,
				QWebView *view,
				const QUrl& url);
		void hookUserAgentForUrlRequested (LeechCraft::IHookProxy_ptr,
				const QUrl&, const QWebPage*) const;
	};
}
}

Q_DECLARE_METATYPE (LeechCraft::Poshuku::Core::UncloseData);
