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
#include <QMessageBox>
#include <QApplication>
#include <QTimer>
#include <QtDebug>

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
				{
					QTimer::singleShot (0,
							this,
							SLOT (delayedInit ()));
				}

				bool ConnectionManager::Connect ()
				{
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

					return true;
				}

				void ConnectionManager::delayedInit ()
				{
					if (!Connect ())
						qApp->quit ();
				}
			};
		};
	};
};

