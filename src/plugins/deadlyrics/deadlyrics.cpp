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

#include "deadlyrics.h"
#include <QIcon>
#include <util/util.h>
#include "sitessearcher.h"

namespace LeechCraft
{
namespace DeadLyrics
{
	void DeadLyRicS::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("deadlyrics");

		Proxy_ = proxy;

		Searchers_ << Searcher_ptr (new SitesSearcher (":/deadlyrics/resources/sites.xml", proxy));
		for (auto searcher : Searchers_)
			connect (searcher.get (),
					SIGNAL (gotLyrics (Media::LyricsResults)),
					this,
					SIGNAL (gotLyrics (Media::LyricsResults)));
	}

	void DeadLyRicS::SecondInit ()
	{
	}

	void DeadLyRicS::Release ()
	{
		Searchers_.clear ();
	}

	QByteArray DeadLyRicS::GetUniqueID () const
	{
		return "org.LeechCraft.DeadLyrics";
	}

	QString DeadLyRicS::GetName () const
	{
		return "DeadLyRicS";
	}

	QString DeadLyRicS::GetInfo () const
	{
		return tr ("Song lyrics searcher.");
	}

	QIcon DeadLyRicS::GetIcon () const
	{
		static QIcon icon ("lcicons:/deadlyrics/resources/images/deadlyrics.svg");
		return icon;
	}

	void DeadLyRicS::RequestLyrics (const Media::LyricsQuery& query, Media::QueryOptions options)
	{
		if (query.Artist_.isEmpty () || query.Title_.isEmpty ())
			return;

		for (auto searcher : Searchers_)
			searcher->Search (query, options);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_deadlyrics, LeechCraft::DeadLyrics::DeadLyRicS);
