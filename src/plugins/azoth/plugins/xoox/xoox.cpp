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

#include "xoox.h"
#include <QIcon>
#include <QTranslator>
#include <util/util.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include <interfaces/azoth/iproxyobject.h>
#include <interfaces/core/ipluginsmanager.h>
#include "glooxprotocol.h"
#include "core.h"
#include "xmlsettingsmanager.h"
#include "rostersaver.h"
#include "capsdatabase.h"
#include "vcardstorage.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("azoth_xoox");

		XSD_.reset (new Util::XmlSettingsDialog);
		XSD_->RegisterObject (&XmlSettingsManager::Instance (), "azothxooxsettings.xml");

		Core::Instance ().SetProxy (proxy);

		connect (&Core::Instance (),
				SIGNAL (gotEntity (LeechCraft::Entity)),
				this,
				SIGNAL (gotEntity (LeechCraft::Entity)));
		connect (&Core::Instance (),
				SIGNAL (delegateEntity (LeechCraft::Entity, int*, QObject**)),
				this,
				SIGNAL (delegateEntity (LeechCraft::Entity, int*, QObject**)));

		const auto& progRep = proxy->GetPluginsManager ()->CreateLoadProgressReporter (this);
		const auto capsDB = new CapsDatabase { progRep };

		VCardStorage_ = std::make_shared<VCardStorage> ();
		GlooxProtocol_ = std::make_shared<GlooxProtocol> (capsDB);
	}

	void Plugin::SecondInit ()
	{
		GlooxProtocol_->SetProxyObject (PluginProxy_);
		GlooxProtocol_->Prepare ();

		new RosterSaver { GlooxProtocol_.get (), PluginProxy_, GlooxProtocol_.get () };
	}

	void Plugin::Release ()
	{
		GlooxProtocol_.reset ();
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Azoth.Xoox";
	}

	QString Plugin::GetName () const
	{
		return "Azoth Xoox";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("XMPP (Jabber) protocol module using the QXmpp library.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/plugins/azoth/plugins/xoox/resources/images/xoox.svg");
		return icon;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> classes;
		classes << "org.LeechCraft.Plugins.Azoth.Plugins.IProtocolPlugin";
		return classes;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}

	QObject* Plugin::GetQObject ()
	{
		return this;
	}

	QList<QObject*> Plugin::GetProtocols () const
	{
		return { GlooxProtocol_.get () };
	}

	void Plugin::initPlugin (QObject *proxy)
	{
		PluginProxy_ = qobject_cast<IProxyObject*> (proxy);
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_azoth_xoox,
		LeechCraft::Azoth::Xoox::Plugin);
