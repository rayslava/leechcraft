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

#include "groupmanager.h"
#include <QtDebug>
#include "msnaccount.h"
#include "msnbuddyentry.h"
#include "callbacks.h"
#include "zheetutil.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Zheet
{
	GroupManager::GroupManager (Callbacks *cb, MSNAccount *acc)
	: QObject (acc)
	, Account_ (acc)
	, CB_ (cb)
	{
		connect (CB_,
				SIGNAL (gotGroups (QList<MSN::Group>)),
				this,
				SLOT (handleGotGroups (QList<MSN::Group>)));
		connect (CB_,
				SIGNAL(removedGroup (QString)),
				this,
				SLOT (handleRemovedGroup (QString)));
		connect (CB_,
				SIGNAL (renamedGroup (QString, QString)),
				this,
				SLOT (handleRenamedGroup (QString, QString)));

		connect (CB_,
				SIGNAL (buddyAddedToGroup (QString, QString)),
				this,
				SLOT (handleBuddyAdded (QString, QString)));
		connect (CB_,
				SIGNAL (buddyRemovedFromGroup (QString, QString)),
				this,
				SLOT (handleBuddyRemoved (QString, QString)));
	}

	void GroupManager::SetGroups (MSNBuddyEntry *entry,
			const QStringList& newGroupsLst, const QStringList& oldGroupsLst)
	{
		const auto& newSet = newGroupsLst.toSet ();
		const auto& oldSet = oldGroupsLst.toSet ();

		Q_FOREACH (const QString& grp, newSet - oldSet)
			AddGroup (entry->GetHumanReadableID (), grp);

		Q_FOREACH (const QString& grp, oldSet - newSet)
			RemoveGroup (entry->GetHumanReadableID (), grp);
	}

	void GroupManager::AddGroup (const QString& entry, const QString& name)
	{
		qDebug () << Q_FUNC_INFO << entry << name << Group2ID_;
		auto conn = Account_->GetNSConnection ();

		if (Group2ID_.contains (name))
		{
			const auto& id = Group2ID_ [name];
			const auto& cid = Account_->GetBuddy (entry)->GetContactID ();
			conn->addToGroup (ZheetUtil::ToStd (id), ZheetUtil::ToStd (cid));
		}
		else
		{
			conn->addGroup (ZheetUtil::ToStd (name));
			PendingAdditions_ [name] << entry;
		}
	}

	void GroupManager::RemoveGroup (const QString& entry, const QString& name)
	{
		qDebug () << Q_FUNC_INFO << entry << name << Group2ID_;
		if (!Group2ID_.contains (name))
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown group"
					<< name;
			return;
		}

		const auto& id = ZheetUtil::ToStd (Group2ID_ [name]);
		const auto& entryId = ZheetUtil::ToStd (Account_->GetBuddy (entry)->GetContactID ());
		Account_->GetNSConnection ()->removeFromGroup (id, entryId);
	}

	void GroupManager::handleGotGroups (const QList<MSN::Group>& groups)
	{
		Q_FOREACH (const MSN::Group& g, groups)
		{
			const auto& name = ZheetUtil::FromStd (g.name);
			const auto& id = ZheetUtil::FromStd (g.groupID);
			Group2ID_ [name] = id;
			ID2Group_ [id] = name;

			Q_FOREACH (const QString& id, PendingAdditions_.take (name))
				AddGroup (id, name);
		}
	}

	void GroupManager::handleRemovedGroup (const QString& id)
	{
		Group2ID_.remove (ID2Group_.take (id));
	}

	void GroupManager::handleRenamedGroup (const QString& id, const QString& name)
	{
		Group2ID_.remove (ID2Group_.take (id));

		Group2ID_ [name] = id;
		ID2Group_ [id] = name;
	}

	void GroupManager::handleBuddyAdded (const QString& id, const QString& groupId)
	{
		Account_->GetBuddyByCID (id)->AddGroup (ID2Group_ [groupId]);
	}

	void GroupManager::handleBuddyRemoved (const QString& id, const QString& groupId)
	{
		Account_->GetBuddyByCID (id)->RemoveGroup (ID2Group_ [groupId]);
	}
}
}
}
