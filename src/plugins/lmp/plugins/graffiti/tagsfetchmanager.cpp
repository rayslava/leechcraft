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

#include "tagsfetchmanager.h"
#include <interfaces/lmp/mediainfo.h>
#include <interfaces/media/itagsfetcher.h>
#include "filesmodel.h"

namespace LeechCraft
{
namespace LMP
{
namespace Graffiti
{
	TagsFetchManager::TagsFetchManager (const QStringList& paths,
			Media::ITagsFetcher *prov, FilesModel *filesModel, QObject *parent)
	: QObject (parent)
	, FilesModel_ (filesModel)
	, FetchedTags_ (0)
	, TotalTags_ (paths.size ())
	{
		for (const auto& path : paths)
		{
			auto pending = prov->FetchTags (path);
			connect (pending->GetQObject (),
					SIGNAL (ready (QString, Media::AudioInfo)),
					this,
					SLOT (handleTagsFetched (QString, Media::AudioInfo)));
		}

		QMetaObject::invokeMethod (this,
				"tagsFetchProgress",
				Q_ARG (int, 0),
				Q_ARG (int, TotalTags_),
				Q_ARG (QObject*, this));
	}

	namespace
	{
		template<typename T>
		bool IsEmptyData (const T&)
		{
			static_assert (!sizeof (T), "unknown data type");
			return false;
		}

		template<>
		bool IsEmptyData<QString> (const QString& str)
		{
			return str.isEmpty ();
		}

		template<>
		bool IsEmptyData<int> (const int& val)
		{
			return !val;
		}

		template<>
		bool IsEmptyData<QStringList> (const QStringList& list)
		{
			return list.isEmpty ();
		}

		template<typename F>
		void UpgradeInfo (MediaInfo& info, MediaInfo& other, F getter)
		{
			static_assert (std::is_lvalue_reference<typename std::result_of<F (MediaInfo&)>::type>::value,
					"functor doesn't return an lvalue reference");

			auto& data = getter (info);
			const auto& otherData = getter (other);
			if (!IsEmptyData (otherData) && IsEmptyData (data))
				data = otherData;
		}
	}

	void TagsFetchManager::handleTagsFetched (const QString& filename, const Media::AudioInfo& result)
	{
		emit tagsFetchProgress (++FetchedTags_, TotalTags_, this);

		const auto& index = FilesModel_->FindIndex (filename);
		if (!index.isValid ())
			return;

		auto newInfo = MediaInfo::FromAudioInfo (result);

		auto info = index.data (FilesModel::Roles::MediaInfoRole).value<MediaInfo> ();
		UpgradeInfo (info, newInfo, [] (MediaInfo& info) -> QString& { return info.Title_; });
		UpgradeInfo (info, newInfo, [] (MediaInfo& info) -> QString& { return info.Artist_; });
		UpgradeInfo (info, newInfo, [] (MediaInfo& info) -> QString& { return info.Album_; });
		UpgradeInfo (info, newInfo, [] (MediaInfo& info) -> int& { return info.Year_; });
		UpgradeInfo (info, newInfo, [] (MediaInfo& info) -> int& { return info.TrackNumber_; });
		UpgradeInfo (info, newInfo, [] (MediaInfo& info) -> QStringList& { return info.Genres_; });
		FilesModel_->UpdateInfo (index, info);

		emit tagsFetched (filename);

		if (FetchedTags_ == TotalTags_)
			emit finished (true);
	}
}
}
}
