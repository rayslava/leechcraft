/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2013  Oleg Linkin
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

#include "tagsproxymodel.h"
#include "blogiquewidget.h"

namespace LeechCraft
{
namespace Blogique
{
	TagsProxyModel::TagsProxyModel (QObject *parent)
	: QSortFilterProxyModel (parent)
	{
		setDynamicSortFilter (true);
		setFilterCaseSensitivity (Qt::CaseInsensitive);
	}

	bool TagsProxyModel::filterAcceptsRow (int sourceRow,
			const QModelIndex& sourceParent) const
	{
		QModelIndex index = sourceModel ()->index (sourceRow, 0, sourceParent);

		return sourceModel ()->data (index).toString ()
				.startsWith (filterRegExp ().pattern ());
	}

	bool TagsProxyModel::lessThan (const QModelIndex& left, const QModelIndex& right) const
	{
		const int leftData = sourceModel ()->data (left, BlogiqueWidget::TagFrequency)
				.toInt ();
		const int rightData = sourceModel ()->data (right, BlogiqueWidget::TagFrequency)
				.toInt ();

		return leftData > rightData;
	}

	int TagsProxyModel::GetCount () const
	{
		return rowCount ();
	}

	QString TagsProxyModel::GetTagName (int idx)
	{
		return mapToSource (index (idx, 0)).data ().toString ();
	}

	void TagsProxyModel::countUpdated ()
	{
		emit countChanged ();
	}
}
}