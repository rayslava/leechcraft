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

#include <functional>
#include <QtKOAuth/QtKOAuth>
#include <interfaces/blasq/iaccount.h>
#include <interfaces/core/icoreproxy.h>

class QStandardItemModel;
class QStandardItem;

namespace LeechCraft
{
namespace Blasq
{
namespace Spegnersi
{
	class FlickrService;

	class FlickrAccount : public QObject
						, public IAccount
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Blasq::IAccount)

		QString Name_;
		const QByteArray ID_;
		FlickrService * const Service_;
		const ICoreProxy_ptr Proxy_;

		KQOAuthRequest * const Req_;
		KQOAuthManager * const AuthMgr_;

		QStandardItemModel * const CollectionsModel_;
		QStandardItem *AllPhotosItem_ = 0;

		QString AuthToken_;
		QString AuthSecret_;

		bool UpdateAfterAuth_ = false;

		enum class State
		{
			Idle,
			AuthRequested,
			CollectionsRequested
		} State_ = State::Idle;

		QList<std::function<void ()>> CallQueue_;
	public:
		FlickrAccount (const QString&, FlickrService*, ICoreProxy_ptr, const QByteArray& = QByteArray ());

		QByteArray Serialize () const;
		static FlickrAccount* Deserialize (const QByteArray&, FlickrService*, ICoreProxy_ptr);

		QObject* GetQObject ();
		IService* GetService () const;
		QString GetName () const;
		QByteArray GetID () const;

		QAbstractItemModel* GetCollectionsModel () const;

		void UpdateCollections ();
	private:
		KQOAuthRequest* MakeRequest (const QUrl&, KQOAuthRequest::RequestType = KQOAuthRequest::AuthorizedRequest);

		void HandleCollectionsReply (const QByteArray&);
	private slots:
		void checkAuthTokens ();
		void requestTempToken ();

		void handleTempToken (const QString&, const QString&);
		void handleAuthorization (const QString&, const QString&);
		void handleAccessToken (const QString&, const QString&);

		void handleReply (const QByteArray&);
	signals:
		void accountChanged (FlickrAccount*);

		void doneUpdating ();
	};
}
}
}
