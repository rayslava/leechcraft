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

#pragma once

#include <memory>
#include <QList>
#include <QObject>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QtKOAuth/QtKOAuth>
#include "tweet.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Woodpecker
{
	class Plugin;
	
	enum class TwitterRequest
	{
		HomeTimeline,
		Mentions,
		UserTimeline,
		Search,
		Update,
		Direct,
		Retweet,
		Reply,
		SpamReport,
		Delete,
		CreateFavorite,
		DeleteFavorite,
		ListFavorites,
	};

	enum class FeedMode
	{
		HomeTimeline,
		Mentions,
		UserTimeline,
		SearchResult,
		Direct,
		Favorites,
	};

	class TwitterInterface : public QObject
	{
		Q_OBJECT

		QNetworkAccessManager *HttpClient_;
		KQOAuthManager *OAuthManager_;
		KQOAuthRequest *OAuthRequest_;
		QString Token_;
		QString TokenSecret_;
		QString ConsumerKey_;
		QString ConsumerKeySecret_;
		QSettings *Settings_;
		FeedMode LastRequestMode_;
		Plugin * const ParentPlugin_;

		void SignedRequest (TwitterRequest req,
				KQOAuthRequest::RequestHttpMethod method = KQOAuthRequest::GET,
				KQOAuthParameters params = KQOAuthParameters ());
		void RequestTwitter (const QUrl& requestAddress);
		QList<Tweet_ptr> ParseReply (const QByteArray& json);

	public:
		explicit TwitterInterface (Plugin *plugin, QObject *parent = 0);
		void SendTweet (const QString& tweet);
		void Retweet (const qulonglong id);
		void Reply (const qulonglong replyid, const QString& tweet);
		void ReportSPAM (const QString& username, const qulonglong userid = 0);
		void Delete (const qulonglong id);
		void GetAccess ();
		void Xauth ();
		void Login (const QString& savedToken, const QString& savedTokenSecret);
		FeedMode GetLastRequestMode () const;
		void SetLastRequestMode (const FeedMode& newLastRequestMode);
		void MakeFavorite (const qulonglong id);
		void DeleteFavorite (const qulonglong id);

	private slots:
		void replyFinished ();

		void onTemporaryTokenReceived (const QString& temporaryToken,
				const QString& temporaryTokenSecret);
		void onAuthorizationReceived (const QString& token, const QString& verifier);
		void onRequestReady (const QByteArray&);
		void onAuthorizedRequestDone ();
		void onAccessTokenReceived (const QString& token, const QString& tokenSecret);
		void onAuthorizationPageRequested (const QUrl&);

	signals:
		void tweetsReady (const QList<Tweet_ptr>&);
		void authorized (const QString&, const QString&);

	public slots:
		void request (const KQOAuthParameters& param, const FeedMode mode);
	};
}
}
}

