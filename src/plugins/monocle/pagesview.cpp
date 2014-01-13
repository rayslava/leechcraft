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

#include "pagesview.h"
#include <QMenu>
#include <QMouseEvent>
#include <QTimeLine>
#include "xmlsettingsmanager.h"
#include "documenttab.h"

namespace LeechCraft
{
namespace Monocle
{
	PagesView::PagesView (QWidget *parent)
	: QGraphicsView (parent)
	, ShowReleaseMenu_ (false)
	, ShowOnNextRelease_ (false)
	, ScrollTimeline_ (new QTimeLine (400, this))
	, DocTab_ (0)
	{
		ScrollTimeline_->setFrameRange (0, 100);
		connect (ScrollTimeline_,
				SIGNAL (frameChanged (int)),
				this,
				SLOT (handleSmoothScroll (int)));
	}

	void PagesView::SetDocumentTab (DocumentTab *tab)
	{
		DocTab_ = tab;
	}

	void PagesView::SetShowReleaseMenu (bool show)
	{
		ShowReleaseMenu_ = show;
		ShowOnNextRelease_ = false;
	}

	QPointF PagesView::GetCurrentCenter () const
	{
		const auto& rectSize = viewport ()->contentsRect ().size () / 2;
		return mapToScene (QPoint (rectSize.width (), rectSize.height ()));
	}

	void PagesView::SmoothCenterOn (qreal x, qreal y)
	{
		if (!XmlSettingsManager::Instance ().property ("SmoothScrolling").toBool ())
		{
			centerOn (x, y);
			return;
		}

		const auto& current = GetCurrentCenter ();
		XPath_ = qMakePair (current.x (), x);
		YPath_ = qMakePair (current.y (), y);

		if (ScrollTimeline_->state () != QTimeLine::NotRunning)
			ScrollTimeline_->stop ();
		ScrollTimeline_->start ();
	}

	void PagesView::mouseMoveEvent (QMouseEvent *event)
	{
		if (event->buttons () != Qt::NoButton && ShowReleaseMenu_)
			ShowOnNextRelease_ = true;

		QGraphicsView::mouseMoveEvent (event);
	}

	void PagesView::mouseReleaseEvent (QMouseEvent *event)
	{
		QGraphicsView::mouseReleaseEvent (event);

		if (ShowOnNextRelease_)
		{
			auto menu = new QMenu (this);
			DocTab_->CreateViewCtxMenuActions (menu);
			menu->popup (event->globalPos ());
			menu->setAttribute (Qt::WA_DeleteOnClose);
			menu->show ();

			ShowOnNextRelease_ = false;
		}
	}

	void PagesView::resizeEvent (QResizeEvent *e)
	{
		QGraphicsView::resizeEvent (e);
		emit sizeChanged ();
	}

	void PagesView::handleSmoothScroll (int frame)
	{
		const int endFrame = ScrollTimeline_->endFrame ();
		auto interp = [frame, endFrame] (const QPair<qreal, qreal>& pair)
				{ return pair.first + (pair.second - pair.first) * frame / endFrame; };
		centerOn (interp (XPath_), interp (YPath_));
	}
}
}
