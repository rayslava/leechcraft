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

#ifndef UTIL_FLOWLAYOUT_H
#define UTIL_FLOWLAYOUT_H
#include <QLayout>
#include <QStyle>
#include <util/utilconfig.h>

namespace LeechCraft
{
namespace Util
{
	/** @brief A simple flow layout implementation.
	 *
	 * Flow layout arranges child items in a dynamic wrappable row, much
	 * like QML's GridView.
	 */
	class UTIL_API FlowLayout : public QLayout
	{
		QList<QLayoutItem*> ItemList_;
		int HSpace_;
		int VSpace_;
	public:
		FlowLayout (QWidget*, int = -1, int = -1, int = -1);
		FlowLayout (int = -1, int = -1, int = -1);
		virtual ~FlowLayout ();

		void addItem (QLayoutItem*);
		int horizontalSpacing () const;
		int verticalSpacing () const;
		Qt::Orientations expandingDirections () const;
		bool hasHeightForWidth () const;
		int heightForWidth (int) const;
		int count () const;
		QLayoutItem* itemAt (int) const;
		QLayoutItem* takeAt (int);
		QSize minimumSize () const;
		void setGeometry (const QRect&);
		QSize sizeHint () const;
	private:
		int DoLayout (const QRect&, bool) const;
		int SmartSpacing (QStyle::PixelMetric) const;
	};
}
}

#endif
