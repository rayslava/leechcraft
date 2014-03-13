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

#include "shortcutmanager.h"
#include <memory>
#include <algorithm>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QSettings>
#include <QtDebug>
#include <interfaces/iinfo.h>
#include <interfaces/ihaveshortcuts.h>
#include <interfaces/core/iiconthememanager.h>
#include "keysequencer.h"
#include "coreproxy.h"

namespace LeechCraft
{
	class SMFilterProxyModel : public QSortFilterProxyModel
	{
	public:
		SMFilterProxyModel (QObject *parent = 0)
		: QSortFilterProxyModel (parent)
		{
		}
	protected:
		bool filterAcceptsRow (int row, const QModelIndex& parent) const
		{
			if (!parent.isValid ())
				return true;

			const QString& filter = filterRegExp ().pattern ();
			if (filter.isEmpty ())
				return true;

			auto checkStr = [row, parent, &filter, this] (int col)
			{
				return sourceModel ()->index (row, col, parent).data ()
						.toString ().contains (filter, Qt::CaseInsensitive);
			};
			if (checkStr (0) || checkStr (1))
				return true;
			return false;
		}
	};

	ShortcutManager::ShortcutManager (QWidget *parent)
	: QWidget (parent)
	, Model_ (new QStandardItemModel (this))
	, Filter_ (new SMFilterProxyModel (this))
	{
		Filter_->setDynamicSortFilter (true);
		Model_->setHorizontalHeaderLabels ({ tr ("Name"), tr ("Shortcut"), tr ("Alternate") });
		Filter_->setSourceModel (Model_);
		Filter_->sort (0);

		Ui_.setupUi (this);
		Ui_.Tree_->setModel (Filter_);
		connect (Ui_.FilterLine_,
				SIGNAL (textChanged (QString)),
				Filter_,
				SLOT (setFilterFixedString (QString)));
		connect (Ui_.FilterLine_,
				SIGNAL (textChanged (QString)),
				Ui_.Tree_,
				SLOT (expandAll ()));
	}

	bool ShortcutManager::HasObject (QObject *object) const
	{
		return GetObjectRow (object) != -1;
	}

	void ShortcutManager::AddObject (QObject *object)
	{
		IInfo *ii = qobject_cast<IInfo*> (object);
		if (!ii)
		{
			qWarning () << Q_FUNC_INFO
				<< object
				<< "couldn't be casted to IInfo";
			return;
		}
		AddObject (object, ii->GetName (), ii->GetInfo (), ii->GetIcon ());
	}

	void ShortcutManager::AddObject (QObject *object,
			const QString& objName, const QString& objDescr,
			const QIcon& objIcon)
	{
		if (HasObject (object))
			return;

		IHaveShortcuts *ihs = qobject_cast<IHaveShortcuts*> (object);

		if (!ihs)
		{
			qWarning () << Q_FUNC_INFO
				<< object
				<< "could not be casted to IHaveShortcuts";
			return;
		}

		QSettings settings ("Deviant", "Leechcraft");
		settings.beginGroup ("Shortcuts");

		auto deEdit = [] (const QList<QStandardItem*>& items) -> void
		{
			for (const auto item : items)
				item->setEditable (false);
		};

		auto parentFirst = new QStandardItem (objName);
		parentFirst->setIcon (objIcon);
		parentFirst->setData (QVariant::fromValue<QObject*> (object), Roles::Object);

		QList<QStandardItem*> parentRow;
		parentRow << parentFirst;
		parentRow << new QStandardItem (objDescr);
		deEdit (parentRow);

		const auto& info = ihs->GetActionInfo ();

		settings.beginGroup (objName);
		Q_FOREACH (const QString& name, info.keys ())
		{
			const auto& sequences = settings.value (name,
					QVariant::fromValue (info [name].Seqs_)).value<QKeySequences_t> ();

			auto first = new QStandardItem (info [name].UserVisibleText_);

			auto icon = info [name].Icon_;
			if (icon.isNull ())
				icon = CoreProxy ().GetIconThemeManager ()->GetIcon ("configure-shortcuts");
			first->setIcon (icon);

			first->setData (name, Roles::OriginalName);
			first->setData (QVariant::fromValue (sequences), Roles::Sequence);

			QList<QStandardItem*> itemRow;
			itemRow << first;
			itemRow << new QStandardItem (sequences.value (0).toString (QKeySequence::NativeText));
			itemRow << new QStandardItem (sequences.value (1).toString (QKeySequence::NativeText));
			deEdit (itemRow);
			parentRow.at (0)->appendRow (itemRow);

			if (sequences != info [name].Seqs_)
				ihs->SetShortcut (name, sequences);
		}

		Model_->appendRow (parentRow);

		settings.endGroup ();
		settings.endGroup ();

		Ui_.Tree_->resizeColumnToContents (0);
		Ui_.Tree_->expand (parentRow.at (0)->index ());
	}

	QKeySequences_t ShortcutManager::GetShortcuts (QObject *object,
			const QString& originalName)
	{
		for (int i = 0, size = Model_->rowCount (); i < size; ++i)
		{
			auto objectItem = Model_->item (i);
			if (objectItem->data (Roles::Object).value<QObject*> () != object)
				continue;

			for (int j = 0, namesSize = objectItem->rowCount (); j < namesSize; ++j)
			{
				auto item = objectItem->child (j);
				if (item->data (Roles::OriginalName).toString () == originalName)
					return item->data (Roles::Sequence).value<QKeySequences_t> ();
			}
			return QKeySequences_t ();
		}
		AddObject (object);
		return GetShortcuts (object, originalName);
	}

	int ShortcutManager::GetObjectRow (QObject *object) const
	{
		for (int i = 0, size = Model_->rowCount (); i < size; ++i)
		{
			const auto objectItem = Model_->item (i);
			const auto o = objectItem->data (Roles::Object).value<QObject*> ();
			if (o == object)
				return i;
		}

		return -1;
	}

	void ShortcutManager::on_Tree__activated (const QModelIndex& prIndex)
	{
		const auto& index = Filter_->mapToSource (prIndex);
		auto item = Model_->itemFromIndex (index.sibling (index.row (), 0));
		// Root or something
		if (!item || item->data (Roles::OriginalName).isNull ())
			return;

		KeySequencer dia (prIndex.column () == 2 ?
					tr ("Set alternate shortcut:") :
					tr ("Set primary shortcut:"),
				this);
		if (dia.exec () == QDialog::Rejected)
			return;

		if (item->data (Roles::OldSequence).isNull ())
			item->setData (item->data (Roles::Sequence), Roles::OldSequence);

		const int numSeqs = 2;

		auto newSeqs = item->data (Roles::Sequence).value<QKeySequences_t> ();
		while (newSeqs.size () < numSeqs)
			newSeqs << QKeySequence ();

		newSeqs [std::max (prIndex.column () - 1, 0)] = dia.GetResult ();
		newSeqs.removeAll (QKeySequence ());

		item->setData (QVariant::fromValue<QKeySequences_t> (newSeqs), Roles::Sequence);

		for (int i = 0; i < numSeqs; ++i)
			Model_->itemFromIndex (index.sibling (index.row (), i + 1))->
					setText (newSeqs.value (i).toString (QKeySequence::NativeText));
	}

	void ShortcutManager::accept ()
	{
		QSettings settings ("Deviant", "Leechcraft");
		settings.beginGroup ("Shortcuts");
		for (int i = 0, size = Model_->rowCount (); i < size; ++i)
		{
			auto objectItem = Model_->item (i);

			for (int j = 0, namesSize = objectItem->rowCount (); j < namesSize; ++j)
			{
				QObject *o = objectItem->data (Roles::Object).value<QObject*> ();
				IInfo *ii = qobject_cast<IInfo*> (o);
				IHaveShortcuts *ihs = qobject_cast<IHaveShortcuts*> (o);
				settings.beginGroup (ii->GetName ());

				auto item = objectItem->child (j);
				if (!item->data (Roles::OldSequence).isNull ())
				{
					QString name = item->data (Roles::OriginalName).toString ();
					const auto& sequences = item->data (Roles::Sequence).value<QKeySequences_t> ();

					settings.setValue (name, QVariant::fromValue<QKeySequences_t> (sequences));
					item->setData (QVariant (), Roles::OldSequence);
					ihs->SetShortcut (name, sequences);
				}

				settings.endGroup ();
			}
		}
		settings.endGroup ();
	}

	void ShortcutManager::reject ()
	{
		for (int i = 0, size = Model_->rowCount (); i < size; ++i)
		{
			auto objectItem = Model_->item (i);
			for (int j = 0, namesSize = objectItem->rowCount (); j < namesSize; ++j)
			{
				auto item = objectItem->child (j);
				if (item->data (Roles::OldSequence).isNull ())
					continue;

				const auto& seq = item->data (Roles::OldSequence).value<QKeySequence> ();
				item->setData (seq, Roles::Sequence);
				item->setData (QVariant (), Roles::OldSequence);

				objectItem->child (j, 1)->setText (seq.toString (QKeySequence::NativeText));
			}
		}
	}
}
