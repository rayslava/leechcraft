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

#include "presenterwidget.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTimer>

namespace LeechCraft
{
namespace Monocle
{
	PresenterWidget::PresenterWidget (IDocument_ptr doc)
	: QWidget (0, Qt::Window | Qt::WindowStaysOnTopHint)
	, PixmapLabel_ (new QLabel)
	, Doc_ (doc)
	, CurrentPage_ (0)
	{
		setStyleSheet ("background-color: black;");

		auto lay = new QHBoxLayout ();
		lay->setSpacing (0);
		lay->setContentsMargins (0, 0, 0, 0);
		lay->addWidget (PixmapLabel_, 0, Qt::AlignVCenter | Qt::AlignHCenter);
		setLayout (lay);

		showFullScreen ();

		QTimer::singleShot (50,
				this,
				SLOT (delayedShowInit ()));
	}

	void PresenterWidget::NavigateTo (int page)
	{
		if (page < 0 || page >= Doc_->GetNumPages ())
			return;

		CurrentPage_ = page;

		const auto& pageSize = Doc_->GetPageSize (page);

		auto scale = std::min (static_cast<double> (width ()) / pageSize.width (),
				static_cast<double> (height ()) / pageSize.height ());

		const auto& img = Doc_->RenderPage (page, scale, scale);

		PixmapLabel_->setFixedSize (img.size ());
		PixmapLabel_->setPixmap (QPixmap::fromImage (img));
	}

	void PresenterWidget::closeEvent (QCloseEvent *event)
	{
		deleteLater ();
		QWidget::closeEvent (event);
	}

	void PresenterWidget::keyPressEvent (QKeyEvent *event)
	{
		switch (event->key ())
		{
		case Qt::Key_Escape:
		case Qt::Key_Enter:
			deleteLater ();
			return;
		case Qt::Key_Backspace:
		case Qt::Key_PageUp:
		case Qt::Key_Left:
			NavigateTo (CurrentPage_ - 1);
			break;
		case Qt::Key_Space:
		case Qt::Key_PageDown:
		case Qt::Key_Right:
			NavigateTo (CurrentPage_ + 1);
			break;
		case Qt::Key_Home:
			NavigateTo (0);
			break;
		case Qt::Key_End:
			NavigateTo (Doc_->GetNumPages () - 1);
			break;
		}

		QWidget::keyPressEvent (event);
	}

	void PresenterWidget::delayedShowInit ()
	{
		NavigateTo (CurrentPage_);
	}
}
}
