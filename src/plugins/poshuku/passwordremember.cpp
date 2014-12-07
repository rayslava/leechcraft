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

#include "passwordremember.h"
#include <QtDebug>
#include <util/xpc/util.h>
#include <interfaces/core/ientitymanager.h>
#include "core.h"

namespace LeechCraft
{
namespace Poshuku
{
	PasswordRemember::PasswordRemember (QWidget *parent)
	: Util::PageNotification (parent)
	{
		Ui_.setupUi (this);
	}

	void PasswordRemember::add (const PageFormsData_t& data)
	{
		TempData_ = data;

		show ();
	}

	void PasswordRemember::on_Remember__released ()
	{
		if (TempData_.isEmpty ())
		{
			hide ();
			return;
		}

		QList<QVariant> keys;
		QList<QVariant> values;
		for (const auto& key : TempData_.keys ())
		{
			keys << "org.LeechCraft.Poshuku.Forms.InputByName/" + key.toUtf8 ();
			QVariantList value;
			for (const auto& ed : TempData_ [key])
				value << QVariant::fromValue (ed);
			values << QVariant (value);
		}

		auto e = Util::MakeEntity (keys,
				{},
				Internal,
				"x-leechcraft/data-persistent-save");
		e.Additional_ ["Values"] = values;
		Core::Instance ().GetProxy ()->GetEntityManager ()->HandleEntity (e);

		TempData_.clear ();

		hide ();
	}

	void PasswordRemember::on_NotNow__released ()
	{
		TempData_.clear ();
		hide ();
	}

	void PasswordRemember::on_Never__released ()
	{
		if (TempData_.isEmpty ())
		{
			hide ();
			return;
		}

		QSet<QString> urls;
		for (const auto& key : TempData_.keys ())
			for (const auto& ed : TempData_ [key])
				urls << ed.PageURL_.toString ();

		for (const auto& url : urls)
			Core::Instance ().GetStorageBackend ()->SetFormsIgnored (url, true);

		TempData_.clear ();
		hide ();
	}
}
}
