/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2009  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "embedwidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include "customwebview.h"

namespace LeechCraft
{
	namespace Plugins
	{
		namespace Poshuku
		{
			namespace Worker
			{
				EmbedWidget::EmbedWidget (QWidget *parent)
				: QX11EmbedWidget (parent)
				, WebView_ (new CustomWebView ())
				{
					show ();
					connect (this,
							SIGNAL (embedded ()),
							this,
							SLOT (handleEmbedded ()));
					connect (this,
							SIGNAL (error (QX11EmbedWidget::Error)),
							this,
							SLOT (handleError (QX11EmbedWidget::Error)));

					QVBoxLayout *lay = new QVBoxLayout (this);
					lay->addWidget (new QPushButton ("push me 1!"));
					lay->addWidget (WebView_);
					lay->addWidget (new QPushButton ("push me 2!"));
					lay->setContentsMargins (0, 0, 0, 0);
					setLayout (lay);
				}

				CustomWebView* EmbedWidget::GetWebView () const
				{
					return WebView_;
				}

				void EmbedWidget::handleEmbedded ()
				{
					qDebug () << Q_FUNC_INFO;
				}

				void EmbedWidget::handleError (QX11EmbedWidget::Error e)
				{
					qWarning () << e;
				}
			};
		};
	};
};

