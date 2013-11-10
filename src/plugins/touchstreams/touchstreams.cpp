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

#include "touchstreams.h"
#include <QIcon>
#include <util/queuemanager.h>
#include <util/util.h>
#include <util/svcauth/vkauthmanager.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "xmlsettingsmanager.h"
#include "audiosearch.h"
#include "albumsmanager.h"
#include "friendsmanager.h"

namespace LeechCraft
{
namespace TouchStreams
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;

		Util::InstallTranslator ("touchstreams");

		Queue_ = new Util::QueueManager (400);

		AuthMgr_ = new Util::SvcAuth::VkAuthManager ("TouchStreams",
				"3298289",
				{ "audio", "friends" },
				XmlSettingsManager::Instance ().property ("Cookies").toByteArray (),
				proxy,
				Queue_);
		connect (AuthMgr_,
				SIGNAL (cookiesChanged (QByteArray)),
				this,
				SLOT (saveCookies (QByteArray)));

		AlbumsMgr_ = new AlbumsManager (AuthMgr_, Queue_, proxy, this);
		FriendsMgr_ = new FriendsManager (AuthMgr_, Queue_, proxy, this);

		XSD_.reset (new Util::XmlSettingsDialog);
		XSD_->RegisterObject (&XmlSettingsManager::Instance (), "touchstreamssettings.xml");

		connect (XSD_.get (),
				SIGNAL (pushButtonClicked (QString)),
				this,
				SLOT (handlePushButton (QString)));
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.TouchStreams";
	}

	void Plugin::Release ()
	{
		delete Queue_;
	}

	QString Plugin::GetName () const
	{
		return "TouchStreams";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("VK.com music streamer.");
	}

	QIcon Plugin::GetIcon () const
	{
		return QIcon ();
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}

	QString Plugin::GetServiceName () const
	{
		return tr ("VKontakte");
	}

	QIcon Plugin::GetServiceIcon () const
	{
		static QIcon icon;
		return icon;
	}

	Media::IPendingAudioSearch* Plugin::Search (const Media::AudioSearchRequest& req)
	{
		auto realReq = req;
		if (realReq.FreeForm_.isEmpty ())
		{
			QStringList parts { req.Artist_, req.Album_, req.Title_ };
			parts.removeAll ({});
			realReq.FreeForm_ = parts.join (" - ");
		}

		return new AudioSearch (Proxy_, realReq, AuthMgr_, Queue_);
	}

	QList<QStandardItem*> Plugin::GetRadioListItems () const
	{
		return { AlbumsMgr_->GetRootItem (), FriendsMgr_->GetRootItem () };
	}

	Media::IRadioStation_ptr Plugin::GetRadioStation (QStandardItem*, const QString&)
	{
		return {};
	}

	void Plugin::RefreshItems (const QList<QStandardItem*>& items)
	{
		AlbumsMgr_->RefreshItems (items);
		FriendsMgr_->RefreshItems (items);
	}

	void Plugin::handlePushButton (const QString& name)
	{
		if (name == "AllowRequestsTriggered")
			AuthMgr_->reauth ();
		else
			qWarning () << Q_FUNC_INFO
					<< "unknown name"
					<< name;
	}

	void Plugin::saveCookies (const QByteArray& cookies)
	{
		XmlSettingsManager::Instance ().setProperty ("Cookies", cookies);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_touchstreams, LeechCraft::TouchStreams::Plugin);
