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

#include "playlistmanager.h"
#include <algorithm>
#include <QStandardItemModel>
#include <QTimer>
#include <QMimeData>
#include <interfaces/core/iiconthememanager.h>
#include <util/sll/functional.h>
#include <util/sll/prelude.h>
#include <util/sll/views.h>
#include <util/gui/util.h>
#include <util/models/dndactionsmixin.h>
#include "core.h"
#include "staticplaylistmanager.h"
#include "localcollection.h"
#include "mediainfo.h"

namespace LeechCraft
{
namespace LMP
{
	namespace
	{
		class PlaylistModel : public Util::DndActionsMixin<QStandardItemModel>
		{
			PlaylistManager *Manager_;
		public:
			enum Roles
			{
				PlaylistProvider = Qt::UserRole + 1
			};

			PlaylistModel (PlaylistManager *parent)
			: DndActionsMixin<QStandardItemModel> (parent)
			, Manager_ (parent)
			{
				setSupportedDragActions (Qt::CopyAction);
			}

			QStringList mimeTypes () const override
			{
				return
				{
					"text/uri-list",
					"x-leechcraft-lmp/media-info-list"
				};
			}

			QMimeData* mimeData (const QModelIndexList& indexes) const override
			{
				QMimeData *result = new QMimeData;

				QList<QUrl> urls;
				QList<MediaInfo> infos;

				for (const auto& idx : indexes)
					for (const auto& item : Manager_->GetSources (idx))
					{
						const auto& url = item.first.ToUrl ();
						if (!url.isValid ())
							continue;

						urls << url;
						infos << item.second.get_value_or ({});
					}

				result->setUrls (urls);
				Util::Save2MimeData (result, "x-leechcraft-lmp/media-info-list", infos);

				return result;
			}
		};
	}

	PlaylistManager::PlaylistManager (QObject *parent)
	: QObject (parent)
	, Model_ (new PlaylistModel (this))
	, StaticRoot_ (new QStandardItem (tr ("Static playlists")))
	, Static_ (new StaticPlaylistManager (this))
	{
		StaticRoot_->setEditable (false);
		Model_->appendRow (StaticRoot_);

		connect (Static_,
				SIGNAL (customPlaylistsChanged ()),
				this,
				SLOT (handleStaticPlaylistsChanged ()));
		QTimer::singleShot (100,
				this,
				SLOT (handleStaticPlaylistsChanged ()));

		auto dynamicRoot = new QStandardItem (tr ("Dynamic playlists"));
		dynamicRoot->setEditable (false);
		Model_->appendRow (dynamicRoot);

		const auto types =
		{
			PlaylistTypes::Random50,
			PlaylistTypes::LovedTracks,
			PlaylistTypes::BannedTracks
		};
		const auto names =
		{
			tr ("50 random tracks"),
			tr ("Loved tracks"),
			tr ("Banned tracks")
		};

		for (const auto& pair : Util::Views::Zip (types, names))
		{
			auto item = new QStandardItem { pair.second };
			item->setData (pair.first, Roles::PlaylistType);
			item->setEditable (false);
			dynamicRoot->appendRow (item);
		}
	}

	QAbstractItemModel* PlaylistManager::GetPlaylistsModel () const
	{
		return Model_;
	}

	StaticPlaylistManager* PlaylistManager::GetStaticManager () const
	{
		return Static_;
	}

	void PlaylistManager::AddProvider (QObject *provObj)
	{
		auto prov = qobject_cast<IPlaylistProvider*> (provObj);
		if (!prov)
			return;

		PlaylistProviders_ << provObj;

		auto root = prov->GetPlaylistsRoot ();
		root->setData (QVariant::fromValue (provObj), PlaylistModel::Roles::PlaylistProvider);
		Model_->appendRow (root);
	}

	bool PlaylistManager::CanDeletePlaylist (const QModelIndex& index) const
	{
		return index.data (Roles::PlaylistType).toInt () == PlaylistTypes::Static;
	}

	void PlaylistManager::DeletePlaylist (const QModelIndex& index)
	{
		if (index.data (Roles::PlaylistType).toInt () == PlaylistTypes::Static)
			Static_->DeleteCustomPlaylist (index.data ().toString ());
	}

	NativePlaylist_t PlaylistManager::GetSources (const QModelIndex& index) const
	{
		auto col = Core::Instance ().GetLocalCollection ();
		auto toSrcs = [col] (const QList<int>& ids)
		{
			return Util::Map (col->TrackList2PathList (ids),
					[] (const QString& path) -> NativePlaylistItem_t
						{ return { path, {} }; });
		};

		switch (index.data (Roles::PlaylistType).toInt ())
		{
		case PlaylistTypes::Static:
			return Static_->GetCustomPlaylist (index.data ().toString ());
		case PlaylistTypes::Random50:
			return toSrcs (col->GetDynamicPlaylist (LocalCollection::DynamicPlaylist::Random50));
		case PlaylistTypes::LovedTracks:
			return toSrcs (col->GetDynamicPlaylist (LocalCollection::DynamicPlaylist::LovedTracks));
		case PlaylistTypes::BannedTracks:
			return toSrcs (col->GetDynamicPlaylist (LocalCollection::DynamicPlaylist::BannedTracks));
		default:
			return Util::Map (index.data (IPlaylistProvider::ItemRoles::SourceURLs).value<QList<QUrl>> (),
					[] (const QUrl& url) -> NativePlaylistItem_t
						{ return { url, {} }; });
		}
	}

	boost::optional<MediaInfo> PlaylistManager::TryResolveMediaInfo (const QUrl& url) const
	{
		for (auto provObj : PlaylistProviders_)
			if (const auto info = qobject_cast<IPlaylistProvider*> (provObj)->GetURLInfo (url))
				return MediaInfo::FromAudioInfo (*info);

		return {};
	}

	void PlaylistManager::handleStaticPlaylistsChanged ()
	{
		while (StaticRoot_->rowCount ())
			StaticRoot_->removeRow (0);

		const auto& icon = Core::Instance ().GetProxy ()->
				GetIconThemeManager ()->GetIcon ("view-media-playlist");
		for (const auto& name : Static_->EnumerateCustomPlaylists ())
		{
			auto item = new QStandardItem (icon, name);
			item->setData (PlaylistTypes::Static, Roles::PlaylistType);
			item->setEditable (false);
			StaticRoot_->appendRow (item);
		}
	}
}
}
