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

#include <QtDebug>
#include <QDataStream>
#include <QVariant>
#include <QStringList>
#include "channel.h"
#include "item.h"
#include "core.h"

namespace LeechCraft
{
namespace Aggregator
{
	Channel::Channel (const IDType_t& id)
	: ChannelID_ (Core::Instance ().GetPool (PTChannel).GetID ())
	, FeedID_ (id)
	{
	}

	Channel::Channel (const IDType_t& id, const IDType_t& chId)
	: ChannelID_ (chId)
	, FeedID_ (id)
	{
	}

	Channel::Channel (const Channel& channel)
	: Items_ (channel.Items_)
	{
		Equalify (channel);
	}

	Channel& Channel::operator= (const Channel& channel)
	{
		Equalify (channel);
		Items_ = channel.Items_;
		return *this;
	}

	int Channel::CountUnreadItems () const
	{
		int result = 0;
		for (size_t i = 0; i < Items_.size (); ++i)
			result += (Items_ [i]->Unread_);
		return result;
	}

	void Channel::Equalify (const Channel& channel)
	{
		ChannelID_ = channel.ChannelID_;
		FeedID_ = channel.FeedID_;
		Title_ = channel.Title_;
		Link_ = channel.Link_;
		Description_ = channel.Description_;
		LastBuild_ = channel.LastBuild_;
		Tags_ = channel.Tags_;
		Language_ = channel.Language_;
		Author_ = channel.Author_;
		PixmapURL_ = channel.PixmapURL_;
		Pixmap_ = channel.Pixmap_;
		Favicon_ = channel.Favicon_;
	}

	ChannelShort Channel::ToShort () const
	{
		ChannelShort cs =
		{
			ChannelID_,
			FeedID_,
			Author_,
			Title_,
			Link_,
			Tags_,
			LastBuild_,
			Favicon_,
			CountUnreadItems ()
		};
		return cs;
	}

	bool operator< (const ChannelShort& cs1, const ChannelShort& cs2)
	{
		return cs1.ChannelID_ < cs2.ChannelID_;
	}

	bool operator== (const ChannelShort& cs1, const ChannelShort& cs2)
	{
		return cs1.ChannelID_ == cs2.ChannelID_;
	}

	bool operator== (const Channel_ptr& ch, const ChannelShort& cs)
	{
		return ch->ChannelID_ == cs.ChannelID_;
	}

	bool operator== (const ChannelShort& cs, const Channel_ptr& ch)
	{
		return ch == cs;
	}

	bool operator== (const Channel& c1, const Channel& c2)
	{
		return c1.ChannelID_ == c2.ChannelID_;
	}

	QDataStream& operator<< (QDataStream& out, const Channel& chan)
	{
		int version = 3;
		out << version
			<< chan.Title_
			<< chan.Link_
			<< chan.Description_
			<< chan.LastBuild_
			<< chan.Tags_
			<< chan.Language_
			<< chan.Author_
			<< chan.PixmapURL_
			<< chan.Pixmap_
			<< chan.Favicon_
			<< static_cast<quint32> (chan.Items_.size ());
		for (size_t i = 0; i < chan.Items_.size (); ++i)
			out << *chan.Items_ [i];
		return out;
	}

	QDataStream& operator>> (QDataStream& in, Channel& chan)
	{
		int version = 0;
		in >> version;
		if (version == 1)
		{
			quint32 size;
			in >> chan.Title_
				>> chan.Link_
				>> chan.Description_
				>> chan.LastBuild_
				>> chan.Tags_
				>> chan.Language_
				>> chan.Author_
				>> chan.PixmapURL_
				>> chan.Pixmap_
				>> chan.Favicon_;
			in >> size;
			for (size_t i = 0; i < size; ++i)
			{
				Item_ptr it (new Item (chan.ChannelID_));
				in >> *it;
				chan.Items_.push_back (it);
			}
		}
		else if (version == 2 || version == 3)
		{

			quint32 size;
			in >> chan.Title_
				>> chan.Link_
				>> chan.Description_
				>> chan.LastBuild_
				>> chan.Tags_
				>> chan.Language_
				>> chan.Author_
				>> chan.PixmapURL_;

			if (version == 3)
				in >> chan.Pixmap_
					>> chan.Favicon_;
			else
			{
				QPixmap px, favicon;
				in >> px
					>> favicon;
				chan.Pixmap_ = px.toImage ();
				chan.Favicon_ = px.toImage ();
			}

			in >> size;
			for (size_t i = 0; i < size; ++i)
			{
				Item_ptr it (new Item (chan.ChannelID_));
				in >> *it;
				chan.Items_.push_back (it);
			}
		}
		return in;
	}
}
}
