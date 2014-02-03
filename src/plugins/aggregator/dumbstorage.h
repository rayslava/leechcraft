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

#pragma once

#include "storagebackend.h"

namespace LeechCraft
{
namespace Aggregator
{
	class DumbStorage : public StorageBackend
	{
	public:
		void Prepare ();
		void GetFeedsIDs (ids_t&) const;
		Feed_ptr GetFeed (const IDType_t&) const;
		IDType_t FindFeed (const QString&) const;
		Feed::FeedSettings GetFeedSettings (const IDType_t&) const;
		void SetFeedSettings (const Feed::FeedSettings&);
		void GetChannels (channels_shorts_t&, const IDType_t&) const;
		Channel_ptr GetChannel (const IDType_t&, const IDType_t&) const;
		IDType_t FindChannel (const QString&, const QString&, const IDType_t&) const;
		void TrimChannel (const IDType_t&, int, int);
		void GetItems (items_shorts_t&, const IDType_t&) const;
		int GetUnreadItems (const IDType_t&) const;
		Item_ptr GetItem (const IDType_t&) const;
		IDType_t FindItem (const QString&, const QString&, const IDType_t&) const;
		void GetItems (items_container_t&, const IDType_t&) const;
		void AddFeed (Feed_ptr);
		void AddChannel (Channel_ptr);
		void AddItem (Item_ptr);
		void UpdateChannel (Channel_ptr);
		void UpdateChannel (const ChannelShort&);
		void UpdateItem (Item_ptr);
		void UpdateItem (const ItemShort&);
		void RemoveItem (const IDType_t&);
		void RemoveChannel (const IDType_t&);
		void RemoveFeed (const IDType_t&);
		bool UpdateFeedsStorage (int, int);
		bool UpdateChannelsStorage (int, int);
		bool UpdateItemsStorage (int, int);
		void ToggleChannelUnread (const IDType_t&, bool);
		QList<ITagsManager::tag_id> GetItemTags (const IDType_t&);
		void SetItemTags (const IDType_t&, const QList<ITagsManager::tag_id>&);
		QList<IDType_t> GetItemsForTag (const ITagsManager::tag_id&);
		IDType_t GetHighestID (const PoolType&) const;
	};
}
}
