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

#include <QDataStream>
#include <QtDebug>
#include "feed.h"
#include "channel.h"
#include "core.h"

namespace LeechCraft
{
namespace Aggregator
{
	Feed::FeedSettings::FeedSettings (IDType_t feedId,
			int ut, int ni, int ia, bool ade)
	: SettingsID_ (Core::Instance ().GetPool (PTFeedSettings).GetID ())
	, FeedID_ (feedId)
	, UpdateTimeout_ (ut)
	, NumItems_ (ni)
	, ItemAge_ (ia)
	, AutoDownloadEnclosures_ (ade)
	{
	}

	Feed::FeedSettings::FeedSettings (IDType_t feedId, IDType_t settingsId,
			int ut, int ni, int ia, bool ade)
	: SettingsID_ (settingsId)
	, FeedID_ (feedId)
	, UpdateTimeout_ (ut)
	, NumItems_ (ni)
	, ItemAge_ (ia)
	, AutoDownloadEnclosures_ (ade)
	{
	}
	
	Feed::Feed ()
	: FeedID_ (Core::Instance ().GetPool (PTFeed).GetID ())
	{
	}
	
	Feed::Feed (const IDType_t& feedId)
	: FeedID_ (feedId)
	{
	}

	bool operator< (const Feed& f1, const Feed& f2)
	{
		return f1.URL_ < f2.URL_;
	}
	
	QDataStream& operator<< (QDataStream& out, const Feed& feed)
	{
		out << feed.URL_
			<< feed.LastUpdate_
			<< static_cast<quint32> (feed.Channels_.size ());
		for (quint32 i = 0; i < feed.Channels_.size (); ++i)
			out << *feed.Channels_.at (i);
		return out;
	}
	
	QDataStream& operator>> (QDataStream& in, Feed& feed)
	{
		quint32 size = 0;
		in >> feed.URL_
			>> feed.LastUpdate_
			>> size;
		for (quint32 i = 0; i < size; ++i)
		{
			Channel_ptr chan (new Channel (feed.FeedID_));
			in >> *chan;
			feed.Channels_.push_back (chan);
		}
		return in;
	}
}
}
