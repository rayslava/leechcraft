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

#include "mrimbuddy.h"
#include <functional>
#include <QImage>
#include <QAction>
#include <QInputDialog>
#include <util/util.h>
#include <interfaces/azoth/azothutil.h>
#include "proto/headers.h"
#include "proto/connection.h"
#include "mrimaccount.h"
#include "mrimmessage.h"
#include "vaderutil.h"
#include "groupmanager.h"
#include "smsdialog.h"
#include "core.h"
#include "selfavatarfetcher.h"
#include "vcarddialog.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Vader
{
	MRIMBuddy::MRIMBuddy (const Proto::ContactInfo& info, MRIMAccount *acc)
	: QObject (acc)
	, A_ (acc)
	, Info_ (info)
	, IsAuthorized_ (true)
	, GaveSubscription_ (true)
	, UpdateNumber_ (new QAction (tr ("Update phone number..."), this))
	, SendSMS_ (new QAction (tr ("Send SMS..."), this))
	, AvatarFetcher_ (new SelfAvatarFetcher (this))
	{
		Status_.State_ = VaderUtil::StatusID2State (info.StatusID_);

		SendSMS_->setProperty ("ActionIcon", "phone");
		connect (UpdateNumber_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleUpdateNumber ()));
		connect (SendSMS_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleSendSMS ()));

		connect (AvatarFetcher_,
				SIGNAL (gotImage (QImage)),
				this,
				SLOT (updateAvatar (QImage)));
		AvatarFetcher_->Restart (info.Email_);

		UpdateClientVersion ();
	}

	void MRIMBuddy::HandleMessage (MRIMMessage *msg)
	{
		AllMessages_ << msg;
		emit gotMessage (msg);
	}

	void MRIMBuddy::HandleAttention (const QString& msg)
	{
		emit attentionDrawn (msg, QString ());
	}

	void MRIMBuddy::HandleTune (const QString& tune)
	{
		QVariantMap tuneMap;
		tuneMap ["artist"] = tune;
		ClientInfo_ ["user_tune"] = tuneMap;
		emit tuneChanged (QString ());
	}

	void MRIMBuddy::HandleCPS (ChatPartState cps)
	{
		emit chatPartStateChanged (cps, QString ());
	}

	void MRIMBuddy::SetGroup (const QString& group)
	{
		Info_.GroupNumber_ = A_->GetGroupManager ()->GetGroupNumber (group);
		Group_ = group;
		emit groupsChanged (Groups ());
	}

	void MRIMBuddy::SetAuthorized (bool auth)
	{
		if (auth == IsAuthorized_)
			return;

		IsAuthorized_ = auth;
		if (!IsAuthorized_)
			SetGroup (tr ("Unauthorized"));
		else
			SetGroup (QString ());
	}

	bool MRIMBuddy::IsAuthorized () const
	{
		return IsAuthorized_;
	}

	void MRIMBuddy::SetGaveSubscription (bool gave)
	{
		GaveSubscription_ = gave;
	}

	bool MRIMBuddy::GaveSubscription () const
	{
		return GaveSubscription_;
	}

	Proto::ContactInfo MRIMBuddy::GetInfo () const
	{
		return Info_;
	}

	namespace
	{
		template<typename T, typename V>
		void CmpXchg (Proto::ContactInfo& info, Proto::ContactInfo newInfo,
				T g, V n)
		{
			if (g (info) != g (newInfo))
			{
				g (info) = g (newInfo);
				n (g (info));
			}
		}

		template<typename T, typename U>
		std::function<T& (Proto::ContactInfo&)> GetMem (U g)
		{
			return g;
		}
	}

	void MRIMBuddy::UpdateInfo (const Proto::ContactInfo& info)
	{
		CmpXchg (Info_, info,
				GetMem<QString> (&Proto::ContactInfo::Alias_),
				[this] (QString name) { emit nameChanged (name); });
		CmpXchg (Info_, info,
				GetMem<QString> (&Proto::ContactInfo::UA_),
				[this] (QString)
				{
					UpdateClientVersion ();
					emit entryGenerallyChanged ();
				});

		bool stChanged = false;
		const int oldVars = Variants ().size ();
		CmpXchg (Info_, info,
				GetMem<quint32> (&Proto::ContactInfo::StatusID_),
				[&stChanged] (quint32) { stChanged = true; });
		CmpXchg (Info_, info,
				GetMem<QString> (&Proto::ContactInfo::StatusTitle_),
				[&stChanged] (QString) { stChanged = true; });
		CmpXchg (Info_, info,
				GetMem<QString> (&Proto::ContactInfo::StatusDesc_),
				[&stChanged] (QString) { stChanged = true; });

		if (stChanged)
		{
			Status_.State_ = VaderUtil::StatusID2State (Info_.StatusID_);
			Status_.StatusString_ = Info_.StatusTitle_;

			if (oldVars != Variants ().size ())
				emit availableVariantsChanged (Variants ());
			emit statusChanged (GetStatus (QString ()), QString ());
		}

		Info_.GroupNumber_ = info.GroupNumber_;
	}

	void MRIMBuddy::HandleWPInfo (const QMap<QString, QString>& values)
	{
		VCardDialog *dia = new VCardDialog ();
		dia->setAttribute (Qt::WA_DeleteOnClose);
		dia->SetInfo (values);
		dia->SetAvatar (GetAvatar ());
		dia->show ();
	}

	qint64 MRIMBuddy::GetID () const
	{
		return Info_.ContactID_;
	}

	void MRIMBuddy::UpdateID (qint64 id)
	{
		Info_.ContactID_ = id;
	}

	QObject* MRIMBuddy::GetQObject ()
	{
		return this;
	}

	QObject* MRIMBuddy::GetParentAccount () const
	{
		return A_;
	}

	ICLEntry::Features MRIMBuddy::GetEntryFeatures () const
	{
		return FPermanentEntry | FSupportsGrouping | FSupportsRenames;
	}

	ICLEntry::EntryType MRIMBuddy::GetEntryType () const
	{
		return ETChat;
	}

	QString MRIMBuddy::GetEntryName () const
	{
		return Info_.Alias_.isEmpty () ?
				Info_.Email_ :
				Info_.Alias_;
	}

	void MRIMBuddy::SetEntryName (const QString& name)
	{
		Info_.Alias_ = name;

		A_->GetConnection ()->ModifyContact (GetID (),
				Info_.GroupNumber_, Info_.Email_, name, Info_.Phone_);
		emit nameChanged (name);
	}

	QString MRIMBuddy::GetEntryID () const
	{
		return A_->GetAccountID () + "_" + Info_.Email_;
	}

	QString MRIMBuddy::GetHumanReadableID () const
	{
		return Info_.Email_;
	}

	QStringList MRIMBuddy::Groups () const
	{
		QStringList result;
		if (!Group_.isEmpty ())
			result << Group_;
		return result;
	}

	void MRIMBuddy::SetGroups (const QStringList& list)
	{
		A_->GetGroupManager ()->SetBuddyGroups (this, list);
	}

	QStringList MRIMBuddy::Variants () const
	{
		return Status_.State_ != SOffline ?
				QStringList (QString ()) :
				QStringList ();
	}

	QObject* MRIMBuddy::CreateMessage (IMessage::MessageType,
			const QString&, const QString& body)
	{
		MRIMMessage *msg = new MRIMMessage (IMessage::DOut, IMessage::MTChatMessage, this);
		msg->SetBody (body);
		return msg;
	}

	QList<QObject*> MRIMBuddy::GetAllMessages () const
	{
		QList<QObject*> result;
		Q_FOREACH (auto m, AllMessages_)
			result << m;
		return result;
	}

	void MRIMBuddy::PurgeMessages (const QDateTime& before)
	{
		Util::StandardPurgeMessages (AllMessages_, before);
	}

	void MRIMBuddy::SetChatPartState (ChatPartState state, const QString&)
	{
		A_->SetTypingState (GetHumanReadableID (), state);
	}

	EntryStatus MRIMBuddy::GetStatus (const QString&) const
	{
		return Status_;
	}

	QImage MRIMBuddy::GetAvatar () const
	{
		return Avatar_;
	}

	QString MRIMBuddy::GetRawInfo () const
	{
		return QString ();
	}

	void MRIMBuddy::ShowInfo ()
	{
		A_->RequestInfo (GetHumanReadableID ());
	}

	QList<QAction*> MRIMBuddy::GetActions () const
	{
		return QList<QAction*> () << UpdateNumber_ << SendSMS_;
	}

	QMap<QString, QVariant> MRIMBuddy::GetClientInfo (const QString&) const
	{
		return ClientInfo_;
	}

	void MRIMBuddy::MarkMsgsRead ()
	{
	}

	void MRIMBuddy::ChatTabClosed ()
	{
	}

	IAdvancedCLEntry::AdvancedFeatures MRIMBuddy::GetAdvancedFeatures () const
	{
		return AFSupportsAttention;
	}

	void MRIMBuddy::DrawAttention (const QString& text, const QString&)
	{
		A_->GetConnection ()->SendAttention (GetHumanReadableID (), text);
	}

	void MRIMBuddy::UpdateClientVersion ()
	{
		auto defClient = [this] ()
		{
			ClientInfo_ ["client_type"] = "mailruagent";
			ClientInfo_ ["client_name"] = tr ("Mail.Ru Agent");
			ClientInfo_.remove ("client_version");
		};

		if (Info_.UA_.contains ("leechcraft azoth", Qt::CaseInsensitive))
		{
			ClientInfo_ ["client_type"] = "leechcraft-azoth";
			ClientInfo_ ["client_name"] = "LeechCraft Azoth";

			QString ver = Info_.UA_;
			ver.remove ("leechcraft azoth", Qt::CaseInsensitive);
			ClientInfo_ ["client_version"] = ver.trimmed ();
		}
		else if (Info_.UA_.isEmpty ())
			defClient ();
		else
		{
			qWarning () << Q_FUNC_INFO << "unknown client" << Info_.UA_;

			defClient ();
		}
	}

	void MRIMBuddy::updateAvatar (const QImage& image)
	{
		Avatar_ = image;
		emit avatarChanged (Avatar_);
	}

	void MRIMBuddy::handleUpdateNumber ()
	{
		const auto& num = QInputDialog::getText (0,
				tr ("Update number"),
				tr ("Enter new number in international format:"),
				QLineEdit::Normal,
				Info_.Phone_);
		if (num.isEmpty () || num == Info_.Phone_)
			return;

		Info_.Phone_ = num;
		A_->GetConnection ()->ModifyContact (GetID (),
				Info_.GroupNumber_, Info_.Email_, Info_.Alias_, Info_.Phone_);
	}

	void MRIMBuddy::handleSendSMS ()
	{
		SMSDialog dia (Info_.Phone_);
		if (dia.exec () != QDialog::Accepted)
			return;

		auto conn = A_->GetConnection ();
		const QString& phone = dia.GetPhone ();
		SentSMS_ [conn->SendSMS2Number (phone, dia.GetText ())] = phone;

		connect (conn,
				SIGNAL (smsDelivered (quint32)),
				this,
				SLOT (handleSMSDelivered (quint32)),
				Qt::UniqueConnection);
		connect (conn,
				SIGNAL (smsBadParms (quint32)),
				this,
				SLOT (handleSMSBadParms (quint32)),
				Qt::UniqueConnection);
		connect (conn,
				SIGNAL (smsServiceUnavailable (quint32)),
				this,
				SLOT (handleSMSServUnavail (quint32)),
				Qt::UniqueConnection);
	}

	void MRIMBuddy::handleSMSDelivered (quint32 seq)
	{
		if (!SentSMS_.contains (seq))
			return;

		Core::Instance ().SendEntity (LeechCraft::Util::MakeNotification ("Azoth",
					tr ("SMS has been sent to %1.")
						.arg (SentSMS_.take (seq)),
				PInfo_));
	}

	void MRIMBuddy::handleSMSBadParms (quint32 seq)
	{
		if (!SentSMS_.contains (seq))
			return;

		Core::Instance ().SendEntity (LeechCraft::Util::MakeNotification ("Azoth",
					tr ("Failed to send SMS to %1: bad parameters.")
						.arg (SentSMS_.take (seq)),
				PCritical_));
	}

	void MRIMBuddy::handleSMSServUnavail (quint32 seq)
	{
		if (!SentSMS_.contains (seq))
			return;

		Core::Instance ().SendEntity (LeechCraft::Util::MakeNotification ("Azoth",
					tr ("Failed to send SMS to %1: service unavailable.")
						.arg (SentSMS_.take (seq)),
				PCritical_));
	}
}
}
}
