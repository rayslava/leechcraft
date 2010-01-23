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

#include "remotewebviewclient.h"
#include <QWidget>
#include <QMessageBox>
#include <QDBusConnection>
#include "remotewebviewclientadaptor.h"
#include "core.h"

namespace LeechCraft
{
	namespace Plugins
	{
		namespace Poshuku
		{
			qint64 RemoteWebViewClient::CurrentID_ = 0;

			RemoteWebViewClient::RemoteWebViewClient (QWidget *parent)
			: QObject (parent)
			, ID_ (CurrentID_++)
			{
				Child_ = new QProcess (this);

				new RemoteWebViewClientAdaptor (this);

				QDBusConnection::sessionBus ().registerService (GetServiceName ());
				QDBusConnection::sessionBus ().registerObject (GetPath (),
						this);

				connect (Child_,
						SIGNAL (error (QProcess::ProcessError)),
						this,
						SLOT (handleError (QProcess::ProcessError)));
				connect (Child_,
						SIGNAL (started ()),
						this,
						SLOT (handleStarted ()));
				Child_->start ("leechcraft_poshuku_worker");
			}

			QString RemoteWebViewClient::GetServiceName () const
			{
				return "org.LeechCraft.Poshuku.WorkerServers";
			}

			QString RemoteWebViewClient::GetPath () const
			{
				return QString ("/%1").arg (ID_);
			}

			void RemoteWebViewClient::handleError (QProcess::ProcessError error)
			{
				QMessageBox::critical (0,
						tr ("LeechCraft"),
						tr ("Process error: %1 %2")
							.arg (error)
							.arg (Child_->errorString ()));
			}

			void RemoteWebViewClient::handleStarted ()
			{
				Child_->write (QString ("%1 %2").arg (GetServiceName ()).arg (GetPath ()).toUtf8 ());
				Child_->closeWriteChannel ();
			}
		};
	};
};

