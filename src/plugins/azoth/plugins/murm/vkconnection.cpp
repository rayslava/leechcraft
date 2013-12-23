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

#include "vkconnection.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtDebug>
#include <QTimer>
#include <qjson/parser.h>
#include <util/svcauth/vkauthmanager.h>
#include <util/queuemanager.h>
#include "longpollmanager.h"
#include "logger.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Murm
{
	namespace
	{
		const QString UserFields = "first_name,last_name,nickname,photo,photo_big,sex,"
				"bdate,city,country,timezone,contacts,education";
	}

	VkConnection::VkConnection (const QString& name,
			const QByteArray& cookies, ICoreProxy_ptr proxy, Logger& logger)
	: AuthMgr_ (new Util::SvcAuth::VkAuthManager (name, "3778319",
			{ "messages", "notifications", "friends", "status", "photos" },
			cookies, proxy, nullptr, this))
	, Proxy_ (proxy)
	, Logger_ (logger)
	, LastCookies_ (cookies)
	, CallQueue_ (new Util::QueueManager (400))
	, LPManager_ (new LongPollManager (this, proxy))
	, MarkOnlineTimer_ (new QTimer (this))
	{
		Logger_ << "==========================================";
		connect (AuthMgr_,
				SIGNAL (cookiesChanged (QByteArray)),
				this,
				SLOT (saveCookies (QByteArray)));
		connect (AuthMgr_,
				SIGNAL (gotAuthKey (QString)),
				this,
				SLOT (callWithKey (QString)));

		connect (LPManager_,
				SIGNAL (listening ()),
				this,
				SLOT (handleListening ()));
		connect (LPManager_,
				SIGNAL (stopped ()),
				this,
				SLOT (handlePollStopped ()));
		connect (LPManager_,
				SIGNAL (pollError ()),
				this,
				SLOT (handlePollError ()));
		connect (LPManager_,
				SIGNAL (gotPollData (QVariantMap)),
				this,
				SLOT (handlePollData (QVariantMap)));

		Dispatcher_ [1] = [this] (const QVariantList&) {};
		Dispatcher_ [2] = [this] (const QVariantList&) {};
		Dispatcher_ [3] = [this] (const QVariantList&) {};

		Dispatcher_ [4] = [this] (const QVariantList& items)
		{
			emit gotMessage ({
					items.value (1).toULongLong (),
					items.value (3).toULongLong (),
					items.value (6).toString (),
					MessageFlags (items.value (2).toInt ()),
					QDateTime::fromTime_t (items.value (4).toULongLong ()),
					items.value (7).toMap ()
				});
		};
		Dispatcher_ [8] = [this] (const QVariantList& items)
			{ emit userStateChanged (items.value (1).toLongLong () * -1, true); };
		Dispatcher_ [9] = [this] (const QVariantList& items)
			{ emit userStateChanged (items.value (1).toLongLong () * -1, false); };
		Dispatcher_ [61] = [this] (const QVariantList& items)
			{ emit gotTypingNotification (items.value (1).toULongLong ()); };

		Dispatcher_ [101] = [this] (const QVariantList&) {};	// unknown stuff

		MarkOnlineTimer_->setInterval (12 * 60 * 1000);
		connect (MarkOnlineTimer_,
				SIGNAL (timeout ()),
				this,
				SLOT (markOnline ()));
	}

	const QByteArray& VkConnection::GetCookies () const
	{
		return LastCookies_;
	}

	void VkConnection::RerequestFriends ()
	{
		Logger_ (IHaveConsole::PacketDirection::Out) << "RerequestFriends";
		PushFriendsRequest ();
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::SendMessage (qulonglong to, const QString& body,
			std::function<void (qulonglong)> idSetter, MessageType type)
	{
		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([=] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/messages.send");

				auto query = "access_token=" + QUrl::toPercentEncoding (key.toUtf8 ());
				query += '&';
				query += type == MessageType::Dialog ? "uid" : "chat_id";
				query += '=' + QByteArray::number (to);
				query += "&type=1&";
				query += "message=" + QUrl::toPercentEncoding (body, {}, "+");

				for (auto i = params.begin (); i != params.end (); ++i)
					query += "&" + QUrl::toPercentEncoding (i.key ()) +
							"=" + QUrl::toPercentEncoding (i.value ());

				QNetworkRequest req (url);
				req.setHeader (QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
				auto reply = nam->post (req, query);
				Logger_ (IHaveConsole::PacketDirection::Out)
						<< url
						<< " : posting"
						<< QString::fromUtf8 (query);
				MsgReply2Setter_ [reply] = idSetter;
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleMessageSent ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::SendTyping (qulonglong to)
	{
		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([this, nam, to] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/messages.setActivity");
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("uid", QString::number (to));
				url.addQueryItem ("type", "typing");

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				connect (reply,
						SIGNAL (finished ()),
						reply,
						SLOT (deleteLater ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	namespace
	{
		template<typename T>
		QString CommaJoin (const QList<T>& ids)
		{
			QStringList converted;
			for (auto id : ids)
				converted << QString::number (id);
			return converted.join (",");
		}
	}

	void VkConnection::MarkAsRead (const QList<qulonglong>& ids)
	{
		const auto& joined = CommaJoin (ids);

		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([this, nam, joined] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/messages.markAsRead");
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("mids", joined);

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				connect (reply,
						SIGNAL (finished ()),
						reply,
						SLOT (deleteLater ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::RequestGeoIds (const QList<int>& codes, GeoSetter_f setter, GeoIdType type)
	{
		const auto& joined = CommaJoin (codes);

		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([=] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QString method;
				switch (type)
				{
				case GeoIdType::Country:
					method = "getCountries";
					break;
				case GeoIdType::City:
					method = "getCities";
					break;
				}

				QUrl url ("https://api.vk.com/method/" + method);
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("cids", joined);

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				CountryReply2Setter_ [reply] = setter;
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleCountriesFetched ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::GetUserInfo (const QList<qulonglong>& ids)
	{
		auto nam = Proxy_->GetNetworkAccessManager ();
		const auto& joined = CommaJoin (ids);
		PreparedCalls_.push_back ([this, nam, joined] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/users.get");
				url.addQueryItem ("access_token", key);
				if (!joined.isEmpty ())
					url.addQueryItem ("uids", joined);
				url.addQueryItem ("fields", UserFields);

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleGotUserInfo ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::GetMessageInfo (qulonglong id, MessageInfoSetter_f setter)
	{
		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([=] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/messages.getById");
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("mid", QString::number (id));
				url.addQueryItem ("photo_sizes", "1");

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				Reply2MessageSetter_ [reply] = setter;
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleMessageInfoFetched ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::GetPhotoInfos (const QStringList& ids, PhotoInfoSetter_f setter)
	{
		const auto& joined = ids.join (",");
		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([=] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/photos.getById");
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("photos", joined);

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				Reply2PhotoSetter_ [reply] = setter;
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handlePhotoInfosFetched ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::AddFriendList (const QString& name, const QList<qulonglong>& ids)
	{
		const auto& joined = CommaJoin (ids);
		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([this, joined, name, nam] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/friends.addList");
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("name", name);
				url.addQueryItem ("uids", joined);

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				Reply2ListName_ [reply] = name;
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleFriendListAdded ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::ModifyFriendList (const ListInfo& list, const QList<qulonglong>& newContents)
	{
		const auto& joined = CommaJoin (newContents);
		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([this, joined, list, nam] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/friends.editList");
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("lid", QString::number (list.ID_));
				url.addQueryItem ("name", list.Name_);
				url.addQueryItem ("uids", joined);

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				connect (reply,
						SIGNAL (finished ()),
						reply,
						SLOT (deleteLater ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::SetNRIList (const QList<qulonglong>& ids)
	{
		const auto& joined = CommaJoin (ids);
		auto nam = Proxy_->GetNetworkAccessManager ();

		PreparedCalls_.push_back ([this, joined, nam] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/storage.set");
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("key", "non_roster_items");
				url.addQueryItem ("value", joined);

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				connect (reply,
						SIGNAL (finished ()),
						reply,
						SLOT (deleteLater ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::CreateChat (const QString& title, const QList<qulonglong>& ids)
	{
		const auto& joined = CommaJoin (ids);
		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([=] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/messages.createChat");
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("title", title);
				url.addQueryItem ("uids", joined);

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				Reply2ChatInfo_ [reply] = { 0, title, ids };
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleChatCreated ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::RequestChatInfo (qulonglong id)
	{
		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([=] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/messages.getChat");
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("chat_id", QString::number (id));

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleChatInfo ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::RemoveChatUser (qulonglong chat, qulonglong user)
	{
		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([=] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/messages.removeChatUser");
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("chat_id", QString::number (chat));
				url.addQueryItem ("uid", QString::number (user));

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				Reply2ChatRemoveInfo_ [reply] = ChatRemoveInfo { chat, user };
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleChatUserRemoved ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::SetStatus (const QString& status)
	{
		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([=] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/status.set");
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("text", status);

				AddParams (url, params);

				auto reply = nam->get (QNetworkRequest (url));
				connect (reply,
						SIGNAL (finished ()),
						reply,
						SLOT (deleteLater ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::SetStatus (const EntryStatus& status)
	{
		Logger_ (IHaveConsole::PacketDirection::Out) << "setting status" << status.State_;
		LPManager_->ForceServerRequery ();

		Status_ = status;
		if (Status_.State_ == SOffline)
		{
			LPManager_->Stop ();
			return;
		}

		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([this, nam] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl lpUrl ("https://api.vk.com/method/users.get");
				lpUrl.addQueryItem ("access_token", key);
				lpUrl.addQueryItem ("fields",
						"first_name,last_name,nickname,photo,photo_big,sex,"
						"bdate,city,country,timezone,contacts,education");
				AddParams (lpUrl, params);
				auto reply = nam->get (QNetworkRequest (lpUrl));
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleGotSelfInfo ()));
				return reply;
			});
		PreparedCalls_.push_back ([this, nam] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl lpUrl ("https://api.vk.com/method/friends.getLists");
				lpUrl.addQueryItem ("access_token", key);
				AddParams (lpUrl, params);
				auto reply = nam->get (QNetworkRequest (lpUrl));
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleGotFriendLists ()));
				return reply;
			});
		AuthMgr_->GetAuthKey ();
	}

	EntryStatus VkConnection::GetStatus () const
	{
		return CurrentStatus_;
	}

	void VkConnection::SetMarkingOnlineEnabled (bool enabled)
	{
		Logger_ (IHaveConsole::PacketDirection::Out) << "SetMarkingOnlineEnabled" << enabled;
		MarkingOnline_ = enabled;

		if (enabled)
		{
			markOnline ();
			MarkOnlineTimer_->start ();
		}
		else
			MarkOnlineTimer_->stop ();
	}

	void VkConnection::QueueRequest (VkConnection::PreparedCall_f call)
	{
		PreparedCalls_ << call;
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::AddParams (QUrl& url, const UrlParams_t& params)
	{
		for (auto i = params.begin (); i != params.end (); ++i)
			url.addQueryItem (i.key (), i.value ());
	}

	void VkConnection::HandleCaptcha (const QString& cid, const QString& value)
	{
		if (!CaptchaId2Call_.contains (cid))
			return;

		auto call = CaptchaId2Call_.take (cid);
		if (value.isEmpty ())
			return;

		call.ClearParams ();
		call.AddParam ({ "captcha_sid", cid });
		call.AddParam ({ "captcha_img", value });

		PreparedCalls_.push_front (call);

		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::PushFriendsRequest ()
	{
		auto nam = Proxy_->GetNetworkAccessManager ();

		PreparedCalls_.push_back ([this, nam] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl friendsUrl ("https://api.vk.com/method/friends.get");
				friendsUrl.addQueryItem ("access_token", key);
				friendsUrl.addQueryItem ("fields", UserFields);
				AddParams (friendsUrl, params);
				auto reply = nam->get (QNetworkRequest (friendsUrl));
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleGotFriends ()));
				return reply;
			});

		PreparedCalls_.push_back ([this, nam] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/storage.get");
				url.addQueryItem ("access_token", key);
				url.addQueryItem ("key", "non_roster_items");
				AddParams (url, params);
				auto reply = nam->get (QNetworkRequest (url));
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleGotNRI ()));
				return reply;
			});
	}

	auto VkConnection::FindRunning (QNetworkReply *reply) const -> RunningCalls_t::const_iterator
	{
		return std::find_if (RunningCalls_.begin (), RunningCalls_.end (),
				[reply] (decltype (RunningCalls_.at (0)) call) { return call.first == reply; });
	}

	auto VkConnection::FindRunning (QNetworkReply *reply) -> RunningCalls_t::iterator
	{
		return std::find_if (RunningCalls_.begin (), RunningCalls_.end (),
				[reply] (decltype (RunningCalls_.at (0)) call) { return call.first == reply; });
	}

	void VkConnection::RescheduleRequest (QNetworkReply *reply)
	{
		const auto pos = FindRunning (reply);
		if (pos != RunningCalls_.end ())
			PreparedCalls_.push_front (pos->second);
		else
			qWarning () << Q_FUNC_INFO
					<< "no running call found for the reply";
	}

	bool VkConnection::CheckFinishedReply (QNetworkReply *reply)
	{
		reply->deleteLater ();

		if (reply->error () == QNetworkReply::NoError)
		{
			APIErrorCount_ = 0;
			return true;
		}

		qWarning () << Q_FUNC_INFO
				<< "reply error:"
				<< reply->error ()
				<< reply->errorString ();

		RescheduleRequest (reply);

		++APIErrorCount_;

		if (!ShouldRerunPrepared_)
		{
			QTimer::singleShot (30000,
					this,
					SLOT (rerunPrepared ()));
			ShouldRerunPrepared_ = true;
		}

		return false;
	}

	bool VkConnection::CheckReplyData (const QVariant& mapVar, QNetworkReply *reply)
	{
		const auto& map = mapVar.toMap ();
		if (!map.contains ("error"))
			return true;

		const auto& errMap = map ["error"].toMap ();
		const auto ec = errMap ["error_code"].toULongLong ();

		Logger_ << "got error:" << errMap ["error_code"].toULongLong () << errMap ["error_msg"].toString ();
		Logger_ << errMap;

		switch (ec)
		{
		case 5:
			RescheduleRequest (reply);
			reauth ();
			break;
		case 14:
		{
			const auto pos = FindRunning (reply);
			if (pos == RunningCalls_.end ())
			{
				qWarning () << Q_FUNC_INFO
						<< "non-running reply captcha";
				break;
			}

			const auto& cid = errMap ["captcha_sid"].toString ();
			const auto& img = errMap ["captcha_img"].toString ();

			CaptchaId2Call_ [cid] = pos->second;

			emit captchaNeeded (cid, QUrl::fromEncoded (img.toUtf8 ()));

			break;
		}
		}

		return false;
	}

	void VkConnection::reauth ()
	{
		Logger_ << "reauthing";
		auto status = GetStatus ();
		SetStatus (EntryStatus { SOffline, {} });

		AuthMgr_->clearAuthData ();

		SetStatus (status);
	}

	void VkConnection::rerunPrepared ()
	{
		ShouldRerunPrepared_ = false;

		if (!PreparedCalls_.isEmpty ())
			AuthMgr_->GetAuthKey ();
	}

	void VkConnection::callWithKey (const QString& key)
	{
		while (!PreparedCalls_.isEmpty ())
		{
			auto f = PreparedCalls_.takeFirst ();
			CallQueue_->Schedule ([this, f, key] () -> void
					{
						const auto reply = f (key);
						Logger_ (IHaveConsole::PacketDirection::Out) << reply->request ().url ();
						RunningCalls_.append ({ reply, f });

						connect (reply,
								SIGNAL (destroyed ()),
								this,
								SLOT (handleReplyDestroyed ()));
					});
		}
	}

	void VkConnection::handleReplyDestroyed ()
	{
		const auto reply = static_cast<QNetworkReply*> (sender ());
		const auto pos = FindRunning (reply);

		if (pos == RunningCalls_.end ())
		{
			qWarning () << Q_FUNC_INFO
					<< "finished a non-running reply";
			return;
		}

		RunningCalls_.erase (pos);
	}

	void VkConnection::markOnline ()
	{
		if (Status_.State_ == SOffline)
			return;

		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([this, nam] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl url ("https://api.vk.com/method/account.setOnline");
				url.addQueryItem ("access_token", key);
				AddParams (url, params);
				auto reply = nam->get (QNetworkRequest (url));
				connect (reply,
						SIGNAL (finished ()),
						reply,
						SLOT (deleteLater ()));
				return reply;
			});
		Logger_ (IHaveConsole::PacketDirection::Out) << "markOnline";
		AuthMgr_->GetAuthKey ();
	}

	void VkConnection::handleListening ()
	{
		Logger_ << "listening now";
		CurrentStatus_ = Status_;
		emit statusChanged (GetStatus ());

		SetMarkingOnlineEnabled (MarkingOnline_);
	}

	void VkConnection::handlePollError ()
	{
		Logger_ << "poll error";
		CurrentStatus_ = EntryStatus ();
		emit statusChanged (GetStatus ());
	}

	void VkConnection::handlePollStopped ()
	{
		Logger_ << "poll stopped";
		CurrentStatus_ = Status_;
		emit statusChanged (GetStatus ());

		emit stoppedPolling ();

		MarkOnlineTimer_->stop ();
	}

	void VkConnection::handlePollData (const QVariantMap& rootMap)
	{
		Logger_ << "got poll data" << rootMap;
		for (const auto& update : rootMap ["updates"].toList ())
		{
			const auto& parmList = update.toList ();
			const auto code = parmList.value (0).toInt ();

			if (!Dispatcher_.contains (code))
				qWarning () << Q_FUNC_INFO
						<< "unknown code"
						<< code
						<< parmList;
			else
				Dispatcher_ [code] (parmList);
		}
	}

	void VkConnection::handleFriendListAdded ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		if (!CheckFinishedReply (reply))
			return;

		const auto& name = Reply2ListName_.take (reply);

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		const auto id = data.toMap () ["response"].toMap () ["lid"].toULongLong ();
		emit addedLists ({ { id, name } });
	}

	namespace
	{
		UserInfo UserMap2Info (const QVariantMap& userMap)
		{
			QList<qulonglong> lists;
			for (const auto& item : userMap ["lists"].toList ())
				lists << item.toULongLong ();

			auto dateString = userMap ["bdate"].toString ();
			if (dateString.count ('.') == 1)
				dateString += ".1800";

			return
			{
				userMap ["uid"].toULongLong (),

				userMap ["first_name"].toString (),
				userMap ["last_name"].toString (),
				userMap ["nickname"].toString (),

				QUrl (userMap ["photo"].toString ()),
				QUrl (userMap ["photo_big"].toString ()),

				userMap ["sex"].toInt (),

				QDate::fromString (dateString, "d.M.yyyy"),

				userMap ["home_phone"].toString (),
				userMap ["mobile_phone"].toString (),

				userMap ["timezone"].toInt (),
				userMap ["country"].toInt (),
				userMap ["city"].toInt (),

				static_cast<bool> (userMap ["online"].toULongLong ()),

				lists
			};
		}
	}

	void VkConnection::handleGotSelfInfo ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		if (!CheckFinishedReply (reply))
			return;

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		const auto& list = data.toMap () ["response"].toList ();
		emit gotSelfInfo (UserMap2Info (list.value (0).toMap ()));
	}

	void VkConnection::handleGotFriendLists ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		if (!CheckFinishedReply (reply))
			return;

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		QList<ListInfo> lists;
		for (const auto& item : data.toMap () ["response"].toList ())
		{
			const auto& map = item.toMap ();
			lists.append ({ map ["lid"].toULongLong (), map ["name"].toString () });
		}
		emit gotLists (lists);

		PushFriendsRequest ();
		AuthMgr_->GetAuthKey ();
	}

	namespace
	{
		QList<UserInfo> ParseUsers (const QVariant& data)
		{
			QList<UserInfo> users;

			for (const auto& item : data.toMap () ["response"].toList ())
			{
				const auto& userMap = item.toMap ();
				if (userMap.contains ("deactivated"))
					continue;

				users << UserMap2Info (userMap);
			}

			return users;
		}
	}

	void VkConnection::handleGotFriends ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		if (!CheckFinishedReply (reply))
			return;

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		const auto& users = ParseUsers (data);
		emit gotUsers (users);

		auto nam = Proxy_->GetNetworkAccessManager ();
		PreparedCalls_.push_back ([this, nam] (const QString& key, const UrlParams_t& params) -> QNetworkReply*
			{
				QUrl msgUrl ("https://api.vk.com/method/messages.get");
				msgUrl.addQueryItem ("access_token", key);
				msgUrl.addQueryItem ("filters", "1");
				AddParams (msgUrl, params);
				auto reply = nam->get (QNetworkRequest (msgUrl));
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleGotUnreadMessages ()));
				return reply;
			});

		LPManager_->start ();
	}

	void VkConnection::handleGotNRI ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		if (!CheckFinishedReply (reply))
			return;

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		const auto& str = data.toMap () ["response"].toString ();
		QList<qulonglong> ids;
		for (const auto& sub : str.split (",", QString::SkipEmptyParts))
		{
			bool ok = false;
			const auto id = sub.toULongLong (&ok);
			if (ok)
				ids << id;
		}

		emit gotNRIList (ids);
	}

	void VkConnection::handleGotUserInfo ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		if (!CheckFinishedReply (reply))
			return;

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		const auto& users = ParseUsers (data);
		emit gotUsers (users);
	}

	void VkConnection::handleGotUnreadMessages ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		if (!CheckFinishedReply (reply))
			return;

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		auto respList = data.toMap () ["response"].toList ();
		if (respList.isEmpty ())
		{
			qWarning () << Q_FUNC_INFO
					<< "no response"
					<< data;
			return;
		}
		respList.removeFirst ();

		for (const auto& msgMapVar : respList)
		{
			const auto& map = msgMapVar.toMap ();
			Logger_ << "got unread message:" << QVariant { map };

			MessageFlags flags = MessageFlag::Unread;
			if (map ["out"].toULongLong ())
				flags |= MessageFlag::Outbox;

			emit gotMessage ({
					map ["mid"].toULongLong (),
					map ["uid"].toULongLong (),
					map ["body"].toString (),
					flags,
					QDateTime::fromTime_t (map ["date"].toULongLong ()),
					{}
				});
		}
	}

	void VkConnection::handleChatCreated ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		if (!CheckFinishedReply (reply))
			return;

		auto info = Reply2ChatInfo_.take (reply);

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		info.ChatID_ = data.toMap () ["response"].toULongLong ();

		emit gotChatInfo (info);
	}

	void VkConnection::handleChatInfo ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		if (!CheckFinishedReply (reply))
			return;

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		const auto& map = data.toMap () ["response"].toMap ();

		QList<qulonglong> users;
		for (auto item : map ["users"].toList ())
			users << item.toULongLong ();

		emit gotChatInfo ({
				map ["chat_id"].toULongLong (),
				map ["title"].toString (),
				users
			});
	}

	void VkConnection::handleChatUserRemoved ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		if (!CheckFinishedReply (reply))
			return;

		auto removeInfo = Reply2ChatRemoveInfo_.take (reply);

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		const auto& map = data.toMap ();
		if (map ["response"].toULongLong () == 1)
			emit chatUserRemoved (removeInfo.Chat_, removeInfo.User_);
	}

	void VkConnection::handleMessageSent ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		const auto& setter = MsgReply2Setter_.take (reply);
		if (!setter)
			return;

		if (!CheckFinishedReply (reply))
			return;

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		const auto code = data.toMap ().value ("response", -1).toULongLong ();
		setter (code);
	}

	void VkConnection::handleCountriesFetched ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		const auto& setter = CountryReply2Setter_.take (reply);
		if (!setter)
			return;

		if (!CheckFinishedReply (reply))
			return;

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		QHash<int, QString> result;
		for (const auto& item : data.toMap () ["response"].toList ())
		{
			const auto& map = item.toMap ();
			result [map ["cid"].toInt ()] = map ["name"].toString ();
		}

		setter (result);
	}

	namespace
	{
		PhotoInfo PhotoMap2Info (const QVariantMap& map)
		{
			QString bigSrc;
			QSize bigSize;
			QString thumbSrc;
			QSize thumbSize;

			QString currentBigType;
			const QStringList bigTypes { "x", "y", "z", "w" };

			for (const auto& elem : map ["sizes"].toList ())
			{
				const auto& eMap = elem.toMap ();

				auto size = [&eMap]
				{
					return QSize (eMap ["width"].toInt (), eMap ["height"].toInt ());
				};

				const auto& type = eMap ["type"].toString ();
				if (type == "m")
				{
					thumbSrc = eMap ["src"].toString ();
					thumbSize = size ();
				}
				else if (bigTypes.indexOf (type) > bigTypes.indexOf (currentBigType))
				{
					currentBigType = type;
					bigSrc = eMap ["src"].toString ();
					bigSize = size ();
				}
			}

			return
			{
				map ["owner_id"].toLongLong (),
				map ["pid"].toULongLong (),
				map ["aid"].toLongLong (),

				thumbSrc,
				thumbSize,
				bigSrc,
				bigSize,

				map ["access_key"].toString ()
			};
		}

		AudioInfo AudioMap2Info (const QVariantMap& map)
		{
			return
			{
				map ["owner_id"].toLongLong (),
				map ["aid"].toULongLong (),
				map ["artist"].toString (),
				map ["title"].toString (),
				map ["duration"].toInt (),
				map ["url"].toString ()
			};
		}

		VideoInfo VideoMap2Info (const QVariantMap& map)
		{
			return
			{
				map ["owner_id"].toLongLong (),
				map ["vid"].toULongLong (),
				map ["access_key"].toString (),
				map ["title"].toString (),
				map ["description"].toString (),
				map ["duration"].toULongLong (),
				map ["views"].toLongLong (),
				map ["image_big"].toString ()
			};
		}

		void HandleAttachments (FullMessageInfo& info, const QVariant& attachments, Logger& logger)
		{
			const auto& attList = attachments.toList ();
			for (const auto& attVar : attList)
			{
				const auto& attMap = attVar.toMap ();
				if (attMap.contains ("photo"))
					info.Photos_ << PhotoMap2Info (attMap ["photo"].toMap ());
				else if (attMap.contains ("audio"))
					info.Audios_ << AudioMap2Info (attMap ["audio"].toMap ());
				else if (attMap.contains ("video"))
					info.Videos_ << VideoMap2Info (attMap ["video"].toMap ());
				else if (attMap.contains ("wall"))
				{
					auto wallMap = attMap ["wall"].toMap ();

					FullMessageInfo repost;
					repost.OwnerID_ = wallMap ["from_id"].toLongLong ();
					repost.ID_ = wallMap ["id"].toULongLong ();
					repost.Text_ = wallMap ["text"].toString ();
					repost.Likes_ = wallMap ["likes"].toMap () ["count"].toInt ();
					repost.Reposts_ = wallMap ["reposts"].toMap () ["count"].toInt ();
					repost.PostDate_ = QDateTime::fromTime_t (wallMap ["date"].toLongLong ());

					HandleAttachments (repost, wallMap.take ("attachments"), logger);
					wallMap.take ("attachment");
					logger << "attachments left:" << wallMap;

					info.ContainedReposts_.append (repost);
				}
				else
					logger << "HandleAttachments" << attMap.keys ();
			}
		}
	}

	void VkConnection::handleMessageInfoFetched ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		const auto& setter = Reply2MessageSetter_.take (reply);
		if (!setter)
			return;

		if (!CheckFinishedReply (reply))
			return;

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		FullMessageInfo info;
		const auto& infoList = data.toMap () ["response"].toList ();
		for (const auto& item : infoList)
		{
			if (item.type () != QVariant::Map)
				continue;

			const auto& map = item.toMap ();
			HandleAttachments (info, map ["attachments"], Logger_);
		}

		setter (info);
	}

	void VkConnection::handlePhotoInfosFetched ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		const auto& setter = Reply2PhotoSetter_.take (reply);
		if (!setter)
		{
			qWarning () << Q_FUNC_INFO
					<< "no setter";
			return;
		}

		if (!CheckFinishedReply (reply))
			return;

		const auto& data = QJson::Parser ().parse (reply);
		if (!CheckReplyData (data, reply))
			return;

		QList<PhotoInfo> result;
		for (const auto& item : data.toMap () ["response"].toList ())
			result << PhotoMap2Info (item.toMap ());

		setter (result);
	}

	void VkConnection::saveCookies (const QByteArray& cookies)
	{
		LastCookies_ = cookies;
		emit cookiesChanged ();
	}
}
}
}
