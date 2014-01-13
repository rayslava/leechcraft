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

#ifndef UTIL_TREEITEM_H
#define UTIL_TREEITEM_H
#include <QList>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <util/utilconfig.h>

namespace LeechCraft
{
	namespace Util
	{
		class TreeItem
		{
			QList<TreeItem*> Children_;
			QMap<int, QVector<QVariant>> Data_;
			TreeItem *Parent_;
		public:
			UTIL_API TreeItem (const QList<QVariant>&, TreeItem *parent = 0);
			UTIL_API ~TreeItem ();

			UTIL_API void AppendChild (TreeItem*);
			UTIL_API void PrependChild (TreeItem*);
			UTIL_API void InsertChild (int, TreeItem*);
			UTIL_API int ChildPosition (const TreeItem*) const;
			UTIL_API void RemoveChild (int);
			UTIL_API TreeItem* Child (int) const;
			UTIL_API int ChildCount () const;
			UTIL_API int ColumnCount (int = Qt::DisplayRole) const;
			UTIL_API QVariant Data (int, int = Qt::DisplayRole) const;
			UTIL_API void ModifyData (int, const QVariant&, int = Qt::DisplayRole);
			UTIL_API const TreeItem* Parent () const;
			UTIL_API TreeItem* Parent ();
			UTIL_API int Row () const;
		};
	};
};

UTIL_API QDebug operator<< (QDebug, const LeechCraft::Util::TreeItem&);
UTIL_API QDebug operator<< (QDebug, const LeechCraft::Util::TreeItem* const);

#endif

