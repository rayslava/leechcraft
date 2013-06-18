/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "account.h"
#include <algorithm>
#include <QtDebug>
#include <QStandardItemModel>
#include <QMessageBox>
#include <QMainWindow>
#include <QPushButton>
#include <interfaces/core/irootwindowsmanager.h>
#include "core.h"
#include "uploadmanager.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace NetStoreManager
{
namespace GoogleDrive
{
	Account::Account (const QString& name, QObject *parentPlugin)
	: QObject (parentPlugin)
	, ParentPlugin_ (parentPlugin)
	, Name_ (name)
	, Trusted_ (false)
	, DriveManager_ (new DriveManager (this, this))
	{
		connect (DriveManager_,
				SIGNAL (gotFiles (const QList<DriveItem>&)),
				this,
				SLOT (handleFileList (const QList<DriveItem>&)));
		connect (DriveManager_,
				SIGNAL (gotSharedFileId (const QString&)),
				this,
				SLOT (handleSharedFileId (const QString&)));
		connect (DriveManager_,
				SIGNAL (gotNewItem (DriveItem)),
				this,
				SLOT (handleGotNewItem (DriveItem)));
		connect (DriveManager_,
				SIGNAL (gotChanges (QList<DriveChanges>)),
				this,
				SLOT (handleGotChanges (QList<DriveChanges>)));
	}

	QObject* Account::GetQObject ()
	{
		return this;
	}

	QObject* Account::GetParentPlugin () const
	{
		return ParentPlugin_;
	}

	QByteArray Account::GetUniqueID () const
	{
		return ("NetStoreManager.GoogleDrive_" + Name_).toUtf8 ();
	}

	AccountFeatures Account::GetAccountFeatures () const
	{
		return FileListings;
	}

	QString Account::GetAccountName () const
	{
		return Name_;
	}

	void Account::Upload (const QString& filepath, const QByteArray& parentId,
			UploadType ut, const QByteArray& id)
	{
		auto uploadManager = new UploadManager (filepath,
				ut, parentId, this, id);

		connect (uploadManager,
				SIGNAL (uploadProgress (quint64, quint64, QString)),
				this,
				SIGNAL (upProgress (quint64, quint64, QString)));
		connect (uploadManager,
				SIGNAL (uploadError (QString, QString)),
				this,
				SIGNAL (upError (QString, QString)));
		connect (uploadManager,
				SIGNAL (finished (QByteArray, QString)),
				this,
				SIGNAL (upFinished (QByteArray, QString)));
		connect (uploadManager,
				SIGNAL (uploadStatusChanged (QString, QString)),
				this,
				SIGNAL (upStatusChanged (QString, QString)));
	}

	void Account::Download (const QByteArray& id, const QString& filepath,
			TaskParameters tp, bool silent, bool open)
	{
		if (id.isEmpty ())
			return;

		DriveManager_->Download (id, filepath, tp, silent, open);
	}

	ListingOps Account::GetListingOps () const
	{
		return ListingOp::Delete | ListingOp::TrashSupporting | ListingOp::DirectorySupport;
	}

	void Account::RefreshListing ()
	{
		DriveManager_->RefreshListing ();
	}

	void Account::Delete (const QList<QByteArray>& ids, bool ask)
	{
		if (ids.isEmpty ())
			return;

		if (ask)
		{
			auto rootWM = Core::Instance ().GetProxy ()->GetRootWindowsManager ();
			auto res = QMessageBox::warning (rootWM->GetPreferredWindow (),
					tr ("LeechCraft"),
					tr ("Are you sure you want to delete all selected items? This action cannot be undone."
						"<br><i>Note: if you delete a directory then all files in it will also be deleted.</i>"),
					QMessageBox::Ok | QMessageBox::Cancel);
			if (res != QMessageBox::Ok)
				return;
		}

		for (const auto& id : ids)
			DriveManager_->RemoveEntry (id);
	}

	void Account::MoveToTrash (const QList<QByteArray>& ids)
	{
		for (const auto& id : ids)
			DriveManager_->MoveEntryToTrash (id);
	}

	void Account::RestoreFromTrash (const QList<QByteArray>& ids)
	{
		for (const auto& id : ids)
			DriveManager_->RestoreEntryFromTrash (id);
	}

	void Account::Copy (const QList<QByteArray>& ids,
			const QByteArray& newParentId)
	{
		for (const auto& id : ids)
			DriveManager_->Copy (id, newParentId);
	}

	void Account::Move (const QList<QByteArray>& ids,
			const QByteArray& newParentId)
	{
		for (const auto& id : ids)
			DriveManager_->Move (id, newParentId);
	}

	void Account::RequestUrl (const QByteArray& id)
	{
		if (id.isNull ())
			return;

		if (!XmlSettingsManager::Instance ().property ("AutoShareOnUrlRequest").toBool ())
		{
			auto rootWM = Core::Instance ().GetProxy ()->GetRootWindowsManager ();
			QMessageBox mbox (QMessageBox::Question,
					tr ("Share item"),
					tr ("The item needs to be shared to obtain the URL. Do you want to share it?"),
					QMessageBox::Yes | QMessageBox::No,
					rootWM->GetPreferredWindow());
			mbox.setDefaultButton (QMessageBox::Yes);

			QPushButton always (tr ("Always"));
			mbox.addButton (&always, QMessageBox::AcceptRole);

			if (mbox.exec () == QMessageBox::No)
				return;
			else if (mbox.clickedButton () == &always)
				XmlSettingsManager::Instance ().setProperty ("AutoShareOnUrlRequest", true);
		}

		DriveManager_->ShareEntry (id);
	}

	void Account::CreateDirectory (const QString& name, const QByteArray& parentId)
	{
		if (name.isEmpty ())
			return;
		DriveManager_->CreateDirectory (name, parentId);
	}

	void Account::Rename (const QByteArray& id, const QString& newName)
	{
		if (id.isEmpty ())
			return;
		DriveManager_->Rename (id, newName);
	}

	void Account::RequestChanges ()
	{
		DriveManager_->RequestFileChanges (XmlSettingsManager::Instance ()
				.Property ("LastChangesId", 0).toLongLong ());
	}

	QByteArray Account::Serialize ()
	{
		QByteArray result;
		QDataStream str (&result, QIODevice::WriteOnly);
		str << static_cast<quint8> (1)
				<< Name_
				<< Trusted_
				<< RefreshToken_;
		return result;
	}

	Account_ptr Account::Deserialize (const QByteArray& data, QObject* parentPlugin)
	{
		QDataStream str (data);
		quint8 version = 0;
		str >> version;

		if (version != 1)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown version"
					<< version;
			return Account_ptr ();
		}

		QString name;
		str >> name;
		Account_ptr acc (new Account (name, parentPlugin));
		str >> acc->Trusted_
				>> acc->RefreshToken_;
		return acc;
	}

	bool Account::IsTrusted () const
	{
		return Trusted_;
	}

	void Account::SetTrusted (bool trust)
	{
		Trusted_ = trust;
	}

	void Account::SetAccessToken (const QString& token)
	{
		AccessToken_ = token;
	}

	void Account::SetRefreshToken (const QString& token)
	{
		RefreshToken_ = token;
	}

	QString Account::GetRefreshToken () const
	{
		return RefreshToken_;
	}

	DriveManager* Account::GetDriveManager () const
	{
		return DriveManager_;
	}

	namespace
	{
		StorageItem CreateItem (const DriveItem& item)
		{
			StorageItem storageItem;
			storageItem.ID_ = item.Id_.toUtf8 ();
			storageItem.ParentID_ = item.ParentId_.toUtf8 ();
			storageItem.Name_ = item.Name_;
			storageItem.Size_ = item.FileSize_;
			storageItem.ModifyDate_ = item.ModifiedDate_;
			storageItem.Hash_ = item.Md5_.toUtf8 ();
			storageItem.HashType_ = StorageItem::HashType::Md5;
			storageItem.IsDirectory_ = item.IsFolder_;
			storageItem.IsTrashed_ = item.Labels_ & DriveItem::ILRemoved;
			storageItem.MimeType_ = item.Mime_;
			storageItem.Url_ = item.DownloadUrl_;
			storageItem.ShareUrl_ = item.ShareUrl_;
			storageItem.Shared_ = item.Shared_;
			for (const auto& key : item.ExportLinks_.keys ())
			{
				const QString mime = item.ExportLinks_.value (key);
				storageItem.ExportLinks [key] = qMakePair (mime, key.queryItems ().last ().second);
			}

			return storageItem;
		}
	}

	void Account::handleFileList (const QList<DriveItem>& items)
	{
		QList<StorageItem> result;

		for (const auto& item : items)
			result << CreateItem (item);

		emit gotListing (result);
	}

	void Account::handleSharedFileId (const QString& id)
	{
		emit gotFileUrl (QUrl (QString ("https://docs.google.com/uc?id=%1&export=download")
				.arg (id)), id.toUtf8 ());
	}

	void Account::handleGotNewItem (const DriveItem& item)
	{
		emit gotNewItem (CreateItem (item), item.ParentId_.toUtf8 ());
	}

	void Account::handleGotChanges (const QList<DriveChanges>& driveChanges)
	{
		QList<Change> changes;
		for (const auto& driveChange : driveChanges)
		{
			//TODO setting for shared files
			if (driveChange.FileResource_.PermissionRole_ != DriveItem::Roles::Owner)
				continue;

			Change change;
			change.Deleted_ = driveChange.Deleted_;
			change.ID_ = driveChange.Id_.toUtf8 ();
			change.ItemID_ = driveChange.FileId_.toUtf8 ();
			if (!change.Deleted_)
				change.Item_ = CreateItem (driveChange.FileResource_);

			changes << change;
		}

		emit gotChanges (changes);
	}

}
}
}
