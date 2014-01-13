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

#include <QtPlugin>

class QModelIndex;

namespace LeechCraft
{
namespace Blasq
{
	struct UploadItem
	{
		QString FilePath_;
		QString Description_;
	};

	class ISupportUploads
	{
	public:
		/** @brief Describes various pecularities of upload services.
		 */
		enum class Feature
		{
			/** @brief Requires an album to be selected on upload.
			 *
			 * Lacking this feature means that photos can be uploaded
			 * without choosing the album to upload to.
			 */
			RequiresAlbumOnUpload,

			/** @brief Supports putting descriptions alongside the photos.
			 */
			SupportsDescriptions
		};

		virtual ~ISupportUploads () {}

		virtual bool HasUploadFeature (Feature) const = 0;

		virtual void CreateCollection (const QModelIndex& parent) = 0;

		virtual void UploadImages (const QModelIndex& collection, const QList<UploadItem>& paths) = 0;
	};
}
}

Q_DECLARE_INTERFACE (LeechCraft::Blasq::ISupportUploads, "org.LeechCraft.Blasq.ISupportUploads/1.0");
