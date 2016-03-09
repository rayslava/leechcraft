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

#include "twitterinterface.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>
#include <QDebug>
#include <QtKOAuth/QtKOAuth>
#include <util/xpc/util.h>
#include <util/sll/parsejson.h>
#include "core.h"
#include "xmlsettingsmanager.h"
#include "twitteruser.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Woodpecker
{
	TwitterInterface::TwitterInterface (Plugin *plugin, QObject *parent)
	: QObject (parent)
	, ParentPlugin_ (plugin)
	{
		HttpClient_ = Core::Instance ().GetCoreProxy ()->GetNetworkAccessManager ();
		OAuthRequest_ = new KQOAuthRequest (this);
		OAuthManager_ = new KQOAuthManager (this);

#ifdef WP_DEBUG
		OAuthRequest_->setEnableDebugOutput (true);
#else
		OAuthRequest_->setEnableDebugOutput (false);
#endif
		ConsumerKey_ = XmlSettingsManager::Instance ()->property ("consumer_key").toString ();
		ConsumerKeySecret_ = XmlSettingsManager::Instance ()->
				property ("consumer_key_secret").toString ();

		connect (OAuthManager_,
				SIGNAL (requestReady (QByteArray)),
				this,
				SLOT (onRequestReady (QByteArray)));

		connect (OAuthManager_,
				SIGNAL (authorizedRequestDone ()),
				this,
				SLOT (onAuthorizedRequestDone ()));
	}

	void TwitterInterface::RequestTwitter (const QUrl& requestAddress)
	{
		auto reply = HttpClient_->get (QNetworkRequest (requestAddress));
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (replyFinished ()));
	}

	void TwitterInterface::replyFinished ()
	{
		QByteArray jsonText (qobject_cast<QNetworkReply*> (sender ())->readAll ());
		sender ()->deleteLater ();

		emit tweetsReady (ParseReply (jsonText));
	}

	QList<Tweet_ptr> TwitterInterface::ParseReply (const QByteArray& json)
	{
		const auto& parsedReply = Util::ParseJson (json, Q_FUNC_INFO);
		if (parsedReply.isNull ())
			return {};

		const auto& answers = LastRequestMode_ == FeedMode::SearchResult ?
				parsedReply.toMap () ["statuses"].toList () :
				parsedReply.toList ();

		QList<Tweet_ptr> result;
		const QLocale locale (QLocale::English);
		for (int i = answers.count () - 1; i >= 0 ; --i)
		{
			const auto& tweetMap = answers [i].toMap ();
			const auto& userMap = tweetMap ["user"].toMap ();
			QString text = tweetMap ["text"].toString ();

			const auto& entities = tweetMap.value ("entities").toMap ();
			for (const auto& media : entities.value ("media").toList ())
			{
				const auto& medium = media.toMap ();
				if (medium ["type"].toString () != "photo")
					continue;

				if (medium.contains ("media_url_https"))
					text.replace (medium ["url"].toString (),
							QString ("<a class=\"link\" href=\"twitter://media/photo/%1\">%1</a>")
								.arg (medium ["media_url_https"].toString ()));
				else if (medium.contains ("media_url"))
					text.replace (medium ["url"].toString (),
							QString ("<a class=\"link\" href=\"twitter://media/photo/%1\">%1</a>")
								.arg (medium ["media_url"].toString ()));
				else
				{
					qWarning () << Q_FUNC_INFO << "Found photo without an url";
					continue;
				}
			};

			const auto& username = userMap ["screen_name"].toString ();
			const auto userid = userMap ["id"].toInt ();
			const auto twitid = tweetMap ["id"].toULongLong ();
			Tweet_ptr tempTweet;
			if (auto user = ParentPlugin_->GetUserManager ()->GetUser(userid))
				tempTweet = std::make_shared<Tweet> (text, twitid, user);
			else
			{
				;
				tempTweet = std::make_shared<Tweet> (text, twitid,
								     ParentPlugin_->GetUserManager ()->AddUser (TwitterUser(userid, username, ParentPlugin_->GetUserManager ())));
			};

			tempTweet->GetAuthor ()->DownloadAvatar (userMap ["profile_image_url"].toString ());
			connect (tempTweet->GetAuthor (),
					SIGNAL (userAvatarReady ()),
					parent (),
					SLOT (setUpdateReady ()));
			tempTweet->SetDateTime (locale.toDateTime (tweetMap ["created_at"].toString (),
					"ddd MMM dd HH:mm:ss +0000 yyyy"));
			result.push_back (tempTweet);
		}

		return result;
	}

	void TwitterInterface::GetAccess ()
	{
#ifdef WP_DEBUG
		qDebug() << Q_FUNC_INFO;
#endif
		connect (OAuthManager_,
				SIGNAL (temporaryTokenReceived (QString, QString)),
				this,
				SLOT (onTemporaryTokenReceived (QString, QString)));

		connect (OAuthManager_,
				SIGNAL (authorizationReceived (QString, QString)),
				this,
				SLOT (onAuthorizationReceived (QString, QString)));

		connect (OAuthManager_,
				SIGNAL (accessTokenReceived (QString, QString)),
				this,
				SLOT (onAccessTokenReceived (QString, QString)));

		connect (OAuthManager_,
				SIGNAL (authorizationPageRequested (QUrl)),
				this,
				SLOT (onAuthorizationPageRequested (QUrl)));

		OAuthRequest_->initRequest (KQOAuthRequest::TemporaryCredentials,
				QUrl ("https://api.twitter.com/oauth/request_token"));
		OAuthRequest_->setConsumerKey (ConsumerKey_);
		OAuthRequest_->setConsumerSecretKey (ConsumerKeySecret_);
		OAuthManager_->setHandleUserAuthorization (true);
		OAuthManager_->setHandleAuthorizationPageOpening (false);

		OAuthManager_->executeRequest (OAuthRequest_);
	}

	void TwitterInterface::SignedRequest (TwitterRequest req,
			KQOAuthRequest::RequestHttpMethod method, KQOAuthParameters params)
	{
		QUrl reqUrl;

		if (Token_.isEmpty () || TokenSecret_.isEmpty ())
		{
			qWarning () << "No access tokens. Aborting.";
			return;
		}

		switch (req)
		{
		case TwitterRequest::HomeTimeline:
			reqUrl = "https://api.twitter.com/1.1/statuses/home_timeline.json";
			params.insert ("count", "50");
			params.insert ("include_entities", "true");
			break;

		case TwitterRequest::UserTimeline:
			reqUrl = "http://api.twitter.com/1.1/statuses/user_timeline.json";
			params.insert ("include_entities", "true");
			break;

		case TwitterRequest::Search:
			reqUrl = "https://api.twitter.com/1.1/search/tweets.json";
			params.insert ("count", "50");
			params.insert ("include_entities", "true");
			break;

		case TwitterRequest::Update:
			reqUrl = "http://api.twitter.com/1.1/statuses/update.json";
			break;

		case TwitterRequest::Direct:
			reqUrl = "https://api.twitter.com/1.1/direct_messages.json";

		case TwitterRequest::Retweet:
			reqUrl = QString ("http://api.twitter.com/1.1/statuses/retweet/")
					.append (params.value ("id"))
					.append (".json");
			break;

		case TwitterRequest::Reply:
			reqUrl = "http://api.twitter.com/1.1/statuses/update.json";
			break;

		case TwitterRequest::SpamReport:
			reqUrl = "http://api.twitter.com/1.1/report_spam.json";
			break;

		case TwitterRequest::Delete:
			reqUrl = QString ("http://api.twitter.com/1.1/statuses/destroy/")
					.append (params.value ("id"))
					.append (".json");
			break;

		case TwitterRequest::CreateFavorite:
			reqUrl = QString ("https://api.twitter.com/1.1/favorites/create.json");
			break;

		case TwitterRequest::DeleteFavorite:
			reqUrl = QString ("https://api.twitter.com/1.1/favorites/destroy.json");
			break;

		case TwitterRequest::ListFavorites:
			reqUrl = "https://api.twitter.com/1.1/favorites/list.json";
			params.insert ("include_entities", "true");
			params.insert ("count", "50");
			break;

		case TwitterRequest::Friends:
			reqUrl = "https://api.twitter.com/1.1/friends/ids.json";
			params.insert ("include_entities", "true");
			params.insert ("count", "5000");
			break;
		default:
			return;
		}

		OAuthRequest_->initRequest (KQOAuthRequest::AuthorizedRequest, reqUrl);
		OAuthRequest_->setHttpMethod (method);
		OAuthRequest_->setConsumerKey (ConsumerKey_);
		OAuthRequest_->setConsumerSecretKey (ConsumerKeySecret_);
		OAuthRequest_->setToken (Token_);
		OAuthRequest_->setTokenSecret (TokenSecret_);
		OAuthRequest_->setAdditionalParameters (params);
		OAuthManager_->executeRequest (OAuthRequest_);
	}

	void TwitterInterface::SendTweet (const QString& tweet)
	{
		KQOAuthParameters param;
		param.insert ("status", tweet);
		SignedRequest (TwitterRequest::Update, KQOAuthRequest::POST, param);
	}

	void TwitterInterface::Retweet (const qulonglong id)
	{
		KQOAuthParameters param;
		param.insert ("id", QString::number (id));
		SignedRequest (TwitterRequest::Retweet, KQOAuthRequest::POST, param);
	}

	void TwitterInterface::Reply (const qulonglong replyid, const QString& tweet)
	{
		KQOAuthParameters param;
		param.insert ("status", tweet);
		param.insert ("in_reply_to_status_id", QString::number (replyid));
		SignedRequest (TwitterRequest::Reply, KQOAuthRequest::POST, param);
	}

	void TwitterInterface::onAuthorizedRequestDone ()
	{
#ifdef WP_DEBUG
		qDebug () << Q_FUNC_INFO << "Request sent to Twitter!";
#endif
	}

	void TwitterInterface::onRequestReady (const QByteArray& response)
	{
#ifdef WP_DEBUG
		qDebug () << Q_FUNC_INFO << "Response from the service: recvd";// << response;
#endif
		emit tweetsReady (ParseReply (response));
	}

	void TwitterInterface::onAuthorizationReceived (const QString&, const QString&)
	{
#ifdef WP_DEBUG
		qDebug () << Q_FUNC_INFO;
#endif
		OAuthManager_->getUserAccessTokens (QUrl ("https://api.twitter.com/oauth/access_token"));

		if (OAuthManager_->lastError () != KQOAuthManager::NoError)
		{
			qWarning () << Q_FUNC_INFO << "Authorization error";
		}
	}

	void TwitterInterface::onAccessTokenReceived (const QString& token, const QString& tokenSecret)
	{
#ifdef WP_DEBUG
		qDebug () << Q_FUNC_INFO << "Access tokens now stored. You are ready to send Tweets from user's account!";
#endif
		this->Token_ = token;
		this->TokenSecret_ = tokenSecret;


		emit authorized (token, tokenSecret);

	}

	void TwitterInterface::onTemporaryTokenReceived (const QString&, const QString&)
	{
		const QUrl userAuthURL ("https://api.twitter.com/oauth/authorize");

		if (OAuthManager_->lastError () == KQOAuthManager::NoError)
		{
#ifdef WP_DEBUG
			qDebug () << Q_FUNC_INFO << "Asking for user's permission to access protected resources. Opening URL: " << userAuthURL;
#endif
			OAuthManager_->getUserAuthorization (userAuthURL);
		}
		else
		{
			qWarning () << Q_FUNC_INFO << "KQOAuthManager error: " << OAuthManager_->lastError ();
		}
	}

	void TwitterInterface::Xauth ()
	{
#ifdef WP_DEBUG
		qDebug() << Q_FUNC_INFO;
#endif
		connect (OAuthManager_,
				SIGNAL (accessTokenReceived (QString, QString)),
				this,
				SLOT (onAccessTokenReceived (QString, QString)));

		KQOAuthRequest_XAuth *oauthRequest = new KQOAuthRequest_XAuth (this);
		oauthRequest->initRequest (KQOAuthRequest::AccessToken,
				QUrl ("https://api.twitter.com/oauth/access_token"));
		oauthRequest->setConsumerKey (XmlSettingsManager::Instance ()->
					property ("consumer_key").toString ());
		oauthRequest->setConsumerSecretKey (XmlSettingsManager::Instance ()->
					property ("consumer_key_secret").toString ());
		OAuthManager_->executeRequest (oauthRequest);
	}

	void TwitterInterface::Login (const QString& savedToken, const QString& savedTokenSecret)
	{
		Token_ = savedToken;
		TokenSecret_ = savedTokenSecret;

#ifdef WP_DEBUG
		qDebug () << Q_FUNC_INFO << "Successfully logged in";
#endif
	}

	void TwitterInterface::ReportSPAM (const QString& username, const qulonglong userid)
	{
		KQOAuthParameters param;

		param.insert ("screen_name", username);
		if (userid)
			param.insert ("user_id", QString::number (userid));
		SignedRequest (TwitterRequest::SpamReport, KQOAuthRequest::POST, param);
	}

	FeedMode TwitterInterface::GetLastRequestMode () const
	{
		return LastRequestMode_;
	}

	void TwitterInterface::SetLastRequestMode (const FeedMode& newLastRequestMode)
	{
		LastRequestMode_ = newLastRequestMode;
	}

	void TwitterInterface::request (const KQOAuthParameters& param, const FeedMode mode)
	{
		switch (mode)
		{
		case FeedMode::UserTimeline:
			SetLastRequestMode (FeedMode::UserTimeline);
			SignedRequest (TwitterRequest::UserTimeline, KQOAuthRequest::GET, param);
			break;

		case FeedMode::HomeTimeline:
			SetLastRequestMode (FeedMode::HomeTimeline);
			SignedRequest (TwitterRequest::HomeTimeline, KQOAuthRequest::GET, param);
			break;

		case FeedMode::SearchResult:
			SetLastRequestMode (FeedMode::SearchResult);
			SignedRequest (TwitterRequest::Search, KQOAuthRequest::GET, param);
			break;

		case FeedMode::Favorites:
			SetLastRequestMode (FeedMode::Favorites);
			SignedRequest (TwitterRequest::ListFavorites, KQOAuthRequest::GET, param);
			break;

		case FeedMode::Users:
			SetLastRequestMode (FeedMode::Users);
			SignedRequest (TwitterRequest::Friends, KQOAuthRequest::GET, param);
			break;

		default:
			qWarning () << Q_FUNC_INFO << "Unknown request";
		}
	}

	void TwitterInterface::Delete (const qulonglong id)
	{
		KQOAuthParameters param;
		param.insert ("id", QString::number (id));
		SignedRequest (TwitterRequest::Delete, KQOAuthRequest::POST, param);
	}

	void TwitterInterface::MakeFavorite (const qulonglong id)
	{
		KQOAuthParameters param;

		param.insert ("id", QString::number (id));
		SignedRequest (TwitterRequest::CreateFavorite, KQOAuthRequest::POST, param);
	}

	void TwitterInterface::DeleteFavorite (const qulonglong id)
	{
		KQOAuthParameters param;

		param.insert ("id", QString::number (id));
		SignedRequest (TwitterRequest::DeleteFavorite, KQOAuthRequest::POST, param);
	}

	void TwitterInterface::onAuthorizationPageRequested (const QUrl& userAuthURL)
	{
		const auto& e = Util::MakeEntity (userAuthURL,
				{}, OnlyHandle | FromUserInitiated);

		Core::Instance ().GetCoreProxy ()->GetEntityManager ()->HandleEntity (e);
	}

}
}
}
