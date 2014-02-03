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

#ifndef COREINSTANCEOBJECT_H
#define COREINSTANCEOBJECT_H
#include <QObject>
#include "interfaces/iinfo.h"
#include "interfaces/ihavesettings.h"
#include "interfaces/ihavetabs.h"
#include "interfaces/ipluginready.h"
#include "interfaces/ihaveshortcuts.h"

class IShortcutProxy;

namespace LeechCraft
{
	class SettingsTab;
	class CorePlugin2Manager;
	class ShortcutManager;

	namespace Util
	{
		class ShortcutManager;
	}

	class CoreInstanceObject : public QObject
							 , public IInfo
							 , public IHaveSettings
							 , public IHaveTabs
							 , public IHaveShortcuts
							 , public IPluginReady
	{
		Q_OBJECT
		Q_INTERFACES (IInfo IHaveSettings IHaveTabs IHaveShortcuts IPluginReady)

		Util::XmlSettingsDialog_ptr XmlSettingsDialog_;
		TabClasses_t Classes_;

		SettingsTab *SettingsTab_;

		CorePlugin2Manager *CorePlugin2Manager_;

		ShortcutManager *ShortcutManager_;

		Util::ShortcutManager *CoreShortcutManager_;
	public:
		CoreInstanceObject (QObject* = 0);

		// IInfo
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		void Release ();
		QByteArray GetUniqueID () const;
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;

		// IHaveSettings
		Util::XmlSettingsDialog_ptr GetSettingsDialog () const;

		// IHaveTabs
		TabClasses_t GetTabClasses () const;
		void TabOpenRequested (const QByteArray&);

		// IHaveShortcuts
		QMap<QString, ActionInfo> GetActionInfo () const;
		void SetShortcut (const QString& id, const QKeySequences_t& sequences);

		// IPluginReady
		QSet<QByteArray> GetExpectedPluginClasses () const;
		void AddPlugin (QObject*);

		CorePlugin2Manager* GetCorePluginManager () const;

		SettingsTab* GetSettingsTab () const;

		IShortcutProxy* GetShortcutProxy () const;
		ShortcutManager* GetShortcutManager () const;
		Util::ShortcutManager* GetCoreShortcutManager () const;
	private:
		void BuildNewTabModel ();
	private slots:
		void handleSettingsButton (const QString&);
		void updateIconSet ();
		void updateColorTheme ();
#ifdef STRICT_LICENSING
		void notifyLicensing ();
#endif
	signals:
		void addNewTab (const QString&, QWidget*);
		void removeTab (QWidget*);
		void changeTabName (QWidget*, const QString&);
		void changeTabIcon (QWidget*, const QIcon&);
		void statusBarChanged (QWidget*, const QString&);
		void raiseTab (QWidget*);

		void gotEntity (const LeechCraft::Entity&);
	};
}

#endif
