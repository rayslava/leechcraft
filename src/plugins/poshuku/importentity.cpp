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

#include "importentity.h"
#include <QMainWindow>
#include <QProgressDialog>
#include <interfaces/structures.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/irootwindowsmanager.h>
#include "core.h"

namespace LeechCraft
{
namespace Poshuku
{
	void ImportEntity (const Entity& e)
	{
		qDebug () << Q_FUNC_INFO;
		auto rootWM = Core::Instance ().GetProxy ()->GetRootWindowsManager ();

		QList<QVariant> history = e.Additional_ ["BrowserHistory"].toList ();
		if (history.size ())
		{
			QProgressDialog progressDia (QObject::tr ("Importing history..."),
					QObject::tr ("Abort history import"),
					0, history.size (),
					rootWM->GetPreferredWindow ());
			int cur = 0;
			qDebug () << "History:" << history.size ();
			Q_FOREACH (const QVariant& hRowVar, history)
			{
				QMap<QString, QVariant> hRow = hRowVar.toMap ();
				QString title = hRow ["Title"].toString ();
				QString url = hRow ["URL"].toString ();
				QDateTime date = hRow ["DateTime"].toDateTime ();

				if (!date.isValid ())
					qWarning () << "skipping entity with invalid date" << title << url;
				else
					Core::Instance ().GetHistoryModel ()->addItem (title, url, date);

				progressDia.setValue (++cur);
				if (progressDia.wasCanceled ())
					break;
			}
		}

		QList<QVariant> bookmarks = e.Additional_ ["BrowserBookmarks"].toList ();
		if (bookmarks.size ())
		{
			QProgressDialog progressDia (QObject::tr ("Importing bookmarks..."),
					QObject::tr ("Abort bookmarks import"),
					0, bookmarks.size (),
					rootWM->GetPreferredWindow ());
			int cur = 0;
			qDebug () << "Bookmarks" << bookmarks.size ();
			Q_FOREACH (const QVariant& hBMVar, bookmarks)
			{
				QMap<QString, QVariant> hBM = hBMVar.toMap ();
				QString title = hBM ["Title"].toString ();
				QString url = hBM ["URL"].toString ();
				QStringList tags = hBM ["Tags"].toStringList ();

				Core::Instance ().GetFavoritesModel ()->addItem (title, url, tags);
				progressDia.setValue (++cur);
				if (progressDia.wasCanceled ())
					break;
			}
		}
	}
}
}
