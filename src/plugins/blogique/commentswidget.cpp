/**********************************************************************
 *  LeechCraft - modular cross-platform feature rich internet client.
 *  Copyright (C) 2010-2013  Oleg Linkin <MaledicutsDeMagog@gmail.com>
 *
 *  Boost Software License - Version 1.0 - August 17th, 2003
 *
 *  Permission is hereby granted, free of charge, to any person or organization
 *  obtaining a copy of the software and accompanying documentation covered by
 *  this license (the "Software") to use, reproduce, display, distribute,
 *  execute, and transmit the Software, and to prepare derivative works of the
 *  Software, and to permit third-parties to whom the Software is furnished to
 *  do so, all subject to the following:
 *
 *  The copyright notices in the Software and this entire statement, including
 *  the above license grant, this restriction and the following disclaimer,
 *  must be included in all copies of the Software, in whole or in part, and
 *  all derivative works of the Software, unless such copies or derivative
 *  works are solely in the form of machine-executable object code generated by
 *  a source language processor.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 *  SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 *  FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 *  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 **********************************************************************/

#include "commentswidget.h"
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QGraphicsObject>
#include <util/qml/colorthemeproxy.h>
#include <util/qml/tooltipitem.h>
#include <util/sys/paths.h>
#include <util/util.h>
#include <interfaces/core/ientitymanager.h>
#include "commentsmodel.h"
#include "core.h"
#include "sortcommentsproxymodel.h"
#include "commentsmanager.h"

namespace LeechCraft
{
namespace Blogique
{
	CommentsWidget::CommentsWidget (QWidget *parent)
	: QWidget (parent)
	, CommentsModel_ (new CommentsModel (this))
	, ProxyModel_ (new SortCommentsProxyModel (this, this))
	{
		Ui_.setupUi (this);

		qmlRegisterType<Util::ToolTipItem> ("org.LC.common", 1, 0, "ToolTip");

		ProxyModel_->setSourceModel (CommentsModel_);

		Ui_.CommentsView_->setResizeMode (QDeclarativeView::SizeRootObjectToView);
		auto context = Ui_.CommentsView_->rootContext ();
		context->setContextProperty ("colorProxy",
				new Util::ColorThemeProxy (Core::Instance ()
						.GetCoreProxy ()->GetColorThemeManager (), this));
		context->setContextProperty ("commentsModel", ProxyModel_);
		context->setContextProperty ("parentWidget", this);
		Ui_.CommentsView_->setSource (QUrl::fromLocalFile (Util::GetSysPath (Util::SysPath::QML,
				"blogique", "commentsview.qml")));
		connect (Ui_.CommentsView_->rootObject (),
				SIGNAL (linkActivated (QString)),
				this,
				SLOT (handleLinkActivated (QString)));
		ProxyModel_->sort (0, Qt::AscendingOrder);

		FillModel ();

		connect (Core::Instance ().GetCommentsManager (),
				SIGNAL (gotNewComments (QList<RecentComment>)),
				this,
				SLOT (handleGotNewComments (QList<RecentComment>)));
	}

	QString CommentsWidget::GetName () const
	{
		return tr ("Comments");
	}

	RecentComment CommentsWidget::GetRecentCommentFromIndex (const QModelIndex& index) const
	{
		return Item2RecentComment_.value (CommentsModel_->itemFromIndex (index), RecentComment ());
	}

	void CommentsWidget::FillModel ()
	{
		AddItemsToModel (Core::Instance ().GetCommentsManager ()->GetComments ());
	}

	void CommentsWidget::AddItemsToModel (const QList<RecentComment>& comments)
	{
		for (const auto& comment : comments)
		{
			if (RecentComments_.contains (comment))
				continue;

			QStandardItem *item = new QStandardItem;
			item->setData (comment.EntrySubject_, CommentsModel::EntrySubject);
			item->setData (comment.EntryUrl_, CommentsModel::EntryUrl);
			item->setData (comment.CommentSubject_, CommentsModel::CommentSubject);
			item->setData (comment.CommentText_, CommentsModel::CommentBody);
			item->setData (comment.CommentAuthor_, CommentsModel::CommentAuthor);
			item->setData (comment.CommentDateTime_.toString (Qt::DefaultLocaleShortDate),
						   CommentsModel::CommentDate);

			Item2RecentComment_ [item] = comment;
			RecentComments_ << comment;

			CommentsModel_->appendRow (item);
		}
	}

	void CommentsWidget::handleLinkActivated (const QString& url)
	{
		Core::Instance ().GetCoreProxy ()->GetEntityManager ()->
				HandleEntity (Util::MakeEntity (url,
						QString (),
						OnlyHandle | FromUserInitiated));
	}

	void CommentsWidget::handleGotNewComments(const QList<RecentComment>& comments)
	{
		AddItemsToModel (comments);
	}

	void CommentsWidget::setItemCursor (QGraphicsObject *object, const QString& shape)
	{
		Q_ASSERT (object);

		Qt::CursorShape cursor = (shape == "PointingHandCursor") ?
			Qt::PointingHandCursor :
			Qt::ArrowCursor;

		object->setCursor (QCursor (cursor));
	}
}
}
