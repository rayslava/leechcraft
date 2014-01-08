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

#include "knowndictsmanager.h"
#include <QFileSystemWatcher>
#include <QStandardItemModel>
#include <QStringListModel>
#include <util/util.h>
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Rosenthal
{
	namespace
	{
		QStringList GetSystemPaths ()
		{
			QStringList candidates;
#ifdef Q_OS_WIN32
			candidates << qApp->applicationDirPath () + "/myspell/";
#else
			candidates << "/usr/local/share/myspell/"
					<< "/usr/share/myspell/"
					<< "/usr/local/share/myspell/dicts/"
					<< "/usr/share/myspell/dicts/"
					<< "/usr/local/share/hunspell/"
					<< "/usr/share/hunspell/";
#endif

			return candidates;
		}
	}

	KnownDictsManager::KnownDictsManager ()
	: LocalPath_ (Util::CreateIfNotExists ("data/dicts/myspell").absolutePath ())
	, Model_ (new QStandardItemModel (this))
	, EnabledModel_ (new QStringListModel (this))
	{
		auto watcher = new QFileSystemWatcher (this);
		watcher->addPath (LocalPath_);
		for (const auto& path : GetSystemPaths ())
			if (QFile::exists (path))
				watcher->addPath (path);

		LoadSettings ();

		connect (watcher,
				SIGNAL (directoryChanged (QString)),
				this,
				SLOT (rebuildDictsModel ()));
		rebuildDictsModel ();

		connect (Model_,
				SIGNAL (itemChanged (QStandardItem*)),
				this,
				SLOT (handleItemChanged (QStandardItem*)));
		XmlSettingsManager::Instance ().RegisterObject ("PrimaryLanguage",
				this, "reemitLanguages");
	}

	QAbstractItemModel* KnownDictsManager::GetModel () const
	{
		return Model_;
	}

	QStringList KnownDictsManager::GetLanguages () const
	{
		auto langs = Languages_;
		const auto& primary = XmlSettingsManager::Instance ()
				.property ("PrimaryLanguage").toString ();
		if (langs.removeOne (primary))
			langs.prepend (primary);
		return langs;
	}

	QString KnownDictsManager::GetDictPath (const QString& language) const
	{
		return Lang2Path_ [language] + language;
	}

	QAbstractItemModel* KnownDictsManager::GetEnabledModel () const
	{
		return EnabledModel_;
	}

	void KnownDictsManager::LoadSettings ()
	{
		const QStringList defLangs { Util::GetLocaleName (), "en_GB" };
		Languages_ = XmlSettingsManager::Instance ()
				.Property ("EnabledLanguages", defLangs).toStringList ();
		EnabledModel_->setStringList (Languages_);
	}

	void KnownDictsManager::SaveSettings ()
	{
		XmlSettingsManager::Instance ().setProperty ("EnabledLanguages", Languages_);
	}

	void KnownDictsManager::rebuildDictsModel ()
	{
		auto candidates = GetSystemPaths ();
		candidates.prepend (LocalPath_);

		Lang2Path_.clear ();
		for (const auto& dir : candidates)
		{
			if (!QFile::exists (dir))
				continue;

			for (auto file : QDir (dir).entryList ({ "*.dic" }))
			{
				if (file.startsWith ("hyph_"))
					continue;

				file.chop (4);
				if (Lang2Path_.contains (file))
					continue;

				Lang2Path_ [file] = dir;
			}
		}

		Model_->clear ();
		Model_->setHorizontalHeaderLabels ({ tr ("Locale"), tr ("Language"), tr ("Country") });

		for (auto i = Lang2Path_.begin (); i != Lang2Path_.end (); ++i)
		{
			auto item = new QStandardItem (i.key ());
			item->setCheckable (true);
			item->setCheckState (Languages_.contains (i.key ()) ? Qt::Checked : Qt::Unchecked);

			const QLocale loc (i.key ());

			QList<QStandardItem*> row { item };
			row << new QStandardItem (loc.nativeLanguageName ());
			row << new QStandardItem (loc.nativeCountryName ());

			for (auto item : row)
				item->setEditable (false);

			Model_->appendRow (row);
		}
	}

	void KnownDictsManager::handleItemChanged (QStandardItem *item)
	{
		if (item->column ())
			return;

		const auto& lang = item->text ();
		const auto enabledNow = item->checkState () == Qt::Checked;
		const auto enabled = Languages_.contains (lang);

		if (enabled == enabledNow)
			return;

		if (enabledNow)
			Languages_ << lang;
		else
			Languages_.removeAll (lang);

		EnabledModel_->setStringList (Languages_);

		reemitLanguages() ;

		SaveSettings ();
	}

	void KnownDictsManager::reemitLanguages ()
	{
		emit languagesChanged (GetLanguages ());
	}
}
}
}
