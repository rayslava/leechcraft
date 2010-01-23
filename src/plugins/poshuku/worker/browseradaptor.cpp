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

#include "browseradaptor.h"
#include "browser.h"
#include <QApplication>
#include <QTimer>
#include <QUrl>
#include <QtDebug>

namespace LeechCraft
{
	namespace Plugins
	{
		namespace Poshuku
		{
			namespace Worker
			{
				BrowserAdaptor::BrowserAdaptor (Browser *b)
				: QDBusAbstractAdaptor (b)
				, Browser_ (b)
				{
				}

				void BrowserAdaptor::LoadURL (const QByteArray& encoded)
				{
					qDebug () << Q_FUNC_INFO << encoded;
					Browser_->LoadURL (QUrl::fromEncoded (encoded));
				}

				void BrowserAdaptor::SetEmbedContainer (qulonglong wid)
				{
					Browser_->SetEmbedContainer (wid);
				}
				
				void BrowserAdaptor::Shutdown ()
				{
					QTimer::singleShot (0,
							qApp,
							SLOT (quit ()));
				}
			};
		};
	};
};

