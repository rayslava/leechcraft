/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Slava Barinov <rayslava@gmail.com>
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


#include "twitteruser.h"
#include "core.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Woodpecker
{
	TwitterUser::TwitterUser (QObject *parent)
	: QObject (parent)
	, Http_ (Core::Instance ().GetCoreProxy ()->GetNetworkAccessManager ())
	{
	}

	TwitterUser::TwitterUser (const qulonglong id, const QString& name,
				  QObject *parent)
	: QObject (parent)
	, Id_ (id)
	, Username_ (name)
	, Http_ (Core::Instance ().GetCoreProxy ()->GetNetworkAccessManager ())
	{
	}

	TwitterUser::TwitterUser (const TwitterUser& original)
	: QObject ()
	, Id_ (original.Id_)
	, Username_ (original.Username_)
	, Http_ (original.Http_)
	, Avatar_ (original.Avatar_)
	{
	}

	TwitterUser& TwitterUser::operator= (const TwitterUser& rhs)
	{
		if (this != &rhs)
		{
			Id_ = rhs.GetID ();
			Username_ = rhs.GetUsername ();
			Avatar_ = rhs.GetAvatar ();
		}
		return *this;
	}

	void TwitterUser::avatarDownloaded ()
	{
		QNetworkReply *reply = qobject_cast<QNetworkReply*> (sender ());
		if (!reply)
			return;

		if (reply->error ())
		{
			qWarning () << Q_FUNC_INFO << "Avatar downloading problem: " << reply->error ();
			reply->deleteLater ();
			return;
		}

		const auto& data = reply->readAll ();

		if (!data.isNull ())
		{
			Avatar_.loadFromData (data);
			emit userAvatarReady ();
		}
		reply->deleteLater ();
	}

	void TwitterUser::DownloadAvatar (const QString& path)
	{
		AvatarUrl_.setUrl(path);
		qDebug () << Q_FUNC_INFO << "Downloading avatar from: " << AvatarUrl_;
		auto reply = Http_->get (QNetworkRequest (AvatarUrl_));
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (avatarDownloaded ()));
	}

	QString TwitterUser::GetUsername () const
	{
		return Username_;
	}

	QPixmap TwitterUser::GetAvatar () const
	{
		return Avatar_;
	}

	QUrl TwitterUser::GetAvatarUrl() const
	{
		return AvatarUrl_;
	}

	qulonglong TwitterUser::GetID() const
	{
		return Id_;
	}
}
}
}
