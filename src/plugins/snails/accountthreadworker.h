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

#include <boost/variant.hpp>
#include <QObject>
#include <vmime/net/session.hpp>
#include <vmime/net/message.hpp>
#include <vmime/net/folder.hpp>
#include <vmime/net/store.hpp>
#include <vmime/security/cert/defaultCertificateVerifier.hpp>
#include <util/sll/assoccache.h>
#include <util/sll/either.h>
#include <interfaces/structures.h>
#include "progresslistener.h"
#include "message.h"
#include "account.h"
#include "accountthreadworkerfwd.h"

class QTimer;

namespace LeechCraft
{
namespace Snails
{
	class Account;
	class MessageChangeListener;

	struct Folder;

	using MessageVector_t = std::vector<vmime::shared_ptr<vmime::net::message>>;
	using VmimeFolder_ptr = vmime::shared_ptr<vmime::net::folder>;
	using CertList_t = std::vector<vmime::shared_ptr<vmime::security::cert::X509Certificate>>;

	class AccountThreadWorker : public QObject
	{
		Q_OBJECT

		Account * const A_;

		QTimer * const NoopTimer_;

		const bool IsListening_;

		const QString ThreadName_;

		MessageChangeListener * const ChangeListener_;

		vmime::shared_ptr<vmime::net::session> Session_;
		vmime::shared_ptr<vmime::net::store> CachedStore_;

		Util::AssocCache<QStringList, vmime::shared_ptr<vmime::net::folder>> CachedFolders_;

		const vmime::shared_ptr<vmime::security::cert::defaultCertificateVerifier> CertVerifier_;
		const vmime::shared_ptr<vmime::security::authenticator> InAuth_;

		enum class FolderMode
		{
			ReadOnly,
			ReadWrite,
			NoChange
		};
	public:
		AccountThreadWorker (bool, const QString&, const CertList_t&, Account*);

		struct FolderMessages
		{
			QList<Message_ptr> NewHeaders_;
			QList<Message_ptr> UpdatedMsgs_;
			QList<QByteArray> OtherIds_;
			QList<QByteArray> RemovedIds_;
		};
		using Folder2Messages_t = QHash<QStringList, FolderMessages>;
	private:
		vmime::shared_ptr<vmime::net::store> MakeStore ();
		vmime::shared_ptr<vmime::net::transport> MakeTransport ();

		VmimeFolder_ptr GetFolder (const QStringList& folder, FolderMode mode);

		Message_ptr FromHeaders (const vmime::shared_ptr<vmime::net::message>&) const;

		Folder2Messages_t FetchMessagesIMAP (const QList<QStringList>&, const QByteArray&);
		FolderMessages FetchMessagesInFolder (const QStringList&, const VmimeFolder_ptr&, const QByteArray&);

		QList<Folder> SyncIMAPFolders (vmime::shared_ptr<vmime::net::store>);
		QList<Message_ptr> FetchFullMessages (const std::vector<vmime::shared_ptr<vmime::net::message>>&);
		ProgressListener* MkPgListener (const QString&);
	public:
		void SetNoopTimeout (int);

		void FlushSockets ();
		void Disconnect ();

		void TestConnectivity ();

		struct SyncResult
		{
			QList<Folder> AllFolders_;
			Folder2Messages_t Messages_;
		};
		SyncResult Synchronize (const QList<QStringList>&, const QByteArray& last);

		using MsgCountError_t = boost::variant<FolderNotFound>;
		using MsgCountResult_t = Util::Either<MsgCountError_t, QPair<int, int>>;
		MsgCountResult_t GetMessageCount (const QStringList& folder);

		using SetReadStatusResult_t = Util::Either<boost::variant<FolderNotFound>, QList<Message_ptr>>;
		SetReadStatusResult_t SetReadStatus (bool read, const QList<QByteArray>& ids, const QStringList& folder);

		FetchWholeMessageResult_t FetchWholeMessage (Message_ptr);

		void FetchAttachment (Message_ptr, const QString&, const QString&);

		void CopyMessages (const QList<QByteArray>& ids, const QStringList& from, const QList<QStringList>& tos);

		using DeleteResult_t = Util::Either<boost::variant<FolderNotFound>, boost::none_t>;
		DeleteResult_t DeleteMessages (const QList<QByteArray>& ids, const QStringList& folder);

		void SendMessage (const Message_ptr&);
	private slots:
		void handleMessagesChanged (const QStringList& folder, const QList<int>& numbers);

		void sendNoop ();
	signals:
		void error (const QString&);
		void gotProgressListener (ProgressListener_g_ptr);

		void folderSyncFinished (const QStringList& folder, const QByteArray& lastRequestedId);
	};
}
}
