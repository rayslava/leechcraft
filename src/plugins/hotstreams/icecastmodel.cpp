/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2012  Georg Rudoy
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

#include "icecastmodel.h"
#include <QUrl>
#include <QtDebug>
#include "roles.h"

Q_DECLARE_METATYPE (QList<QUrl>)

namespace LeechCraft
{
namespace HotStreams
{
	bool operator== (const IcecastModel::StationInfo& s1, const IcecastModel::StationInfo& s2)
	{
		return s1.Bitrate_ == s2.Bitrate_ &&
				s1.Name_ == s2.Name_ &&
				s1.Genre_ == s2.Genre_ &&
				s1.MIME_ == s2.MIME_ &&
				s1.URLs_ == s2.URLs_;
	}

	namespace
	{
		enum class IndexType
		{
			Root,
			Category,
			Genre,
			Station
		};

		IndexType GetIndexType (const QModelIndex& index)
		{
			if (!index.isValid ())
				return IndexType::Root;

			const auto id = static_cast<quint32> (index.internalId ());
			if (id == 0xffffffff)
				return IndexType::Category;

			if (!id)
				return IndexType::Genre;

			return IndexType::Station;
		}

		int GetGenreIndex (const QModelIndex& index)
		{
			return index.internalId () - 1;
		}

		quint32 MakeStationId (int genre)
		{
			return genre + 1;
		}

		quint32 MakeGenreId ()
		{
			return 0;
		}

		quint32 MakeCategoryId ()
		{
			return 0xffffffff;
		}
	}

	IcecastModel::IcecastModel (QObject *parent)
	: QAbstractItemModel { parent }
	{
	}

	QModelIndex IcecastModel::index (int row, int column, const QModelIndex& parent) const
	{
		if (!hasIndex (row, column, parent))
			return {};

		switch (GetIndexType (parent))
		{
		case IndexType::Root:
			return createIndex (row, column, MakeCategoryId ());
		case IndexType::Category:
			return createIndex (row, column, MakeGenreId ());
		case IndexType::Genre:
			return createIndex (row, column, MakeStationId (parent.row ()));
		case IndexType::Station:
			return {};
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown index type"
				<< parent;
		return {};
	}

	QModelIndex IcecastModel::parent (const QModelIndex& child) const
	{
		switch (GetIndexType (child))
		{
		case IndexType::Root:
		case IndexType::Category:
			return {};
		case IndexType::Genre:
			return createIndex (0, 0, MakeCategoryId ());
		case IndexType::Station:
			return createIndex (GetGenreIndex (child), 0, MakeGenreId ());
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown index type"
				<< child;
		return {};
	}

	int IcecastModel::rowCount (const QModelIndex& parent) const
	{
		switch (GetIndexType (parent))
		{
		case IndexType::Root:
			return 1;
		case IndexType::Category:
			return Stations_.size ();
		case IndexType::Genre:
			return Stations_.value (parent.row ()).second.size ();
		case IndexType::Station:
			return 0;
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown index type"
				<< parent;
		return 0;
	}

	int IcecastModel::columnCount (const QModelIndex&) const
	{
		return 1;
	}

	Qt::ItemFlags IcecastModel::flags (const QModelIndex& index) const
	{
		auto flags = QAbstractItemModel::flags (index);
		if (GetIndexType (index) == IndexType::Station)
			flags |= Qt::ItemIsDragEnabled;
		return flags;
	}

	QVariant IcecastModel::data (const QModelIndex& index, int role) const
	{
		if (role == Qt::DecorationRole)
			return RadioIcon_;

		switch (GetIndexType (index))
		{
		case IndexType::Root:
			return {};
		case IndexType::Category:
			switch (role)
			{
			case Qt::DisplayRole:
				return tr ("Icecast");
			case Media::RadioItemRole::ItemType:
				return Media::RadioType::None;
			default:
				return {};
			}
		case IndexType::Genre:
			switch (role)
			{
			case Qt::DisplayRole:
				return Stations_.value (index.row ()).first;
			case Media::RadioItemRole::ItemType:
				return Media::RadioType::None;
			default:
				return {};
			}
		case IndexType::Station:
			return GetStationData (index, role);
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown index type"
				<< index;
		return {};
	}

	void IcecastModel::SetStations (const StationInfoList_t& stations)
	{
		if (stations == Stations_)
			return;

		const auto prevSize = Stations_.size ();
		if (prevSize)
			beginRemoveRows (index (0, 0, {}), 0, prevSize - 1);
		Stations_.clear ();
		if (prevSize)
			endRemoveRows ();

		const auto newSize = stations.size ();
		if (newSize)
			beginInsertRows (index (0, 0, {}), 0, newSize - 1);
		Stations_ = stations;
		if (newSize)
			endInsertRows ();
	}

	QVariant IcecastModel::GetStationData (const QModelIndex& index, int role) const
	{
		const auto& genred = Stations_.value (GetGenreIndex (index));
		const auto& station = genred.second.value (index.row ());

		switch (role)
		{
		case Qt::DisplayRole:
		case StreamItemRoles::PristineName:
			return station.Name_;
		case StreamItemRoles::PlaylistFormat:
			return "urllist";
		case Media::RadioItemRole::ItemType:
			return Media::RadioType::Predefined;
		case Media::RadioItemRole::RadioID:
			return QVariant::fromValue (station.URLs_);
		default:
			return {};
		}
	}

}
}