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

#include "trmanager.h"
#include <QThread>
#include <QTranslator>
#include <QTimer>
#include <util/util.h>

namespace LeechCraft
{
namespace HttHare
{
	TrManager::TrManager (QObject *parent)
	: QObject (parent)
	{
		auto timer = new QTimer (this);
		connect (timer,
				SIGNAL (timeout ()),
				this,
				SLOT (purge ()));
		timer->start (60 * 60 * 1000);
	}

	QString TrManager::Translate (const QStringList& locales, const char* context, const char* src)
	{
		MapLock_.lock ();
		auto& map = Translators_ [QThread::currentThreadId ()];
		MapLock_.unlock ();

		for (auto locale : locales)
		{
			if (locale.size () > 2)
				locale = locale.left (2);

			if (locale == "ru")
				locale = "ru_RU";

			if (!map.contains (locale))
				map [locale] = Util::LoadTranslator ("htthare", locale);

			if (const auto transl = map [locale])
			{
				const auto& str = transl->translate (context, src);
				if (!str.isEmpty ())
					return str;
			}
		}

		return QString::fromUtf8 (src);
	}

	void TrManager::purge ()
	{
		MapLock_.lock ();
		for (const auto& threadMap : Translators_)
			for (auto tr : threadMap)
				tr->deleteLater ();
		Translators_.clear ();
		MapLock_.unlock ();
	}
}
}
