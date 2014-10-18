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

#include "notificationsmanager.h"
#include <QMainWindow>
#include <util/xpc/util.h>
#include <util/xpc/notificationactionhandler.h>
#include <util/xpc/defaulthookproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/irootwindowsmanager.h>
#include <interfaces/an/constants.h>
#include <interfaces/media/audiostructs.h>
#include "interfaces/azoth/iclentry.h"
#include "interfaces/azoth/iaccount.h"
#include "interfaces/azoth/imessage.h"
#include "interfaces/azoth/iadvancedclentry.h"
#include "interfaces/azoth/iextselfinfoaccount.h"
#include "interfaces/azoth/isupporttune.h"
#include "interfaces/azoth/isupportmood.h"
#include "interfaces/azoth/isupportactivity.h"
#include "interfaces/azoth/isupportgeolocation.h"
#include "interfaces/azoth/ihavecontacttune.h"
#include "interfaces/azoth/ihavecontactmood.h"
#include "xmlsettingsmanager.h"
#include "util.h"
#include "core.h"
#include "joinconferencedialog.h"
#include "chattabsmanager.h"
#include "cltooltipmanager.h"
#include "proxyobject.h"
#include "activitydialog.h"
#include "mooddialog.h"

namespace LeechCraft
{
namespace Azoth
{
	NotificationsManager::NotificationsManager (IEntityManager *manager, QObject *parent)
	: QObject { parent }
	, EntityMgr_ { manager }
	{
	}

	void NotificationsManager::AddAccount (QObject *accObject)
	{
		connect (accObject,
				SIGNAL (itemSubscribed (QObject*, const QString&)),
				this,
				SLOT (handleItemSubscribed (QObject*, const QString&)));
		connect (accObject,
				SIGNAL (itemUnsubscribed (QObject*, const QString&)),
				this,
				SLOT (handleItemUnsubscribed (QObject*, const QString&)));
		connect (accObject,
				SIGNAL (itemUnsubscribed (const QString&, const QString&)),
				this,
				SLOT (handleItemUnsubscribed (const QString&, const QString&)));
		connect (accObject,
				SIGNAL (itemCancelledSubscription (QObject*, const QString&)),
				this,
				SLOT (handleItemCancelledSubscription (QObject*, const QString&)));
		connect (accObject,
				SIGNAL (itemGrantedSubscription (QObject*, const QString&)),
				this,
				SLOT (handleItemGrantedSubscription (QObject*, const QString&)));
		connect (accObject,
				SIGNAL (mucInvitationReceived (QVariantMap, QString, QString)),
				this,
				SLOT (handleMUCInvitation (QVariantMap, QString, QString)));

		connect (accObject,
				SIGNAL (authorizationRequested (QObject*, const QString&)),
				this,
				SLOT (handleAuthorizationRequested (QObject*, const QString&)));

		connect (accObject,
				SIGNAL (statusChanged (const EntryStatus&)),
				this,
				SLOT (handleAccountStatusChanged (const EntryStatus&)));
	}

	void NotificationsManager::RemoveAccount (QObject *accObject)
	{
		disconnect (accObject,
				0,
				this,
				0);
	}

	void NotificationsManager::AddCLEntry (QObject *entryObj)
	{
		connect (entryObj,
				SIGNAL (statusChanged (EntryStatus, QString)),
				this,
				SLOT (handleStatusChanged (EntryStatus, QString)));

		if (qobject_cast<IAdvancedCLEntry*> (entryObj))
		{
			connect (entryObj,
					SIGNAL (attentionDrawn (const QString&, const QString&)),
					this,
					SLOT (handleAttentionDrawn (const QString&, const QString&)));
			connect (entryObj,
					SIGNAL (activityChanged (QString)),
					this,
					SLOT (handleActivityChanged (QString)));
			connect (entryObj,
					SIGNAL (locationChanged (QString)),
					this,
					SLOT (handleLocationChanged (QString)));
		}

		if (qobject_cast<IHaveContactTune*> (entryObj))
			connect (entryObj,
					SIGNAL (tuneChanged (QString)),
					this,
					SLOT (handleTuneChanged (QString)));

		if (qobject_cast<IHaveContactMood*> (entryObj))
			connect (entryObj,
					SIGNAL (moodChanged (QString)),
					this,
					SLOT (handleMoodChanged (QString)));
	}

	void NotificationsManager::RemoveCLEntry (QObject *entryObj)
	{
		disconnect (entryObj,
				0,
				this,
				0);
	}

	void NotificationsManager::HandleMessage (IMessage *msg)
	{
		const bool showMsg = XmlSettingsManager::Instance ()
				.property ("ShowMsgInNotifications").toBool ();

		const auto other = qobject_cast<ICLEntry*> (msg->OtherPart ());
		const auto parentCL = qobject_cast<ICLEntry*> (msg->ParentCLEntry ());

		QString msgString;
		bool isHighlightMsg = false;
		switch (msg->GetMessageType ())
		{
		case IMessage::Type::ChatMessage:
			if (XmlSettingsManager::Instance ()
					.property ("NotifyAboutIncomingMessages").toBool ())
			{
				if (!showMsg)
					msgString = tr ("Incoming chat message from <em>%1</em>.")
							.arg (other->GetEntryName ());
				else
				{
					const auto& body = msg->GetEscapedBody ();
					const auto& notifMsg = body.size () > 50 ?
							body.left (50) + "..." :
							body;
					msgString = tr ("Incoming chat message from <em>%1</em>: <em>%2</em>")
							.arg (other->GetEntryName ())
							.arg (notifMsg);
				}
			}
			break;
		case IMessage::Type::MUCMessage:
		{
			isHighlightMsg = Core::Instance ().IsHighlightMessage (msg);
			if (isHighlightMsg && XmlSettingsManager::Instance ()
					.property ("NotifyAboutConferenceHighlights").toBool ())
			{
				if (!showMsg)
					msgString = tr ("Highlighted in conference <em>%1</em> by <em>%2</em>.")
							.arg (parentCL->GetEntryName ())
							.arg (other->GetEntryName ());
				else
				{
					const auto& body = msg->GetEscapedBody ();
					const auto& notifMsg = body.size () > 50 ?
							body.left (50) + "..." :
							body;
					msgString = tr ("Highlighted in conference <em>%1</em> by <em>%2</em>: <em>%3</em>")
							.arg (parentCL->GetEntryName ())
							.arg (other->GetEntryName ())
							.arg (notifMsg);
				}
			}
			break;
		}
		default:
			return;
		}

		auto e = Util::MakeNotification ("Azoth",
				msgString,
				PInfo_);

		if (msgString.isEmpty ())
			e.Mime_ += "+advanced";

		auto entry = msg->GetMessageType () == IMessage::Type::MUCMessage ?
				parentCL :
				other;
		BuildNotification (e, entry);

		const auto count = ++UnreadCounts_ [entry];
		if (msg->GetMessageType () == IMessage::Type::MUCMessage)
		{
			e.Additional_ ["org.LC.Plugins.Azoth.SubSourceID"] = other->GetEntryID ();
			e.Additional_ ["org.LC.AdvNotifications.EventType"] = isHighlightMsg ?
					AN::TypeIMMUCHighlight :
					AN::TypeIMMUCMsg;
			e.Additional_ ["NotificationPixmap"] =
					QVariant::fromValue<QPixmap> (QPixmap::fromImage (other->GetAvatar ()));

			if (isHighlightMsg)
				e.Additional_ ["org.LC.AdvNotifications.FullText"] =
					tr ("%n message(s) from", 0, count) + ' ' + other->GetEntryName () +
							" <em>(" + parentCL->GetEntryName () + ")</em>";
			else
				e.Additional_ ["org.LC.AdvNotifications.FullText"] =
					tr ("%n message(s) in", 0, count) + ' ' + parentCL->GetEntryName ();
		}
		else
		{
			e.Additional_ ["org.LC.AdvNotifications.EventType"] = AN::TypeIMIncMsg;
			e.Additional_ ["org.LC.AdvNotifications.FullText"] =
				tr ("%n message(s) from", 0, count) +
						' ' + other->GetEntryName ();
		}

		e.Additional_ ["org.LC.AdvNotifications.Count"] = count;

		e.Additional_ ["org.LC.AdvNotifications.ExtendedText"] = tr ("%n message(s)", 0, count);
		e.Additional_ ["org.LC.Plugins.Azoth.Msg"] = msg->GetEscapedBody ();

		const auto nh = new Util::NotificationActionHandler { e, this };
		nh->AddFunction (tr ("Open chat"),
				[parentCL] { Core::Instance ().GetChatTabsManager ()->OpenChat (parentCL, true); });
		nh->AddDependentObject (parentCL->GetQObject ());

		EntityMgr_->HandleEntity (e);
	}

	void NotificationsManager::NotifyWithReason (QObject *entryObj, const QString& msg,
			const char *func, const QString& eventType,
			const QString& patternLite, const QString& patternFull)
	{
		const auto entry = qobject_cast<ICLEntry*> (entryObj);
		if (!entry)
		{
			qWarning () << func
					<< entryObj
					<< "doesn't implement ICLEntry";
			return;
		}

		const auto& str = msg.isEmpty () ?
				patternLite
					.arg (entry->GetEntryName ())
					.arg (entry->GetHumanReadableID ()) :
				patternFull
					.arg (entry->GetEntryName ())
					.arg (entry->GetHumanReadableID ())
					.arg (msg);

		auto e = Util::MakeNotification ("Azoth", str, PInfo_);
		BuildNotification (e, entry, "Event");
		e.Additional_ ["org.LC.AdvNotifications.EventType"] = eventType;
		e.Additional_ ["org.LC.AdvNotifications.FullText"] = str;
		e.Additional_ ["org.LC.AdvNotifications.Count"] = 1;
		e.Additional_ ["org.LC.Plugins.Azoth.Msg"] = msg;

		EntityMgr_->HandleEntity (e);
	}

	void NotificationsManager::handleClearUnreadMsgCount (QObject *entryObj)
	{
		const auto entry = qobject_cast<ICLEntry*> (entryObj);
		UnreadCounts_.remove (entry);

		const auto& entryID = entry->GetEntryID ();

		auto e = Util::MakeNotification ("Azoth", {}, PInfo_);
		e.Additional_ ["org.LC.AdvNotifications.SenderID"] = "org.LeechCraft.Azoth";
		e.Additional_ ["org.LC.AdvNotifications.EventID"] =
				"org.LC.Plugins.Azoth.IncomingMessageFrom/" + entryID;
		e.Additional_ ["org.LC.AdvNotifications.EventCategory"] = "org.LC.AdvNotifications.Cancel";

		EntityMgr_->HandleEntity (e);

		e.Additional_ ["org.LC.AdvNotifications.EventID"] =
				"org.LC.Plugins.Azoth.AttentionDrawnBy/" + entryID;

		EntityMgr_->HandleEntity (e);
	}

	void NotificationsManager::handleItemSubscribed (QObject *entryObj, const QString& msg)
	{
		if (!XmlSettingsManager::Instance ()
				.property ("NotifySubscriptions").toBool ())
			return;

		NotifyWithReason (entryObj, msg, Q_FUNC_INFO,
				AN::TypeIMSubscrSub,
				tr ("%1 (%2) subscribed to us."),
				tr ("%1 (%2) subscribed to us: %3."));
	}

	void NotificationsManager::handleItemUnsubscribed (QObject *entryObj, const QString& msg)
	{
		if (!XmlSettingsManager::Instance ()
				.property ("NotifyUnsubscriptions").toBool ())
			return;

		NotifyWithReason (entryObj, msg, Q_FUNC_INFO,
				AN::TypeIMSubscrUnsub,
				tr ("%1 (%2) unsubscribed from us."),
				tr ("%1 (%2) unsubscribed from us: %3."));
	}

	/** @todo Option for disabling notifications of unsubscription events from
		* non-roster items.
		*/
	void NotificationsManager::handleItemUnsubscribed (const QString& entryId, const QString& msg)
	{
		if (!XmlSettingsManager::Instance ()
				.property ("NotifyAboutNonrosterUnsub").toBool ())
			return;

		const auto& str = msg.isEmpty () ?
				tr ("%1 unsubscribed from us.")
					.arg (entryId) :
				tr ("%1 unsubscribed from us: %2.")
					.arg (entryId)
					.arg (msg);
		EntityMgr_->HandleEntity (Util::MakeNotification ("Azoth", str, PInfo_));
	}

	void NotificationsManager::handleItemCancelledSubscription (QObject *entryObj, const QString& msg)
	{
		if (!XmlSettingsManager::Instance ()
				.property ("NotifySubCancels").toBool ())
			return;

		NotifyWithReason (entryObj, msg, Q_FUNC_INFO,
				AN::TypeIMSubscrRevoke,
				tr ("%1 (%2) cancelled our subscription."),
				tr ("%1 (%2) cancelled our subscription: %3."));
	}

	void NotificationsManager::handleItemGrantedSubscription (QObject *entryObj, const QString& msg)
	{
		if (!XmlSettingsManager::Instance ()
				.property ("NotifySubGrants").toBool ())
			return;

		NotifyWithReason (entryObj, msg, Q_FUNC_INFO,
				AN::TypeIMSubscrGrant,
				tr ("%1 (%2) granted subscription."),
				tr ("%1 (%2) granted subscription: %3."));
	}

	void NotificationsManager::handleAccountStatusChanged (const EntryStatus& status)
	{
		const auto acc = qobject_cast<IAccount*> (sender ());

		if (status.State_ == SOffline)
			LastAccountStatusChange_.remove (acc);
		else if (!LastAccountStatusChange_.contains (acc))
			LastAccountStatusChange_ [acc] = QDateTime::currentDateTime ();
	}

	void NotificationsManager::handleStatusChanged (const EntryStatus& entrySt, const QString& variant)
	{
		const auto entry = qobject_cast<ICLEntry*> (sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "sender is not a ICLEntry:"
					<< sender ();
			return;
		}

		const auto acc = entry->GetParentAccount ();
		if (!LastAccountStatusChange_.contains (acc) ||
				LastAccountStatusChange_ [acc].secsTo (QDateTime::currentDateTime ()) < 5)
			return;

		const auto extAcc = qobject_cast<IExtSelfInfoAccount*> (entry->GetParentAccount ()->GetQObject ());
		if (extAcc &&
				extAcc->GetSelfContact () == entry->GetQObject ())
			return;

		ProxyObject azothProxy;

		const auto& name = entry->GetEntryName ();
		const auto& status = CLTooltipManager::Status2Str (entrySt, &azothProxy);

		const auto& text = variant.isEmpty () ?
				Core::tr ("%1 is now %2.")
					.arg (name)
					.arg (status) :
				Core::tr ("%1/%2 is now %3.")
					.arg (name)
					.arg (variant)
					.arg (status);

		auto e = Util::MakeNotification ("LeechCraft", text, PInfo_);
		e.Mime_ += "+advanced";

		BuildNotification (e, entry);
		e.Additional_ ["org.LC.AdvNotifications.EventType"] = AN::TypeIMStatusChange;
		e.Additional_ ["NotificationPixmap"] =
				QVariant::fromValue<QPixmap> (QPixmap::fromImage (entry->GetAvatar ()));

		e.Additional_ ["org.LC.AdvNotifications.FullText"] = text;
		e.Additional_ ["org.LC.AdvNotifications.ExtendedText"] = text;
		e.Additional_ ["org.LC.AdvNotifications.Count"] = 1;

		e.Additional_ ["org.LC.Plugins.Azoth.Msg"] = entrySt.StatusString_;
		e.Additional_ ["org.LC.Plugins.Azoth.NewStatus"] = azothProxy.StateToString (entrySt.State_);

		EntityMgr_->HandleEntity (e);
	}

	namespace
	{
		QString GetTuneHRText (ICLEntry *entry, const Media::AudioInfo& info)
		{
			const auto& entryName = entry->GetEntryName ();
			return !info.Title_.isEmpty () ?
					NotificationsManager::tr ("%1 is now listening to %2 by %3")
							.arg ("<em>" + entryName + "</em>")
							.arg ("<em>" + info.Title_ + "</em>")
							.arg ("<em>" + info.Artist_ + "</em>") :
					NotificationsManager::tr ("%1 stopped listening to music")
							.arg (entryName);
		}
	}

	void NotificationsManager::handleTuneChanged (const QString& variant)
	{
		const auto entry = qobject_cast<ICLEntry*> (sender ());

		const auto& info = qobject_cast<IHaveContactTune*> (sender ())->GetUserTune (variant);
		const auto& text = GetTuneHRText (entry, info);

		auto e = Util::MakeNotification ("LeechCraft", text, PInfo_);
		e.Mime_ += "+advanced";

		BuildNotification (e, entry, "TuneChangeEvent");
		e.Additional_ ["org.LC.AdvNotifications.EventType"] = AN::TypeIMEventTuneChange;
		e.Additional_ ["NotificationPixmap"] =
				QVariant::fromValue (QPixmap::fromImage (entry->GetAvatar ()));

		e.Additional_ ["org.LC.AdvNotifications.FullText"] = text;
		e.Additional_ ["org.LC.AdvNotifications.ExtendedText"] = text;
		e.Additional_ ["org.LC.AdvNotifications.Count"] = 1;

		e.Additional_ [AN::Field::MediaArtist] = info.Artist_;
		e.Additional_ [AN::Field::MediaAlbum] = info.Album_;
		e.Additional_ [AN::Field::MediaPlayerURL] = info.Other_ ["URL"];
		e.Additional_ [AN::Field::MediaTitle] = info.Title_;
		e.Additional_ [AN::Field::MediaLength] = info.Length_;

		EntityMgr_->HandleEntity (e);
	}

	namespace
	{
		struct ActivityInfo
		{
			QString General_;
			QString Specific_;
			QString Text_;
		};

		QString GetActivityHRText (ICLEntry *entry, const ActivityInfo& info)
		{
			const auto& entryName = entry->GetEntryName ();
			if (info.General_.isEmpty ())
				return NotificationsManager::tr ("%1 is not doing anything anymore.")
						.arg ("<em>" + entryName + "</em>");

			if (info.Specific_.isEmpty ())
				return NotificationsManager::tr ("%1 is now %2.")
						.arg ("<em>" + entryName + "</em>")
						.arg (ActivityDialog::ToHumanReadable (info.General_));

			return NotificationsManager::tr ("%1 is now %2 (in particular, %3).")
					.arg ("<em>" + entryName + "</em>")
					.arg (ActivityDialog::ToHumanReadable (info.General_))
					.arg (ActivityDialog::ToHumanReadable (info.Specific_));
		}
	}

	void NotificationsManager::handleActivityChanged (const QString& variant)
	{
		const auto entry = qobject_cast<ICLEntry*> (sender ());

		const auto& map = entry->GetClientInfo (variant) ["user_activity"].toMap ();
		const ActivityInfo info
		{
			map ["general"].toString (),
			map ["specific"].toString (),
			map ["text"].toString ()
		};
		const auto& text = GetActivityHRText (entry, info);

		auto e = Util::MakeNotification ("LeechCraft", text, PInfo_);
		e.Mime_ += "+advanced";

		BuildNotification (e, entry, "ActivityChangeEvent");
		e.Additional_ ["org.LC.AdvNotifications.EventType"] = AN::TypeIMEventActivityChange;
		e.Additional_ ["NotificationPixmap"] =
				QVariant::fromValue (QPixmap::fromImage (entry->GetAvatar ()));

		e.Additional_ ["org.LC.AdvNotifications.FullText"] = text;
		e.Additional_ ["org.LC.AdvNotifications.ExtendedText"] = text;
		e.Additional_ ["org.LC.AdvNotifications.Count"] = 1;

		e.Additional_ [AN::Field::IMActivityGeneral] = info.General_;
		e.Additional_ [AN::Field::IMActivitySpecific] = info.Specific_;
		e.Additional_ [AN::Field::IMActivityText] = info.Text_;

		EntityMgr_->HandleEntity (e);
	}

	namespace
	{
		QString GetMoodHRText (ICLEntry *entry, const MoodInfo& info)
		{
			const auto& entryName = entry->GetEntryName ();
			if (info.Mood_.isEmpty ())
				return NotificationsManager::tr ("%1 is not in any particular mood anymore.")
						.arg ("<em>" + entryName + "</em>");

			return NotificationsManager::tr ("%1 is now %2.")
					.arg ("<em>" + entryName + "</em>")
					.arg (MoodDialog::ToHumanReadable (info.Mood_));
		}
	}

	void NotificationsManager::handleMoodChanged (const QString& variant)
	{
		const auto entry = qobject_cast<ICLEntry*> (sender ());
		const auto& info = qobject_cast<IHaveContactMood*> (sender ())->GetUserMood (variant);
		const auto& text = GetMoodHRText (entry, info);

		auto e = Util::MakeNotification ("LeechCraft", text, PInfo_);
		e.Mime_ += "+advanced";

		BuildNotification (e, entry, "MoodChangeEvent");
		e.Additional_ ["org.LC.AdvNotifications.EventType"] = AN::TypeIMEventMoodChange;
		e.Additional_ ["NotificationPixmap"] =
				QVariant::fromValue (QPixmap::fromImage (entry->GetAvatar ()));

		e.Additional_ ["org.LC.AdvNotifications.FullText"] = text;
		e.Additional_ ["org.LC.AdvNotifications.ExtendedText"] = text;
		e.Additional_ ["org.LC.AdvNotifications.Count"] = 1;

		e.Additional_ [AN::Field::IMMoodGeneral] = info.Mood_;
		e.Additional_ [AN::Field::IMMoodText] = info.Text_;

		EntityMgr_->HandleEntity (e);
	}

	namespace
	{
		struct GeolocationInfo
		{
			bool IsValid_;
			double Lon_;
			double Lat_;

			QString Country_;
			QString Locality_;
		};

		QString GetHRLocationText (ICLEntry *entry, const GeolocationInfo& info)
		{
			const auto& entryName = entry->GetEntryName ();
			if (!info.IsValid_)
				return NotificationsManager::tr ("%1's location is not known.")
						.arg (entryName);

			const bool hasCountry = !info.Country_.isEmpty ();
			const bool hasLocality = !info.Locality_.isEmpty ();
			if (hasCountry && hasLocality)
				return NotificationsManager::tr ("%1's is now in %2 (%3).")
						.arg (entryName)
						.arg (info.Locality_)
						.arg (info.Country_);

			if (hasCountry || hasLocality)
				return NotificationsManager::tr ("%1's is now in %2 (%3).")
						.arg (entryName)
						.arg (hasCountry ? info.Country_ : info.Locality_);

			return NotificationsManager::tr ("%1's location updated.")
					.arg (entryName);
		}
	}

	void NotificationsManager::handleLocationChanged (const QString& variant)
	{
		const auto entry = qobject_cast<ICLEntry*> (sender ());
		const auto acc = entry->GetParentAccount ();
		const auto isg = qobject_cast<ISupportGeolocation*> (acc->GetQObject ());
		if (!isg)
		{
			qWarning () << Q_FUNC_INFO
					<< "account"
					<< acc->GetQObject ()
					<< "does not implement ISupportGeolocation";
			return;
		}

		const auto& infoMap = isg->GetUserGeolocationInfo (sender (), variant);
		const auto& info = [&infoMap]
			{
				bool lonOk = false;
				bool latOk = false;

				GeolocationInfo info
				{
					true,
					infoMap ["lon"].toDouble (&lonOk),
					infoMap ["lat"].toDouble (&latOk),
					infoMap ["country"].toString (),
					infoMap ["locality"].toString ()
				};
				info.IsValid_ = lonOk && latOk;
				return info;
			} ();

		const auto& text = GetHRLocationText (entry, info);

		auto e = Util::MakeNotification ("LeechCraft", text, PInfo_);
		e.Mime_ += "+advanced";

		BuildNotification (e, entry, "LocationChangeEvent");
		e.Additional_ ["org.LC.AdvNotifications.EventType"] = AN::TypeIMEventLocationChange;
		e.Additional_ ["NotificationPixmap"] =
				QVariant::fromValue (QPixmap::fromImage (entry->GetAvatar ()));

		e.Additional_ ["org.LC.AdvNotifications.FullText"] = text;
		e.Additional_ ["org.LC.AdvNotifications.ExtendedText"] = text;
		e.Additional_ ["org.LC.AdvNotifications.Count"] = 1;

		e.Additional_ [AN::Field::IMLocationLongitude] = info.Lon_;
		e.Additional_ [AN::Field::IMLocationLatitude] = info.Lat_;

		EntityMgr_->HandleEntity (e);
	}

	void NotificationsManager::handleAttentionDrawn (const QString& text, const QString&)
	{
		if (XmlSettingsManager::Instance ()
				.property ("IgnoreDrawAttentions").toBool ())
			return;

		const auto entry = qobject_cast<ICLEntry*> (sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "doesn't implement ICLEntry";
			return;
		}

		const auto& str = text.isEmpty () ?
				tr ("%1 requests your attention")
					.arg (entry->GetEntryName ()) :
				tr ("%1 requests your attention: %2")
					.arg (entry->GetEntryName ())
					.arg (text);

		auto e = Util::MakeNotification ("Azoth", str, PInfo_);
		BuildNotification (e, entry, "AttentionDrawnBy");
		e.Additional_ ["org.LC.AdvNotifications.DeltaCount"] = 1;
		e.Additional_ ["org.LC.AdvNotifications.EventType"] = AN::TypeIMAttention;
		e.Additional_ ["org.LC.AdvNotifications.ExtendedText"] = tr ("Attention requested");
		e.Additional_ ["org.LC.AdvNotifications.FullText"] = tr ("Attention requested by %1")
				.arg (entry->GetEntryName ());
		e.Additional_ ["org.LC.Plugins.Azoth.Msg"] = text;

		const auto nh = new Util::NotificationActionHandler { e, this };
		nh->AddFunction (tr ("Open chat"),
				[entry, this] { Core::Instance ().GetChatTabsManager ()->OpenChat (entry, true); });
		nh->AddDependentObject (entry->GetQObject ());

		EntityMgr_->HandleEntity (e);
	}

	void NotificationsManager::handleAuthorizationRequested (QObject *entryObj, const QString& msg)
	{
		const auto& proxy = std::make_shared<Util::DefaultHookProxy> ();
		emit hookGotAuthRequest (proxy, entryObj, msg);
		if (proxy->IsCancelled ())
			return;

		const auto entry = qobject_cast<ICLEntry*> (entryObj);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< entryObj
					<< "doesn't implement ICLEntry";
			return;
		}

		const auto& str = msg.isEmpty () ?
				tr ("Subscription requested by %1.")
					.arg (entry->GetEntryName ()) :
				tr ("Subscription requested by %1: %2.")
					.arg (entry->GetEntryName ())
					.arg (msg);
		auto e = Util::MakeNotification ("Azoth", str, PInfo_);

		BuildNotification (e, entry, "AuthRequestFrom");
		e.Additional_ ["org.LC.AdvNotifications.EventType"] = AN::TypeIMSubscrRequest;
		e.Additional_ ["org.LC.AdvNotifications.FullText"] = str;
		e.Additional_ ["org.LC.AdvNotifications.Count"] = 1;
		e.Additional_ ["org.LC.Plugins.Azoth.Msg"] = msg;

		const auto nh = new Util::NotificationActionHandler (e, this);
		nh->AddFunction (tr ("Authorize"), [this, entry] () { AuthorizeEntry (entry); });
		nh->AddFunction (tr ("Deny"), [this, entry] () { DenyAuthForEntry (entry); });
		nh->AddFunction (tr ("View info"), [entry] () { entry->ShowInfo (); });
		nh->AddDependentObject (entry->GetQObject ());
		EntityMgr_->HandleEntity (e);
	}

	namespace
	{
		void SuggestJoiningMUC (IAccount *acc, const QVariantMap& ident)
		{
			const auto rootWM = Core::Instance ().GetProxy ()->GetRootWindowsManager ();
			const auto dia = new JoinConferenceDialog { { acc }, rootWM->GetPreferredWindow () };
			dia->SetIdentifyingData (ident);
			dia->show ();
		}
	}

	void NotificationsManager::handleMUCInvitation (const QVariantMap& ident,
			const QString& inviter, const QString& reason)
	{
		const auto acc = qobject_cast<IAccount*> (sender ());
		if (!acc)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "doesn't implement IAccount";
			return;
		}

		const auto& name = ident ["HumanReadableName"].toString ();

		const auto str = reason.isEmpty () ?
				tr ("You have been invited to %1 by %2.")
					.arg (name)
					.arg (inviter) :
				tr ("You have been invited to %1 by %2: %3")
					.arg (name)
					.arg (inviter)
					.arg (reason);

		auto e = Util::MakeNotification ("Azoth", str, PInfo_);
		e.Additional_ ["org.LC.AdvNotifications.SenderID"] = "org.LeechCraft.Azoth";
		e.Additional_ ["org.LC.AdvNotifications.EventCategory"] = AN::CatIM;
		e.Additional_ ["org.LC.AdvNotifications.VisualPath"] = QStringList (name);
		e.Additional_ ["org.LC.AdvNotifications.EventID"] =
				"org.LC.Plugins.Azoth.Invited/" + name + '/' + inviter;
		e.Additional_ ["org.LC.AdvNotifications.EventType"] = AN::TypeIMMUCInvite;
		e.Additional_ ["org.LC.AdvNotifications.FullText"] = str;
		e.Additional_ ["org.LC.AdvNotifications.Count"] = 1;
		e.Additional_ ["org.LC.Plugins.Azoth.Msg"] = reason;

		const auto& cancel = Util::MakeANCancel (e);

		const auto nh = new Util::NotificationActionHandler { e };
		nh->AddFunction (tr ("Join"), [this, acc, ident, cancel] ()
				{
					SuggestJoiningMUC (acc, ident);
					EntityMgr_->HandleEntity (cancel);
				});
		nh->AddDependentObject (acc->GetQObject ());

		EntityMgr_->HandleEntity (e);
	}
}
}
