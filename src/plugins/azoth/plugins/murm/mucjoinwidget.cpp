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

#include "mucjoinwidget.h"
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include "vkaccount.h"
#include "vkentry.h"
#include <util/gui/clearlineeditaddon.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Murm
{
	namespace
	{
		enum Role
		{
			EntryObj = Qt::UserRole + 1
		};

		class UsersSortFilterProxy : public QSortFilterProxyModel
		{
		public:
			UsersSortFilterProxy (QObject *parent)
			: QSortFilterProxyModel (parent)
			{
				setDynamicSortFilter (true);
				setSortCaseSensitivity (Qt::CaseInsensitive);
			}
		};
	}

	MucJoinWidget::MucJoinWidget (ICoreProxy_ptr proxy)
	: UsersModel_ (new QStandardItemModel (this))
	, UsersFilter_ (new UsersSortFilterProxy (this))
	{
		UsersFilter_->setSourceModel (UsersModel_);

		Ui_.setupUi (this);
		Ui_.UsersTree_->setModel (UsersFilter_);

		new Util::ClearLineEditAddon (proxy, Ui_.UsersFilter_);
		connect (Ui_.UsersFilter_,
				SIGNAL (textChanged (QString)),
				UsersFilter_,
				SLOT (setFilterFixedString (QString)));
	}

	void MucJoinWidget::AccountSelected (QObject *accObj)
	{
		UsersModel_->clear ();

		auto acc = qobject_cast<VkAccount*> (accObj);
		for (auto entryObj : acc->GetCLEntries ())
		{
			auto entry = qobject_cast<VkEntry*> (entryObj);
			if (!entry)
				continue;

			auto item = new QStandardItem (entry->GetEntryName ());
			item->setEditable (false);
			item->setData (QVariant::fromValue (entryObj), Role::EntryObj);
			item->setCheckable (true);
			item->setCheckState (Qt::Unchecked);
			UsersModel_->appendRow (item);
		}
	}

	void MucJoinWidget::Join (QObject *accObj)
	{
		auto acc = qobject_cast<VkAccount*> (accObj);

		acc->CreateChat (Ui_.ChatName_->text (), GetSelectedEntries ());
	}

	void MucJoinWidget::Cancel ()
	{
	}

	QVariantMap MucJoinWidget::GetIdentifyingData () const
	{
		return {};
	}

	void MucJoinWidget::SetIdentifyingData (const QVariantMap&)
	{
	}

	QList<VkEntry*> MucJoinWidget::GetSelectedEntries () const
	{
		QList<VkEntry*> result;
		for (int i = 0; i < UsersModel_->rowCount (); ++i)
		{
			const auto item = UsersModel_->item (i);
			if (item->checkState () != Qt::Checked)
				continue;

			result << qobject_cast<VkEntry*> (item->data (Role::EntryObj).value<QObject*> ());
		}
		return result;
	}
}
}
}
