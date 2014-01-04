/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "keywords.h"
#include <QApplication>
#include <QKeyEvent>
#include <QDebug>
#include <QLineEdit>
#include <QUrl>
#include <util/util.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "keywordsmanagerwidget.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Poshuku
{
namespace Keywords
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("poshuku_keywords");

		CoreProxy_ = proxy;
		Model_.reset (new QStandardItemModel);
		Model_->setHorizontalHeaderLabels (QStringList (tr ("Keyword"))
			<< tr ("Url"));

		QSettings keywords (QCoreApplication::organizationName (),
			QCoreApplication::applicationName () + "_Poshuku_Keywords");

		Q_FOREACH (const QString& keyword, keywords.allKeys ())
		{
			const QString& url = keywords.value (keyword).toString ();
			QStandardItem *keywordItem = new QStandardItem (keyword);
			QStandardItem *urlItem = new QStandardItem (url);
			QList<QStandardItem*> items;

			items << keywordItem << urlItem;
			Model_->appendRow (items);
			UpdateKeywords (keyword, url);
		}

		SettingsDialog_.reset (new Util::XmlSettingsDialog);
		SettingsDialog_->RegisterObject (XmlSettingsManager::Instance (),
			"poshukukeywordssettings.xml");
		SettingsDialog_->SetCustomWidget ("KeywordsManagerWidget",
			new KeywordsManagerWidget (Model_.get (), this));
	}

	void Plugin::SecondInit ()
	{
	}

	void Plugin::Release ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Poshuku.Keywords";
	}

	QString Plugin::GetName () const
	{
		return "Poshuku Keywords";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("URL keywords support for the Poshuku browser.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/plugins/poshuku/plugins/keywords/resources/images/keywords.svg");
		return icon;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Poshuku.Plugins/1.0";
		return result;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return SettingsDialog_;
	}

	void Plugin::UpdateKeywords (const QString& keyword, const QString& url)
	{
		Keywords2Urls_ [keyword] = url;
	}

	void Plugin::RemoveKeyword (const QString& keyword)
	{
		Keywords2Urls_.remove (keyword);
	}

	void Plugin::hookURLEditReturnPressed (LeechCraft::IHookProxy_ptr proxy,
			QObject *browserWidget)
	{
		QLineEdit *urlEdit = 0;

		QMetaObject::invokeMethod (browserWidget,
			"getAddressBar",
			Q_RETURN_ARG (QLineEdit*, urlEdit));

		if (!urlEdit)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable get url edit"
					<< "from"
					<< browserWidget;
			return;
		}

		if (urlEdit->text ().isEmpty ())
			return;

		QStringList keywords = urlEdit->text ().split (" ", QString::SkipEmptyParts);

		if (keywords.isEmpty())
			return;

		QString redirect = Keywords2Urls_.value (keywords.takeFirst ());

		if (redirect.isEmpty ())
			return;

		while (!keywords.isEmpty ())
			redirect = redirect.arg (keywords.takeFirst ());

		urlEdit->setText (redirect);
		QMetaObject::invokeMethod (urlEdit, "returnPressed", Qt::QueuedConnection);
		proxy->CancelDefault ();
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_poshuku_keywords, LeechCraft::Poshuku::Keywords::Plugin);
