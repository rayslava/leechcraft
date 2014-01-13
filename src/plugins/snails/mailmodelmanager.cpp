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

#include "mailmodelmanager.h"
#include <QStandardItemModel>
#include <util/util.h>

namespace LeechCraft
{
namespace Snails
{
	MailModelManager::MailModelManager (QObject *parent)
	: QObject (parent)
	, Model_ (new QStandardItemModel (this))
	, CurrentFolder_ ("INBOX")
	{
		clear ();
	}

	QAbstractItemModel* MailModelManager::GetModel () const
	{
		return Model_;
	}

	void MailModelManager::UpdateReadStatus (const QByteArray& id, bool isRead)
	{
		if (!MailID2Item_.contains (id))
			return;

		QStandardItem *item = MailID2Item_ [id];
		const QModelIndex& sIdx = item->index ();
		for (int i = 0; i < MailColumns::Max; ++i)
		{
			QStandardItem *other = Model_->
					itemFromIndex (sIdx.sibling (sIdx.row (), i));
			other->setData (isRead, MailRole::ReadStatus);
		}
		QMetaObject::invokeMethod (Model_,
				"dataChanged",
				Q_ARG (QModelIndex, sIdx.sibling (sIdx.row (), 0)),
				Q_ARG (QModelIndex, sIdx.sibling (sIdx.row (), MailColumns::Max - 1)));
	}

	void MailModelManager::SetCurrentFolder (const QStringList& folder)
	{
		CurrentFolder_ = folder;
	}

	void MailModelManager::clear ()
	{
		Model_->clear ();
		MailID2Item_.clear ();

		QStringList headers;
		headers << tr ("From")
				<< tr ("Subject")
				<< tr ("Date")
				<< tr ("Size");
		Model_->setHorizontalHeaderLabels (headers);
	}

	void MailModelManager::appendMessages (const QList<Message_ptr>& messages)
	{
		Q_FOREACH (Message_ptr message, messages)
		{
			if (!message->GetFolders ().contains (CurrentFolder_))
				continue;

			if (MailID2Item_.contains (message->GetID ()))
				Model_->removeRow (MailID2Item_ [message->GetID ()]->row ());

			QList<QStandardItem*> row;
			row << new QStandardItem (GetNiceMail (message->GetAddress (Message::Address::From)));
			row << new QStandardItem (message->GetSubject ());
			row << new QStandardItem (message->GetDate ().toString ());
			row << new QStandardItem (Util::MakePrettySize (message->GetSize ()));
			Model_->appendRow (row);

			row [MailColumns::From]->setData (row [MailColumns::From]->text (), MailRole::Sort);
			row [MailColumns::Subj]->setData (row [MailColumns::Subj]->text (), MailRole::Sort);
			row [MailColumns::Date]->setData (message->GetDate (), MailRole::Sort);
			row [MailColumns::Size]->setData (message->GetSize (), MailRole::Sort);

			Q_FOREACH (auto item, row)
				item->setData (message->GetID (), MailRole::ID);
			MailID2Item_ [message->GetID ()] = row.first ();

			UpdateReadStatus (message->GetID (), message->IsRead ());
		}
	}

	void MailModelManager::replaceMessages (const QList<Message_ptr>& messages)
	{
		clear ();
		appendMessages (messages);
	}
}
}
