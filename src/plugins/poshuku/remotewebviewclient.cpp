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
#include <QUrl>
#include <QWidget>
#include <QDBusReply>
#include <QMessageBox>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QX11EmbedContainer>
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
			, Child_ (new QProcess (this))
			, Container_ (new QX11EmbedContainer ())
			{
				Container_->setWindowTitle ("Worker view container");
				Container_->show ();
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
				connect (Child_,
						SIGNAL (finished (int, QProcess::ExitStatus)),
						this,
						SLOT (handleFinished (int, QProcess::ExitStatus)));
				Child_->start ("leechcraft_poshuku_worker");
			}

			RemoteWebViewClient::~RemoteWebViewClient ()
			{
				Container_->discardClient ();
				if (ClientInterface_ &&
						ClientInterface_->isValid ())
					ClientInterface_->call ("Shutdown");
			}
			
			QWidget* RemoteWebViewClient::GetWidget () const
			{
				return Container_;
			}

			void RemoteWebViewClient::Load (const QUrl& url)
			{
				if (ClientInterface_ &&
						Container_->clientWinId ())
				{
					qDebug () << Q_FUNC_INFO << url;
					if (ClientInterface_->call ("LoadURL", url.toEncoded ())
							.type () == QDBusMessage::ErrorMessage)
						qWarning () << Q_FUNC_INFO
							<< ClientInterface_->lastError ();
				}
				else
					PendingURL_ = url;
			}

			void RemoteWebViewClient::SetHtml (const QString& html, const QUrl& base)
			{
				if (ClientInterface_ &&
						Container_->clientWinId ())
					ClientInterface_->call ("SetHtml", html, base.toEncoded ());
				else
					PendingHtml_ = qMakePair (html, base);
			}

			qint64 RemoteWebViewClient::GetID () const
			{
				return ID_;
			}

			void RemoteWebViewClient::HandleReady (const QString& service, const QString& path)
			{
				ClientInterface_.reset (new QDBusInterface (service, path));
				if (!ClientInterface_->isValid ())
				{
					qWarning () << Q_FUNC_INFO
						<< service
						<< path
						<< "failed to connect";
					return;
				}

				InitiateEmbedding ();
			}

			void RemoteWebViewClient::InitiateEmbedding ()
			{
				QDBusReply<qulonglong> clReply = ClientInterface_->call ("GetEmbedWidget");
				if (!clReply.isValid ())
				{
					qWarning () << Q_FUNC_INFO
						<< "failed to get embed widget"
						<< clReply.error ().message ();
					Child_->kill ();
					return;
				}
				Container_->embedClient (clReply.value ());

				connect (Container_,
						SIGNAL (clientIsEmbedded ()),
						this,
						SLOT (handleClientIsEmbedded ()));
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
				qWarning () << tr ("Process error: %1 %2")
							.arg (error)
							.arg (Child_->errorString ());
			}

			void RemoteWebViewClient::handleStarted ()
			{
				Child_->write (QString ("%1 %2")
						.arg (GetServiceName ())
						.arg (GetPath ()).toUtf8 ());
				Child_->closeWriteChannel ();
			}

			void RemoteWebViewClient::handleFinished (int ecode, QProcess::ExitStatus est)
			{
				if (!ecode && est == QProcess::NormalExit)
					qDebug () << Q_FUNC_INFO
						<< "child successfully finished for"
						<< GetServiceName ()
						<< GetPath ();
				else
					qWarning () << Q_FUNC_INFO
						<< "child finished badly"
						<< ecode
						<< est
						<< "for"
						<< GetServiceName ()
						<< GetPath ();
				ClientInterface_.reset ();
			}

			void RemoteWebViewClient::handleClientIsEmbedded ()
			{
				qDebug () << Q_FUNC_INFO;

				emit viewIsReady ();

				if (PendingURL_.isValid ())
					Load (PendingURL_);
				else if (PendingHtml_.first.size ())
					SetHtml (PendingHtml_.first, PendingHtml_.second);
			}

			void RemoteWebViewClient::handleClientError (QX11EmbedContainer::Error e)
			{
				qWarning () << Q_FUNC_INFO
					<< e;
			}
		};
	};
};

