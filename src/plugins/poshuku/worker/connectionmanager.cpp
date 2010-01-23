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

#include "connectionmanager.h"
#include <QDBusError>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QMessageBox>
#include <QApplication>
#include <QTimer>
#include <QUrl>
#include <QtDebug>
#include "browser.h"
#include "browseradaptor.h"

namespace LeechCraft
{
	namespace Plugins
	{
		namespace Poshuku
		{
			namespace Worker
			{
				ConnectionManager::ConnectionManager (const QString& service,
						const QString& path, QObject *parent)
				: QObject (parent)
				, BusConnection_ (QDBusConnection::sessionBus ())
				, Service_ (service)
				, Path_ (path)
				, ID_ (-1)
				{
					QTimer::singleShot (0,
							this,
							SLOT (delayedInit ()));
				}

				bool ConnectionManager::Connect ()
				{
					qDebug () << Q_FUNC_INFO << Service_ << Path_;
					if (!BusConnection_.isConnected ())
					{
						QDBusError error = BusConnection_.lastError ();
						QString message = error.message ();
						qWarning () << Q_FUNC_INFO
							<< message
							<< error.type ();
						QMessageBox::critical (0,
								tr ("Poshuku Worker"),
								tr ("Bus error: %1")
									.arg (message));
						return false;
					}
					
					if (!BusConnection_.interface ()->
							isServiceRegistered (Service_))
					{
						qWarning () << Q_FUNC_INFO
							<< Service_
							<< "not registered";
						QMessageBox::critical (0,
								tr ("Poshuku Worker"),
								tr ("Declared service %1 is not registered "
									"in the session bus.")
									.arg (Service_));
						return false;
					}

					ServerInterface_.reset (new QDBusInterface (Service_, Path_,
								"org.LeechCraft.Poshuku.IWorkerServer"));
					if (!ServerInterface_->isValid ())
					{
						qWarning () << Q_FUNC_INFO
							<< Service_
							<< Path_
							<< "failed to connect";
						return false;
					}

					return true;
				}

				bool ConnectionManager::Prepare ()
				{
					QDBusReply<qint64> idReply = ServerInterface_->call ("GetID");
					if (!idReply.isValid ())
					{
						qWarning () << Q_FUNC_INFO
							<< idReply.error ().message ()
							<< idReply.error ().type ();
						QMessageBox::critical (0,
								tr ("Poshuku Worker"),
								tr ("Could not get ID: %1")
									.arg (idReply.error ().message ()));
						return false;
					}

					ID_ = idReply.value ();

					Browser_.reset (new Browser (this));
					new BrowserAdaptor (Browser_.get ());

					QString service = QString ("org.LeechCraft.Poshuku.Worker-%1").arg (ID_);
					QString path = QString ("/RemoteWebView");
					qDebug () << service << path;
					BusConnection_.registerService (service);
					BusConnection_.registerObject (path,
							Browser_.get ());

					ServerInterface_->asyncCall ("Ready", service, path);

					return true;
				}

				void ConnectionManager::delayedInit ()
				{
					if (!Connect () || !Prepare ())
						qApp->quit ();
				}
			};
		};
	};
};

