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

#include "vkauthmanager.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtDebug>
#include <QTimer>
#include <QWebView>
#include <util/network/customcookiejar.h>
#include <util/queuemanager.h>

namespace LeechCraft
{
namespace Util
{
namespace SvcAuth
{
	namespace
	{
		QUrl URLFromClientID (const QString& id, const QStringList& scope)
		{
			QUrl url = QUrl::fromEncoded ("https://oauth.vk.com/authorize?redirect_uri=http%3A%2F%2Foauth.vk.com%2Fblank.html&response_type=token&state=");
			url.addQueryItem ("client_id", id);
			url.addQueryItem ("scope", scope.join (","));
			return url;
		}
	}

	VkAuthManager::VkAuthManager (const QString& id, const QStringList& scope,
			const QByteArray& cookies, ICoreProxy_ptr proxy,
			QueueManager *queueMgr, QObject *parent)
	: QObject (parent)
	, Proxy_ (proxy)
	, AuthNAM_ (new QNetworkAccessManager (this))
	, Cookies_ (new Util::CustomCookieJar)
	, Queue_ (queueMgr)
	, ValidFor_ (0)
	, IsRequesting_ (false)
	, URL_ (URLFromClientID (id, scope))
	, IsRequestScheduled_ (false)
	, ScheduleTimer_ (new QTimer (this))
	{
		AuthNAM_->setCookieJar (Cookies_);
		Cookies_->Load (cookies);

		ScheduleTimer_->setSingleShot (true);
		connect (ScheduleTimer_,
				SIGNAL (timeout ()),
				this,
				SLOT (execScheduledRequest ()));
	}

	void VkAuthManager::GetAuthKey ()
	{
		if (Token_.isEmpty () ||
				ReceivedAt_.secsTo (QDateTime::currentDateTime ()) > ValidFor_)
		{
			RequestAuthKey ();
			return;
		}

		InvokeQueues (Token_);
		emit gotAuthKey (Token_);
	}

	void VkAuthManager::Reauth ()
	{
		auto view = new QWebView;
		view->setWindowTitle (tr ("VK.com authentication"));
		view->setWindowFlags (Qt::Window);
		view->resize (800, 600);
		view->page ()->setNetworkAccessManager (AuthNAM_);
		view->show ();

		view->setUrl (URL_);

		connect (view,
				SIGNAL (urlChanged (QUrl)),
				this,
				SLOT (handleViewUrlChanged (QUrl)));
	}

	void VkAuthManager::ManageQueue (VkAuthManager::RequestQueue_ptr queue)
	{
		if (!Queue_)
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot manage request queue if queue manager wasn't set";
			return;
		}

		ManagedQueues_ << queue;
	}

	void VkAuthManager::UnmanageQueue (VkAuthManager::RequestQueue_ptr queue)
	{
		ManagedQueues_.removeAll (queue);
	}

	void VkAuthManager::ManageQueue (VkAuthManager::PrioRequestQueue_ptr queue)
	{
		if (!Queue_)
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot manage request queue if queue manager wasn't set";
			return;
		}

		PrioManagedQueues_ << queue;
	}

	void VkAuthManager::UnmanageQueue (VkAuthManager::PrioRequestQueue_ptr queue)
	{
		PrioManagedQueues_.removeAll (queue);
	}

	void VkAuthManager::InvokeQueues (const QString& token)
	{
		for (auto queue : PrioManagedQueues_)
			while (!queue->isEmpty ())
			{
				const auto& pair = queue->takeFirst ();
				const auto& f = pair.first;
				Queue_->Schedule ([f, token] { f (token); }, nullptr, pair.second);
			}

		for (auto queue : ManagedQueues_)
			while (!queue->isEmpty ())
			{
				const auto& f = queue->takeFirst ();
				Queue_->Schedule ([f, token] { f (token); });
			}
	}

	void VkAuthManager::HandleError ()
	{
		IsRequesting_ = false;
	}

	void VkAuthManager::RequestURL (const QUrl& url)
	{
		qDebug () << Q_FUNC_INFO << url;
		auto reply = AuthNAM_->get (QNetworkRequest (url));
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleGotForm ()));
	}

	void VkAuthManager::RequestAuthKey ()
	{
		if (IsRequestScheduled_ && ScheduleTimer_->isActive ())
			ScheduleTimer_->stop ();

		if (IsRequesting_)
			return;

		RequestURL (URL_);
		IsRequesting_ = true;
	}

	bool VkAuthManager::CheckIsBlank (QUrl location)
	{
		if (location.path () != "/blank.html")
			return false;

		location = QUrl::fromEncoded (location.toEncoded ().replace ('#', '?'));
		Token_ = location.queryItemValue ("access_token");
		ValidFor_ = location.queryItemValue ("expires_in").toInt ();
		ReceivedAt_ = QDateTime::currentDateTime ();
		qDebug () << Q_FUNC_INFO << Token_ << ValidFor_;
		IsRequesting_ = false;

		InvokeQueues (Token_);
		emit gotAuthKey (Token_);

		return true;
	}

	void VkAuthManager::execScheduledRequest ()
	{
		IsRequestScheduled_ = false;

		RequestAuthKey ();
	}

	void VkAuthManager::handleGotForm ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		if (reply->error () != QNetworkReply::NoError)
		{
			qWarning () << Q_FUNC_INFO
					<< reply->errorString ();

			IsRequesting_ = false;

			if (!IsRequestScheduled_)
			{
				IsRequestScheduled_ = true;
				ScheduleTimer_->start (30000);
			}

			return;
		}

		const auto& location = reply->header (QNetworkRequest::LocationHeader).toUrl ();
		if (location.isEmpty ())
		{
			Reauth ();
			return;
		}

		if (CheckIsBlank (location))
			return;

		RequestURL (location);
	}

	void VkAuthManager::handleViewUrlChanged (const QUrl& url)
	{
		if (!CheckIsBlank (url))
			return;

		emit cookiesChanged (Cookies_->Save ());
		sender ()->deleteLater ();
	}
}
}
}
