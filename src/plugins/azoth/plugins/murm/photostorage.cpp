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

#include "photostorage.h"
#include <QUrl>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFuture>
#include <QFutureInterface>
#include <QtDebug>
#include <util/sll/queuemanager.h>
#include <util/sll/slotclosure.h>
#include <util/sys/paths.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Murm
{
	PhotoStorage::PhotoStorage (QNetworkAccessManager *nam, const QString& subpath, QObject *parent)
	: QObject (parent)
	, NAM_ (nam)
	, FetchQueue_ (new Util::QueueManager (100, this))
	, StorageDir_ (Util::CreateIfNotExists ("azoth/murm/photo/" + subpath))
	{
	}

	namespace
	{
		QString Url2Filename (const QUrl& url)
		{
			auto result = url.toString ();
			result.replace ('/', '_');
			result.replace (':', '_');
			return result;
		}

		void HandleReplyFinished (QNetworkReply *reply,
				const QString& filename, QFutureInterface<QImage> iface)
		{
			const auto& data = reply->readAll ();

			reply->deleteLater ();

			const auto& image = QImage::fromData (data);
			iface.reportResult (&image);

			QFile file { filename };
			if (!file.open (QIODevice::WriteOnly))
			{
				qWarning () << Q_FUNC_INFO
						<< "error opening file for"
						<< reply->request ().url ()
						<< filename
						<< file.errorString ();
				return;
			}

			file.write (data);
		}
	}

	QFuture<QImage> PhotoStorage::GetImage (const QUrl& url)
	{
		QFutureInterface<QImage> iface;
		iface.reportStarted ();

		const auto& filename = Url2Filename (url);
		if (StorageDir_.exists (filename))
		{
			QImage image { StorageDir_.absoluteFilePath (filename) };
			if (!image.isNull ())
			{
				iface.reportFinished (&image);
				return iface.future ();
			}
		}

		if (Pending_.contains (url))
			return Pending_.value (url);

		const auto& future = iface.future ();
		Pending_ [url] = future;
		FetchQueue_->Schedule ([=] () mutable
				{
					const auto& reply = NAM_->get (QNetworkRequest (url));
					new Util::SlotClosure<Util::DeleteLaterPolicy>
					{
						[=]
						{
							Pending_.remove (url);
							HandleReplyFinished (reply,
									StorageDir_.absoluteFilePath (filename),
									iface);
						},
						reply,
						SIGNAL (finished ()),
						reply
					};
				});
		return future;
	}
}
}
}
