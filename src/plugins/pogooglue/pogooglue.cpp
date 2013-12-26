/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
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

#include "pogooglue.h"
#include <QIcon>
#include <QUrl>
#include <util/util.h>
#include <interfaces/entitytesthandleresult.h>

namespace LeechCraft
{
namespace Pogooglue
{
	void Plugin::Init (ICoreProxy_ptr)
	{
		Util::InstallTranslator ("pogooglue");
	}

	void Plugin::SecondInit ()
	{
	}

	void Plugin::Release ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Pogooglue";
	}

	QString Plugin::GetName () const
	{
		return "Pogooglue";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Allows one to search for selected text in Google in two clicks.");
	}

	QIcon Plugin::GetIcon () const
	{
		return QIcon ();
	}

	EntityTestHandleResult Plugin::CouldHandle (const Entity& e) const
	{
		if (e.Mime_ != "x-leechcraft/data-filter-request" ||
				!e.Entity_.canConvert<QString> ())
			return EntityTestHandleResult ();

		if (e.Additional_.contains ("DataFilter"))
		{
			const auto& rawCat = e.Additional_ ["DataFilter"].toByteArray ();
			const auto& catStr = QString::fromUtf8 (rawCat.data ());
			const auto& vars = GetFilterVariants ();
			if (std::find_if (vars.begin (), vars.end (),
					[&catStr] (decltype (vars.front ()) var)
						{ return var.ID_ == catStr; }) == vars.end ())
				return EntityTestHandleResult ();
		}

		const auto& str = e.Entity_.toString ();
		return str.size () < 200 && str.count ("\n") < 3 ?
				EntityTestHandleResult (EntityTestHandleResult::PIdeal) :
				EntityTestHandleResult ();
	}

	void Plugin::Handle (Entity e)
	{
		const auto& str = e.Entity_.toString ();
		GoogleIt (str);
	}

	QString Plugin::GetFilterVerb () const
	{
		return tr ("Google it!");
	}

	QList<IDataFilter::FilterVariant> Plugin::GetFilterVariants () const
	{
		return { { GetUniqueID () + "_Google", "Google", "Google", {} } };
	}

	void Plugin::GoogleIt (QString text)
	{
		QString urlStr = QString ("http://www.google.com/search?q=%2"
				"&client=leechcraft_poshuku"
				"&ie=utf-8"
				"&rls=org.leechcraft:%1")
			.arg (QLocale::system ().name ().replace ('_', '-'))
			.arg (QString::fromUtf8 (QUrl::toPercentEncoding (text)));
		QUrl result = QUrl::fromEncoded (urlStr.toUtf8 ());

		const auto& e = Util::MakeEntity (result,
				QString (),
				LeechCraft::FromUserInitiated | LeechCraft::OnlyHandle);
		emit gotEntity (e);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_pogooglue, LeechCraft::Pogooglue::Plugin);
