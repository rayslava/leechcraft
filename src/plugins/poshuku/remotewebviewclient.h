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

#ifndef PLUGINS_POSHUKU_REMOTEWEBVIEWCLIENT_H
#define PLUGINS_POSHUKU_REMOTEWEBVIEWCLIENT_H
#include <QObject>
#include <QProcess>

class QWidget;

namespace LeechCraft
{
	namespace Plugins
	{
		namespace Poshuku
		{
			class RemoteWebViewClient : public QObject
			{
				Q_OBJECT

				static qint64 CurrentID_;
				qint64 ID_;
				QProcess *Child_;
			public:
				RemoteWebViewClient (QWidget* = 0);

				qint64 GetID () const;
			private:
				QString GetServiceName () const;
				QString GetPath () const;
			private slots:
				void handleError (QProcess::ProcessError);
				void handleStarted ();
			};
		};
	};
};

#endif

