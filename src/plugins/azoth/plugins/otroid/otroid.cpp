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

#include "otroid.h"
#include <cstring>
#include <QCoreApplication>
#include <QIcon>
#include <QAction>
#include <QMessageBox>
#include <QTranslator>
#include <QtConcurrentRun>
#include <QFutureWatcher>

extern "C"
{
#include <libotr/version.h>
#include <libotr/privkey.h>
#include <libotr/message.h>
#include <libotr/proto.h>

#if OTRL_VERSION_MAJOR >= 4
#include <libotr/instag.h>
#endif
}
#if OTRL_VERSION_MAJOR >= 4
#include <QTimer>
#endif

#include <interfaces/azoth/iprotocol.h>
#include <interfaces/azoth/iaccount.h>
#include <interfaces/azoth/iclentry.h>
#include <interfaces/azoth/imessage.h>
#include <util/util.h>

namespace LeechCraft
{
namespace Azoth
{
namespace OTRoid
{
	namespace OTR
	{
		int IsLoggedIn (void *opData, const char *accName,
				const char*, const char *recipient)
		{
			return static_cast<Plugin*> (opData)->IsLoggedIn (QString::fromUtf8 (accName),
					QString::fromUtf8 (recipient));
		}

		void InjectMessage (void *opData, const char *accName,
				const char*, const char *recipient, const char *msg)
		{
			static_cast<Plugin*> (opData)->InjectMsg (QString::fromUtf8 (accName),
					QString::fromUtf8 (recipient),
					QString::fromUtf8 (msg),
					true,
					IMessage::DOut);
		}

		void Notify (void *opData, OtrlNotifyLevel level,
				const char *accountname, const char *protocol,
				const char *username, const char *title,
				const char *primary, const char *secondary)
		{
			auto u = [] (const char *cs) { return QString::fromUtf8 (cs); };

			Priority prio = PInfo_;
			switch (level)
			{
			case OTRL_NOTIFY_ERROR:
				prio = PCritical_;
				break;
			case OTRL_NOTIFY_WARNING:
				prio = PWarning_;
				break;
			case OTRL_NOTIFY_INFO:
				prio = PInfo_;
				break;
			}

			static_cast<Plugin*> (opData)->Notify (u (accountname),
					u (username), prio, u (title), u (primary), u (secondary));
		}

#if OTRL_VERSION_MAJOR >= 4
		void HandleMsgEvent (void *opData, OtrlMessageEvent event,
				ConnContext *context, const char *message, gcry_error_t)
		{
			qDebug () << Q_FUNC_INFO
					<< event
					<< message;

			auto plugin = static_cast<Plugin*> (opData);

			const auto& contact = plugin->GetVisibleEntryName (QString::fromUtf8 (context->accountname),
					QString::fromUtf8 (context->username));

			QString msg;
			switch (event)
			{
			case OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED:
				msg = Plugin::tr ("The following message received from %1 "
								   "was not encrypted:").arg (contact);
				break;
			case OTRL_MSGEVENT_CONNECTION_ENDED:
				msg = Plugin::tr ("Your message was not sent. Either end your "
								   "private conversation, or restart it.");
				break;
			case OTRL_MSGEVENT_RCVDMSG_UNRECOGNIZED:
				msg = Plugin::tr ("Unreadable encrypted message was received.");
				break;
			case OTRL_MSGEVENT_RCVDMSG_NOT_IN_PRIVATE:
				msg = Plugin::tr ("Received an encrypted message but it cannot "
								   "be read because no private connection is "
								   "established yet.");
				break;
			case OTRL_MSGEVENT_RCVDMSG_UNREADABLE:
				msg = Plugin::tr ("Received message is unreadable.");
				break;
			case OTRL_MSGEVENT_RCVDMSG_MALFORMED:
				msg = Plugin::tr ("Received message contains malformed data.");
				break;
			}

			if (!msg.isEmpty ())
			{
				plugin->InjectMsg (QString::fromUtf8 (context->accountname),
						QString::fromUtf8 (context->username),
						msg, false, IMessage::DIn,
						IMessage::MTServiceMessage);
			}
		}

		void TimerControl (void *opData, unsigned int interval)
		{
			static_cast<Plugin*> (opData)->SetPollTimerInterval (interval);
		}
#endif

		void HandleNewFingerprint (void *opData, OtrlUserState,
				const char *accountname, const char *protocol,
				const char *username, unsigned char fingerprint [20])
		{
			char fpHash [45];
			otrl_privkey_hash_to_human (fpHash, fingerprint);
			QString hrHash (fpHash); // human readable fingerprint

			const auto plugin = static_cast<Plugin*> (opData);

			const auto& msg = Plugin::tr ("You have received a new fingerprint from %1: %2")
					.arg (plugin->GetVisibleEntryName (QString::fromUtf8 (accountname), QString::fromUtf8 (username)))
					.arg (hrHash);
			plugin->InjectMsg (QString::fromUtf8 (accountname),
					QString::fromUtf8 (username),
					msg, false, IMessage::DIn, IMessage::MTServiceMessage);
		}

		void HandleGoneSecure (void *opData, ConnContext *context)
		{
			const auto& msg = Plugin::tr ("Private conversation started");
			static_cast<Plugin*> (opData)->
					InjectMsg (QString::fromUtf8 (context->accountname),
							QString::fromUtf8 (context->username),
							msg, false, IMessage::DIn, IMessage::MTServiceMessage);
		}

		void HandleGoneInsecure (void *opData, ConnContext *context)
		{
			const auto& msg = Plugin::tr ("Private conversation lost");
			static_cast<Plugin*> (opData)->
					InjectMsg (QString::fromUtf8 (context->accountname),
							QString::fromUtf8 (context->username),
							msg, false, IMessage::DIn, IMessage::MTServiceMessage);
		}

		void HandleStillSecure (void *opData, ConnContext *context, int)
		{
			const auto& msg = QObject::tr ("Private conversation refreshed");
			static_cast<Plugin*> (opData)->
					InjectMsg (QString::fromUtf8 (context->accountname),
							QString::fromUtf8 (context->username),
							msg, false, IMessage::DIn, IMessage::MTServiceMessage);
		}
	}

	void Plugin::Init (ICoreProxy_ptr)
	{
		Util::InstallTranslator ("azoth_otroid");

		OTRL_INIT;

		OtrDir_ = Util::CreateIfNotExists ("azoth/otr/");

		UserState_ = otrl_userstate_create ();

		otrl_privkey_read (UserState_, GetOTRFilename ("privkey"));
		otrl_privkey_read_fingerprints (UserState_,
				GetOTRFilename ("fingerprints"), NULL, NULL);

		memset (&OtrOps_, 0, sizeof (OtrOps_));
		OtrOps_.policy = [] (void*, ConnContext*) { return OtrlPolicy { OTRL_POLICY_DEFAULT }; };
		OtrOps_.create_privkey = [] (void *opData, const char *accName, const char *proto)
				{ static_cast<Plugin*> (opData)->CreatePrivkey (accName, proto); };
		OtrOps_.is_logged_in = &OTR::IsLoggedIn;
		OtrOps_.inject_message = &OTR::InjectMessage;
		OtrOps_.update_context_list = [] (void*) {};
		OtrOps_.new_fingerprint = &OTR::HandleNewFingerprint;
		OtrOps_.write_fingerprints = [] (void *opData)
				{ static_cast<Plugin*> (opData)->WriteFingerprints (); };
		OtrOps_.account_name = [] (void *opData, const char *acc, const char*) -> const char*
				{
					const auto& name = static_cast<Plugin*> (opData)->
							GetAccountName (QString::fromUtf8 (acc)).toUtf8 ();

					const char *orig = name.constData ();
					char *result = new char [name.size ()];
					std::strncpy (result, orig, name.size ());
					return result;
				};
		OtrOps_.account_name_free = [] (void*, const char *name) { delete [] name; };
		OtrOps_.gone_secure = &OTR::HandleGoneSecure;
		OtrOps_.gone_insecure = &OTR::HandleGoneInsecure;
		OtrOps_.still_secure = &OTR::HandleStillSecure;
#if OTRL_VERSION_MAJOR >= 4
		OtrOps_.handle_msg_event = &OTR::HandleMsgEvent;
		OtrOps_.timer_control = &OTR::TimerControl;

		PollTimer_ = new QTimer (this);
		connect (PollTimer_,
				SIGNAL (timeout ()),
				this,
				SLOT (pollOTR ()));

		SetPollTimerInterval (otrl_message_poll_get_default_interval (UserState_));
#else
		OtrOps_.notify = &OTR::Notify;
		OtrOps_.log_message = [] (void*, const char *msg)
				{ qDebug () << "OTR:" << QString::fromUtf8 (msg).trimmed (); };
		OtrOps_.display_otr_message = [] (void *opData, const char *accountname,
				const char *protocol, const char *username, const char *msg) -> int
			{
				static_cast<Plugin*> (opData)->InjectMsg (QString::fromUtf8 (accountname),
						QString::fromUtf8 (username),
						QString::fromUtf8 (msg), false, IMessage::DIn);
				return 0;
			};
#endif
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Azoth.OTRoid";
	}

	void Plugin::Release ()
	{
		otrl_userstate_free (UserState_);
	}

	QString Plugin::GetName () const
	{
		return "Azoth OTRoid";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Azoth OTRoid adds support for Off-the-Record deniable encryption system.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/plugins/azoth/plugins/otroid/resources/images/otroid.svg");
		return icon;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Plugins.Azoth.Plugins.IGeneralPlugin";
		return result;
	}

	int Plugin::IsLoggedIn (const QString& accId, const QString& entryId)
	{
		QObject *entryObj = AzothProxy_->GetEntry (entryId, accId);
		ICLEntry *entry = qobject_cast<ICLEntry*> (entryObj);

		if (!entry)
			return -1;

		return entry->Variants ().isEmpty () ? 0 : 1;
	}

	void Plugin::InjectMsg (const QString& accId, const QString& entryId,
			const QString& body, bool hidden, IMessage::Direction dir, IMessage::MessageType type)
	{
		QObject *entryObj = AzothProxy_->GetEntry (entryId, accId);
		ICLEntry *entry = qobject_cast<ICLEntry*> (entryObj);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "no such entry"
					<< entryId
					<< accId;
			return;
		}

		InjectMsg (entry, body, hidden, dir, type);
	}

	void Plugin::InjectMsg (ICLEntry *entry, const QString& body, bool hidden,
			IMessage::Direction dir, IMessage::MessageType type)
	{
		if (dir == IMessage::DOut)
		{
			QObject *msgObj = entry->CreateMessage (type, {}, body);
			if (hidden)
				msgObj->setProperty ("Azoth/HiddenMessage", true);

			IMessage *msg = qobject_cast<IMessage*> (msgObj);
			if (!msg)
				return;

			msg->Send ();
		}
		else
		{
			auto entryObj = entry->GetQObject ();
			auto msgObj = AzothProxy_->CreateCoreMessage (body,
					QDateTime::currentDateTime (),
					type, dir, entryObj, entryObj);

			PendingInjectedMessages_ << msgObj;

			auto msg = qobject_cast<IMessage*> (msgObj);
			msg->Store ();
		}
	}

	void Plugin::Notify (const QString&, const QString&,
			Priority prio, const QString& title,
			const QString& prim, const QString& sec)
	{
		QString text = prim;
		if (!sec.isEmpty ())
			text += "<br />" + sec;

		emit gotEntity (Util::MakeNotification (title, text, prio));
	}

	void Plugin::WriteFingerprints ()
	{
		otrl_privkey_write_fingerprints (UserState_, GetOTRFilename ("fingerprints"));
	}

	QString Plugin::GetAccountName (const QString& accId)
	{
		QObject *accObj = AzothProxy_->GetAccount (accId);
		IAccount *acc = qobject_cast<IAccount*> (accObj);
		if (!acc)
		{
			qWarning () << Q_FUNC_INFO
					<< "empty account for"
					<< accId
					<< accObj;
			return QString ();
		}

		return acc->GetAccountName ();
	}

	namespace
	{
		QString GetVisibleEntryNameImpl (ICLEntry *entry)
		{
			const auto& id = entry->GetHumanReadableID ();
			const auto& name = entry->GetEntryName ();
			return name != id ?
					QString ("%1 (%2)").arg (name).arg (id) :
					id;
		}
	}

	QString Plugin::GetVisibleEntryName (const QString& accId, const QString& entryId)
	{
		QObject *entryObj = AzothProxy_->GetEntry (entryId, accId);
		auto entry = qobject_cast<ICLEntry*> (entryObj);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "no such entry"
					<< entryId
					<< accId;
			return entryId;
		}

		return GetVisibleEntryNameImpl (entry);
	}

	void Plugin::CreatePrivkey (const char *accName, const char *proto)
	{
		if (IsGenerating_)
			return;

		if (QMessageBox::question (nullptr,
				"Azoth OTRoid",
				tr ("Private keys for account %1 need to be generated. This takes quite some "
					"time (from a few seconds to a couple of minutes), and while you can use "
					"LeechCraft in the meantime, all the messages will be sent unencrypted "
					"until keys are generated. You will be notified when this process finishes. "
					"Do you want to generate keys now?"
					"<br /><br />You can also move mouse randomily to help generating entropy.")
					.arg (GetAccountName (QString::fromUtf8 (accName))),
				QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;

		IsGenerating_ = true;

		QEventLoop loop;
		QFutureWatcher<gcry_error_t> watcher;
		connect (&watcher,
				SIGNAL (finished ()),
				&loop,
				SLOT (quit ()));
		auto future = QtConcurrent::run (otrl_privkey_generate,
				UserState_, GetOTRFilename ("privkey"), accName, proto);
		watcher.setFuture (future);

		loop.exec ();

		IsGenerating_ = false;

		char fingerprint [45];
		if (!otrl_privkey_fingerprint (UserState_, fingerprint, accName, proto))
		{
			qWarning () << Q_FUNC_INFO
					<< "failed";
			return;
		}

		QMessageBox::information (nullptr,
				"Azoth OTRoid",
				tr ("Keys are generated. Thanks for your patience."));
	}

#if OTRL_VERSION_MAJOR >= 4
	void Plugin::SetPollTimerInterval (unsigned int seconds)
	{
		if (PollTimer_->isActive ())
			PollTimer_->stop ();

		if (seconds)
			PollTimer_->start (seconds * 1000);
	}
#endif

	void Plugin::initPlugin (QObject *obj)
	{
		AzothProxy_ = qobject_cast<IProxyObject*> (obj);
	}

	void Plugin::hookEntryActionAreasRequested (IHookProxy_ptr proxy,
			QObject *action, QObject*)
	{
		if (!action->property ("Azoth/OTRoid/IsGood").toBool ())
			return;

		const QStringList ours { "contactListContextMenu", "tabContextMenu", "toolbar" };
		proxy->SetReturnValue (proxy->GetReturnValue ().toStringList () + ours);
	}

	void Plugin::hookEntryActionsRemoved (IHookProxy_ptr,
			QObject *entry)
	{
		auto act = Entry2Action_.take (entry);
		Action2Entry_.remove (act);
		delete act;
	}

	void Plugin::hookEntryActionsRequested (IHookProxy_ptr proxy, QObject *entry)
	{
		if (qobject_cast<ICLEntry*> (entry)->GetEntryType () == ICLEntry::ETMUC)
			return;

		if (!Entry2Action_.contains (entry))
			CreateActions (entry);

		QList<QVariant> list = proxy->GetReturnValue ().toList ();
		list << QVariant::fromValue<QObject*> (Entry2Action_ [entry]);
		proxy->SetReturnValue (list);
	}

	void Plugin::hookGotMessage (IHookProxy_ptr proxy, QObject *msgObj)
	{
		if (IsGenerating_)
			return;

		if (PendingInjectedMessages_.remove (msgObj))
			return;

		IMessage *msg = qobject_cast<IMessage*> (msgObj);
		if (!msg)
		{
			qWarning () << Q_FUNC_INFO
					<< msgObj
					<< "doesn't implement IMessage";
			return;
		}

		if (msg->GetDirection () == IMessage::DOut &&
				Msg2OrigText_.contains (msgObj))
		{
			msg->SetBody (Msg2OrigText_.take (msgObj));
			return;
		}

		if (msg->GetMessageType () != IMessage::MTChatMessage ||
			msg->GetDirection () != IMessage::DIn)
			return;

		QObject *entryObj = msg->ParentCLEntry ();
		ICLEntry *entry = qobject_cast<ICLEntry*> (entryObj);
		if (!entry ||
				entry->GetEntryType () == ICLEntry::ETMUC)
			return;

		IAccount *acc = qobject_cast<IAccount*> (entry->GetParentAccount ());
		IProtocol *proto = qobject_cast<IProtocol*> (acc->GetParentProtocol ());

		char *newMsg = 0;
		OtrlTLV *tlvs = 0;
		int ignore = otrl_message_receiving (UserState_, &OtrOps_, this,
				acc->GetAccountID ().constData (),
				proto->GetProtocolID ().constData (),
				entry->GetEntryID ().toUtf8 ().constData (),
				msg->GetBody ().toUtf8 ().constData (),
				&newMsg,
				&tlvs,
				NULL,
#if OTRL_VERSION_MAJOR >= 4
				NULL,
#endif
				NULL);

		OtrlTLV *tlv = otrl_tlv_find (tlvs, OTRL_TLV_DISCONNECTED);
		if (tlv)
		{
			const auto& message = tr ("%1 has ended the private conversation with you, "
					"you should do the same.")
						.arg (GetVisibleEntryNameImpl (entry));
			InjectMsg (acc->GetAccountID (), entry->GetEntryID (),
						message, false, IMessage::DIn, IMessage::MTServiceMessage);
		}
		otrl_tlv_free (tlvs);

#if (OTRL_VERSION_MAJOR >= 4)
		// Magic hack to force it work similar to libotr < 4.0.0.
		// If user received unencrypted message he (she) should be notified.
		// See OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED as well.
		if (!msg->GetBody ().startsWith("?OTR") && ignore && !newMsg)
			ignore = 0;
#endif

		if (ignore)
		{
			proxy->CancelDefault ();
			msgObj->setProperty ("Azoth/HiddenMessage", true);
		}

		if (newMsg)
		{
			msg->SetBody (QString::fromUtf8 (newMsg));
			otrl_message_free (newMsg);
		}

		if (ignore || newMsg)
		{
			if (!Entry2Action_.contains (entryObj))
				CreateActions (entryObj);
			if (!tlv)
				Entry2Action_ [entryObj]->setChecked (true);
		}

	}

	void Plugin::hookMessageCreated (IHookProxy_ptr proxy, QObject*, QObject *msgObj)
	{
		if (IsGenerating_)
			return;

		IMessage *msg = qobject_cast<IMessage*> (msgObj);
		if (!msg)
		{
			qWarning () << Q_FUNC_INFO
					<< msgObj
					<< "doesn't implement IMessage";
			return;
		}

		QObject *entryObj = msg->OtherPart ();
		if (!Entry2Action_.contains (entryObj) ||
				!Entry2Action_ [entryObj]->isChecked ())
			return;

		ICLEntry *entry = qobject_cast<ICLEntry*> (entryObj);
		IAccount *acc = qobject_cast<IAccount*> (entry->GetParentAccount ());
		IProtocol *proto = qobject_cast<IProtocol*> (acc->GetParentProtocol ());

		char *newMsg = 0;
		const auto err = otrl_message_sending (UserState_,
				&OtrOps_,
				this,
				acc->GetAccountID ().constData (),
				proto->GetProtocolID ().constData (),
				entry->GetEntryID ().toUtf8 ().constData (),
#if OTRL_VERSION_MAJOR >= 4
				OTRL_INSTAG_BEST,
#endif
				msg->GetBody ().toUtf8 ().constData (),
				NULL,
				&newMsg,
#if OTRL_VERSION_MAJOR >= 4
				OTRL_FRAGMENT_SEND_SKIP,
				NULL,
#endif
				NULL,
				NULL);

		if (err)
		{
			qWarning () << Q_FUNC_INFO
					<< "OTR error occured, aborting";
			proxy->CancelDefault ();
		}

		if (newMsg)
		{
			Msg2OrigText_ [msgObj] = msg->GetBody ();
			msg->SetBody (QString::fromUtf8 (newMsg));
		}

		otrl_message_free (newMsg);
	}

	const char* Plugin::GetOTRFilename (const QString& fname) const
	{
		return OtrDir_.absoluteFilePath (fname).toUtf8 ().constData ();
	}

	void Plugin::CreateActions (QObject *entry)
	{
		QAction *otr = new QAction (tr ("Enable OTR"), this);
		otr->setCheckable (true);
		otr->setIcon (GetIcon ());
		otr->setProperty ("Azoth/OTRoid/IsGood", true);
		connect (otr,
				SIGNAL (triggered ()),
				this,
				SLOT (handleOtrAction ()));

		Entry2Action_ [entry] = otr;
		Action2Entry_ [otr] = entry;
	}

	void Plugin::handleOtrAction ()
	{
		auto act = qobject_cast<QAction*> (sender ());

		auto entryObj = Action2Entry_ [act];
		auto entry = qobject_cast<ICLEntry*> (entryObj);
		auto acc = qobject_cast<IAccount*> (entry->GetParentAccount ());
		const auto& accId = acc->GetAccountID ();

		auto proto = qobject_cast<IProtocol*> (acc->GetParentProtocol ());
		const auto& protoId = proto->GetProtocolID ();

		if (!act->isChecked ())
		{
			otrl_message_disconnect (UserState_, &OtrOps_, this,
					accId.constData (), protoId.constData (),
#if OTRL_VERSION_MAJOR >= 4
					entry->GetEntryID ().toUtf8 ().constData (), OTRL_INSTAG_BEST);
#else
					entry->GetEntryID ().toUtf8 ().constData ());
#endif
			const auto& message = tr ("Private conversation closed");
			InjectMsg (acc->GetAccountID (), entry->GetEntryID (),
						message, false, IMessage::DIn, IMessage::MTServiceMessage);
			return;
		}
		else
		{
			const auto& message = tr ("Attempting to start a private conversation");
			InjectMsg (acc->GetAccountID (), entry->GetEntryID (),
					   message, false, IMessage::DIn, IMessage::MTServiceMessage);
		}

		char fingerprint [45];
		if (!otrl_privkey_fingerprint (UserState_, fingerprint,
				accId.constData (), protoId.constData ()))
			CreatePrivkey (accId.constData (), protoId.constData());

		std::shared_ptr<char> msg (otrl_proto_default_query_msg (accId.constData (),
#if OTRL_VERSION_MAJOR >= 4
					OTRL_POLICY_ALLOW_V2), free);
		// Yes, this is a malicious hack. And in the bright future
		// (OTRL_POLICY_ALLOW_V3 | OTRL_POLICY_ALLOW_V2) or OTRL_POLICY_DEFAULT
		// should be used. But for now this is only possible solution for fixing
		// the problem of initialization of private conversation when both sides
		// use libotr 4.0.x.
#else
					OTRL_POLICY_DEFAULT), free);
#endif
		InjectMsg (entry, QString::fromUtf8 (msg.get ()), true, IMessage::DOut);
	}

#if OTRL_VERSION_MAJOR >= 4
	void Plugin::pollOTR ()
	{
		otrl_message_poll (UserState_, &OtrOps_, this);
	}
#endif
}
}
}

LC_EXPORT_PLUGIN (leechcraft_azoth_otroid, LeechCraft::Azoth::OTRoid::Plugin);
