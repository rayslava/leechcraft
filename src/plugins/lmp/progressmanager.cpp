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

#include "progressmanager.h"
#include <QStandardItemModel>
#include <interfaces/ijobholder.h>
#include "sync/syncmanagerbase.h"

namespace LeechCraft
{
namespace LMP
{
	ProgressManager::ProgressManager (QObject *parent)
	: QObject (parent)
	, Model_ (new QStandardItemModel (this))
	{
	}

	QAbstractItemModel* ProgressManager::GetModel () const
	{
		return Model_;
	}

	void ProgressManager::AddSyncManager (SyncManagerBase *syncManager)
	{
		connect (syncManager,
				SIGNAL (transcodingProgress (int, int, SyncManagerBase*)),
				this,
				SLOT (handleTCProgress (int, int, SyncManagerBase*)));
		connect (syncManager,
				SIGNAL (uploadProgress (int, int, SyncManagerBase*)),
				this,
				SLOT (handleUploadProgress (int, int, SyncManagerBase*)));
	}

	void ProgressManager::HandleWithHash (int done, int total,
			SyncManagerBase *syncer, Syncer2Row_t& hash, const QString& name, const QString& status)
	{
		if (!hash.contains (syncer))
		{
			if (done == total)
				return;

			const QList<QStandardItem*> row
			{
				new QStandardItem (name),
				new QStandardItem (status),
				new QStandardItem ()
			};
			auto item = row.at (JobHolderColumn::JobProgress);
			item->setData (QVariant::fromValue<JobHolderRow> (JobHolderRow::ProcessProgress),
					CustomDataRoles::RoleJobHolderRow);

			hash [syncer] = row;
			Model_->appendRow (row);
		}

		const auto& row = hash [syncer];
		if (done == total)
		{
			Model_->removeRow (row.first ()->row ());
			hash.remove (syncer);
			return;
		}

		auto item = row.at (JobHolderColumn::JobProgress);
		item->setText (tr ("%1 of %2").arg (done).arg (total));
		item->setData (done, ProcessState::Done);
		item->setData (total, ProcessState::Total);
	}

	void ProgressManager::handleTCProgress (int done, int total, SyncManagerBase *syncer)
	{
		HandleWithHash (done, total, syncer, TCRows_,
				tr ("Audio transcoding"), tr ("Transcoding..."));
	}

	void ProgressManager::handleUploadProgress (int done, int total, SyncManagerBase *syncer)
	{
		HandleWithHash (done, total, syncer, UpRows_,
				tr ("Audio upload"), tr ("Uploading..."));
	}
}
}
