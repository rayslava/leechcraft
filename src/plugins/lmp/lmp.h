/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include <QObject>
#include <interfaces/iinfo.h>
#include <interfaces/ihavetabs.h>
#include <interfaces/ihavesettings.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/iactionsexporter.h>
#include <interfaces/ihaverecoverabletabs.h>
#include <interfaces/ipluginready.h>
#include <interfaces/ihaveshortcuts.h>
#include <interfaces/ijobholder.h>

namespace LeechCraft
{
namespace LMP
{
	class PlayerTab;

	class Plugin : public QObject
				 , public IInfo
				 , public IHaveTabs
				 , public IHaveSettings
				 , public IEntityHandler
				 , public IActionsExporter
				 , public IHaveRecoverableTabs
				 , public IHaveShortcuts
				 , public IPluginReady
				 , public IJobHolder
	{
		Q_OBJECT
		Q_INTERFACES (IInfo
				IHaveTabs
				IHaveSettings
				IEntityHandler
				IActionsExporter
				IHaveRecoverableTabs
				IHaveShortcuts
				IPluginReady
				IJobHolder)

		TabClassInfo PlayerTC_;
		TabClassInfo ArtistBrowserTC_;

		PlayerTab *PlayerTab_;

		Util::XmlSettingsDialog_ptr XSD_;

		QAction *ActionRescan_;
		QAction *ActionCollectionStats_;

		QMap<QString, Entity> GlobAction2Entity_;
		QMap<QString, ActionInfo> GlobAction2Info_;
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		QByteArray GetUniqueID () const;
		void Release ();
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;

		TabClasses_t GetTabClasses () const;
		void TabOpenRequested (const QByteArray&);

		Util::XmlSettingsDialog_ptr GetSettingsDialog () const;

		EntityTestHandleResult CouldHandle (const Entity&) const;
		void Handle (Entity);

		QList<QAction*> GetActions (ActionsEmbedPlace area) const;
		QMap<QString, QList<QAction*>> GetMenuActions () const;

		void RecoverTabs (const QList<TabRecoverInfo>& infos);

		void SetShortcut (const QString&, const QKeySequences_t&);
		QMap<QString, ActionInfo> GetActionInfo () const;

		QSet<QByteArray> GetExpectedPluginClasses () const;
		void AddPlugin (QObject* plugin);

		QAbstractItemModel* GetRepresentation () const;
	private:
		void InitShortcuts ();
	private slots:
		void handleFullRaiseRequested ();
		void showCollectionStats ();

		void handleArtistBrowseRequested (const QString&, const DynPropertiesList_t& = DynPropertiesList_t ());
	signals:
		void addNewTab (const QString&, QWidget*);
		void removeTab (QWidget*);
		void changeTabName (QWidget*, const QString&);
		void changeTabIcon (QWidget*, const QIcon&);
		void statusBarChanged (QWidget*, const QString&);
		void raiseTab (QWidget*);

		void gotActions (QList<QAction*>, LeechCraft::ActionsEmbedPlace);

		void gotEntity (const LeechCraft::Entity&);
	};
}
}
