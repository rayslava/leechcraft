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

#include "enablesoundactionmanager.h"
#include <QAction>
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace AdvancedNotifications
{
	EnableSoundActionManager::EnableSoundActionManager (QObject *parent)
	: QObject (parent)
	, EnableAction_ (new QAction (tr ("Enable sound notifications"), this))
	{
		EnableAction_->setCheckable (true);
		EnableAction_->setProperty ("ActionIcon", "audio-volume-high");
		EnableAction_->setProperty ("ActionIconOff", "audio-volume-muted");
		EnableAction_->setProperty ("Action/ID", "org.LeechCraft.AdvancedNotifications.EnableSound");

		connect (EnableAction_,
				SIGNAL (toggled (bool)),
				this,
				SLOT (enableSounds (bool)));

		XmlSettingsManager::Instance ().RegisterObject ("EnableAudioNots",
				this, "xsdPropChanged");
		xsdPropChanged ();
	}

	QAction* EnableSoundActionManager::GetAction () const
	{
		return EnableAction_;
	}

	QList<QAction*> EnableSoundActionManager::GetActions (ActionsEmbedPlace aep) const
	{
		QList<QAction*> result;

		switch (aep)
		{
		case ActionsEmbedPlace::QuickLaunch:
		case ActionsEmbedPlace::TrayMenu:
			result << EnableAction_;
		default:
			break;
		}

		return result;
	}

	void EnableSoundActionManager::xsdPropChanged ()
	{
		EnableAction_->setChecked (XmlSettingsManager::Instance ()
				.property ("EnableAudioNots").toBool ());
	}

	void EnableSoundActionManager::enableSounds (bool enable)
	{
		if (enable != XmlSettingsManager::Instance ()
				.property ("EnableAudioNots").toBool ())
			XmlSettingsManager::Instance ().setProperty ("EnableAudioNots", enable);
	}
}
}
