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

#include "item.h"
#include <stdexcept>
#include <QFile>
#include <QUrl>
#include <QProcess>
#include <util/util.h>
#include <interfaces/core/ientitymanager.h>
#include "desktopparser.h"

namespace LeechCraft
{
namespace Util
{
namespace XDG
{
	bool Item::operator== (const Item& item) const
	{
		return IsHidden_ == item.IsHidden_ &&
				Name_ == item.Name_ &&
				GenericName_ == item.GenericName_ &&
				Comments_ == item.Comments_ &&
				Categories_ == item.Categories_ &&
				Command_ == item.Command_ &&
				WD_ == item.WD_ &&
				IconName_ == item.IconName_;
	}

	bool Item::IsValid () const
	{
		return !Name_.isEmpty ();
	}

	bool Item::IsHidden () const
	{
		return IsHidden_;
	}

	void Item::Execute (ICoreProxy_ptr proxy) const
	{
		auto command = GetCommand ();

		if (GetType () == Type::Application)
		{
			command.remove ("%c");
			command.remove ("%f");
			command.remove ("%F");
			command.remove ("%u");
			command.remove ("%U");
			command.remove ("%i");
			auto items = command.split (' ', QString::SkipEmptyParts);
			auto removePred = [] (const QString& str)
				{ return str.size () == 2 && str.at (0) == '%'; };
			items.erase (std::remove_if (items.begin (), items.end (), removePred),
					items.end ());
			if (items.isEmpty ())
				return;

			QProcess::startDetached (items.at (0), items.mid (1), GetWorkingDirectory ());
		}
		else if (GetType () == Type::URL)
		{
			const auto& e = Util::MakeEntity (QUrl (command),
					QString (),
					FromUserInitiated | OnlyHandle);
			proxy->GetEntityManager ()->HandleEntity (e);
		}
		else
		{
			qWarning () << Q_FUNC_INFO
					<< "don't know how to execute this type of app";
		}
	}

	namespace
	{
		QString ByLang (const QHash<QString, QString>& cont, const QString& lang)
		{
			return cont.value (cont.contains (lang) ? lang : QString ());
		}
	}

	QString Item::GetName (const QString& lang) const
	{
		return ByLang (Name_, lang);
	}

	QString Item::GetGenericName (const QString& lang) const
	{
		return ByLang (GenericName_, lang);
	}

	QString Item::GetComment (const QString& lang) const
	{
		return ByLang (Comments_, lang);
	}

	QString Item::GetIconName () const
	{
		return IconName_;
	}

	QStringList Item::GetCategories () const
	{
		return Categories_;
	}

	Type Item::GetType () const
	{
		return Type_;
	}

	QString Item::GetCommand () const
	{
		return Command_;
	}

	QString Item::GetWorkingDirectory () const
	{
		return WD_;
	}

	QString Item::GetPermanentID () const
	{
		return GetCommand ();
	}

	void Item::SetIcon (const QIcon& icon)
	{
		Icon_ = icon;
	}

	QIcon Item::GetIcon () const
	{
		return Icon_;
	}

	QDebug Item::DebugPrint (QDebug dbg) const
	{
		dbg.nospace () << "DesktopItem\n{\n\tNames: " << Name_
				<< "\n\tGenericNames: " << GenericName_
				<< "\n\tComments: " << Comments_
				<< "\n\tCategories: " << Categories_
				<< "\n\tCommand: " << Command_
				<< "\n\tWorkingDir: " << WD_
				<< "\n\tIconName: " << IconName_
				<< "\n\tHidden: " << IsHidden_
				<< "\n}\n";
		return dbg.space ();
	}

	namespace
	{
		QHash<QString, QString> FirstValues (const QHash<QString, QStringList>& hash)
		{
			QHash<QString, QString> result;
			for (auto i = hash.begin (), end = hash.end (); i != end; ++i)
				result [i.key ()] = i->value (0);
			return result;
		}
	}

	Item_ptr Item::FromDesktopFile (const QString& filename)
	{
		QFile file (filename);
		if (!file.open (QIODevice::ReadOnly))
			throw std::runtime_error ("Unable to open file");

		const auto& result = Util::XDG::DesktopParser () (file.readAll ());
		const auto& group = result ["Desktop Entry"];

		Item_ptr item (new Item);
		item->Name_ = FirstValues (group ["Name"]);
		item->GenericName_ = FirstValues (group ["GenericName"]);
		item->Comments_ = FirstValues (group ["Comment"]);

		item->Categories_ = group ["Categories"] [QString ()];

		auto getSingle = [&group] (const QString& name) { return group [name] [QString ()].value (0); };

		item->IconName_ = getSingle ("Icon");

		const auto& typeStr = getSingle ("Type");
		if (typeStr == "Application")
		{
			item->Type_ = Type::Application;
			item->Command_ = getSingle ("Exec");
			item->WD_ = getSingle ("Path");
		}
		else if (typeStr == "URL")
		{
			item->Type_ = Type::URL;
			item->Command_ = getSingle ("URL");
		}
		else if (typeStr == "Directory")
			item->Type_ = Type::Dir;
		else
			item->Type_ = Type::Other;

		item->IsHidden_ = getSingle ("NoDisplay").toLower () == "true";

		return item;
	}

	QDebug operator<< (QDebug dbg, const Item& item)
	{
		return item.DebugPrint (dbg);
	}
}
}
}
