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

#include "entrybase.h"
#include <util/util.h>
#include <interfaces/azoth/azothutil.h>
#include "vkaccount.h"
#include "vkmessage.h"
#include "vkconnection.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Murm
{
	EntryBase::EntryBase (VkAccount *acc)
	: QObject (acc)
	, Account_ (acc)
	{
	}

	void EntryBase::Store (VkMessage *msg)
	{
		Messages_ << msg;
		emit gotMessage (msg);
	}

	QObject* EntryBase::GetQObject ()
	{
		return this;
	}

	QObject* EntryBase::GetParentAccount () const
	{
		return Account_;
	}

	QObject* EntryBase::CreateMessage (IMessage::MessageType type, const QString&, const QString& body)
	{
		auto msg = new VkMessage (IMessage::DOut, type, this);
		msg->SetBody (body);
		return msg;
	}

	QList<QObject*> EntryBase::GetAllMessages () const
	{
		QList<QObject*> result;
		for (auto obj : Messages_)
			result << obj;
		return result;
	}

	void EntryBase::PurgeMessages (const QDateTime& before)
	{
		Util::StandardPurgeMessages (Messages_, before);
	}

	namespace
	{
		QString Photo2Replacement (const PhotoInfo& info)
		{
			const auto& fullSizeStr = QString::number (info.FullSize_.width ()) +
					QString::fromUtf8 ("×") +
					QString::number (info.FullSize_.height ());
			return QString ("<a href='%1' target='_blank'><img src='%2' width='%3' height='%4' alt='%5' /></a>")
					.arg (info.Full_)
					.arg (info.Thumbnail_)
					.arg (info.ThumbnailSize_.width ())
					.arg (info.ThumbnailSize_.height ())
					.arg (fullSizeStr);
		}

		QString Audio2Replacement (const AudioInfo& info, ICoreProxy_ptr proxy)
		{
			auto durStr = LeechCraft::Util::MakeTimeFromLong (info.Duration_);
			if (durStr.startsWith ("00:"))
				durStr = durStr.mid (3);

			QUrl azothUrl;
			azothUrl.setScheme ("azoth");
			azothUrl.setHost ("sendentities");
			azothUrl.addQueryItem ("count", "1");
			azothUrl.addQueryItem ("entityVar0", info.URL_.toEncoded ());
			azothUrl.addQueryItem ("entityType0", "url");
			azothUrl.addQueryItem ("addCount0", "1");

			auto enqueueUrl = azothUrl;
			enqueueUrl.addQueryItem ("flags0", "OnlyHandle");
			enqueueUrl.addQueryItem ("add0key0", "Action");
			enqueueUrl.addQueryItem ("add0value0", "AudioEnqueue");

			auto playUrl = azothUrl;
			playUrl.addQueryItem ("flags0", "OnlyHandle");
			playUrl.addQueryItem ("add0key0", "Action");
			playUrl.addQueryItem ("add0value0", "AudioEnqueuePlay");

			auto downloadUrl = azothUrl;
			downloadUrl.addQueryItem ("flags0", "OnlyDownload");

			QString result;

			auto addImage = [&proxy, &result] (const QString& icon, const QString& name) -> void
			{
				const auto& px = proxy->GetIcon (icon).pixmap (16, 16);
				const auto& data = LeechCraft::Util::GetAsBase64Src (px.toImage ());
				result += "<img src='" + data + "' width='16' height='16' alt='" + name + "' title='" + name + "' />";
			};

			result += "<div>";
			result += "<a href='";
			result += QString::fromUtf8 (enqueueUrl.toEncoded ());
			result += "'>";
			addImage ("list-add", EntryBase::tr ("Enqueue"));
			result += "</a> <a href='";
			result += QString::fromUtf8 (playUrl.toEncoded ());
			result += "'>";
			addImage ("media-playback-start", EntryBase::tr ("Play"));
			result += "</a> <a href='";
			result += QString::fromUtf8 (downloadUrl.toEncoded ());
			result += "'>";
			addImage ("download", EntryBase::tr ("Download"));
			result += "</a> ";
			result += info.Artist_ + QString::fromUtf8 (" — ") + info.Title_;
			result += " <span style='float:right'>" + durStr + "</span>";
			result += "</div>";
			return result;
		}
	}

	void EntryBase::HandleAttaches (VkMessage *msg, const MessageInfo& info)
	{
		struct AttachInfo
		{
			QString Type_;
			QString ID_;
		};
		QMap<int, AttachInfo> Attaches_;

		const QString attachMarker ("attach");
		const QString typeMarker ("_type");
		for (auto pos = info.Params_.begin (); pos != info.Params_.end (); ++pos)
		{
			auto key = pos.key ();
			if (!key.startsWith (attachMarker))
				continue;

			key = key.mid (attachMarker.size ());
			const bool isType = key.endsWith (typeMarker);
			if (isType)
				key.chop (typeMarker.size ());

			bool ok = false;
			const auto num = key.toInt (&ok);
			if (!ok)
				continue;

			auto& attach = Attaches_ [num];
			if (isType)
				attach.Type_ = pos->toString ();
			else
				attach.ID_ = pos->toString ();
		}

		QStringList photoIds, wallIds, audioIds;
		for (const auto& info : Attaches_)
			if (info.Type_ == "photo")
				photoIds << info.ID_;
			else if (info.Type_ == "wall")
				wallIds << info.ID_;
			else if (info.Type_ == "audio")
				audioIds << info.ID_;
		if (photoIds.isEmpty () && wallIds.isEmpty () && audioIds.isEmpty ())
			return;

		const QString audioDivStyle = "border-color: #CDCCCC; "
				"margin-top: 2px; margin-bottom: 0px; "
				"border-width: 1px; border-style: solid; border-radius: 5px; "
				"padding-left: 5px; padding-right: 5px; padding-top: 2px; padding-bottom: 2px;";

		QString newContents = msg->GetBody ();
		for (const auto& id : photoIds)
			newContents += "<div id='photostub_" + id + "'></div>";
		for (const auto& id : wallIds)
			newContents += "<div id='wallstub_" + id + "'></div>";
		for (const auto& id : audioIds)
			newContents += "<div id='audiostub_" + id + "' style='" + audioDivStyle + "'></div>";
		msg->SetBody (newContents);

		QPointer<VkMessage> safeMsg (msg);
		Account_->GetConnection ()->GetMessageInfo (msg->GetID (),
				[this, safeMsg, audioDivStyle] (const FullMessageInfo& msgInfo) -> void
				{
					if (!safeMsg)
						return;

					QString js;
					auto body = safeMsg->GetBody ();

					QList<QPair<QString, QString>> replacements;
					for (const auto& info : msgInfo.Photos_)
					{
						const auto& id = QString ("photostub_%1_%2")
								.arg (info.OwnerID_)
								.arg (info.PhotoID_);
						replacements.append ({ id, Photo2Replacement (info) });
					}

					for (const auto& audio : msgInfo.Audios_)
					{
						const auto& id = QString ("audiostub_%1_%2")
								.arg (audio.OwnerID_)
								.arg (audio.ID_);
						replacements.append ({ id,
									Audio2Replacement (audio, Account_->GetCoreProxy ()) });
					}

					for (const auto& repost : msgInfo.ContainedReposts_)
					{
						const auto& id = QString ("wallstub_%1_%2")
								.arg (repost.OwnerID_)
								.arg (repost.ID_);

						auto replacement = repost.Text_;
						for (const auto& photo : repost.Photos_)
							replacement += "<br/>" + Photo2Replacement (photo);

						if (!repost.Audios_.empty ())
						{
							replacement += "<div style='" + audioDivStyle + "'>";
							for (const auto& audio : repost.Audios_)
								replacement += Audio2Replacement (audio,
										Account_->GetCoreProxy ());
							replacement += "</div>";
						}

						replacement += "<div style='text-align:right'>";
						replacement += tr ("Posted on: %1")
								.arg (repost.PostDate_.toString ());
						replacement += "<br/";
						replacement += tr ("%n like(s)", 0, repost.Likes_);
						replacement += "; ";
						replacement += tr ("%n repost(s)", 0, repost.Reposts_);
						replacement += "</div>";

						replacements.append ({ id, replacement });
					}

					for (auto& pair : replacements)
					{
						body.replace ("<div id='" + pair.first + "'></div>",
								"<div>" + pair.second + "</div>");

						pair.second.replace ('\\', "\\\\");
						pair.second.replace ('"', "\\\"");

						js += QString ("try { document.getElementById('%1').innerHTML = \"%2\"; } catch (e) {};")
								.arg (pair.first)
								.arg (pair.second);
					}

					safeMsg->SetBody (body);

					auto safeThis = qobject_cast<EntryBase*> (safeMsg->ParentCLEntry ());
					safeThis->performJS (js);
				});
	}
}
}
}
