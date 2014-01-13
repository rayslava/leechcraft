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

#include "core.h"
#include "editorpage.h"

namespace LeechCraft
{
namespace Popishu
{
	Core::Core ()
	{
		TabClass_.TabClass_ = "Popishu";
		TabClass_.VisibleName_ = "Popishu";
		TabClass_.Description_ = tr ("The Popishu text editor");
		TabClass_.Icon_ = QIcon ("lcicons:/resources/images/popishu.svg");
		TabClass_.Priority_ = 70;
		TabClass_.Features_ = TFOpenableByRequest | TFSuggestOpening;
	}

	Core& Core::Instance ()
	{
		static Core c;
		return c;
	}

	TabClassInfo Core::GetTabClass () const
	{
		return TabClass_;
	}

	void Core::SetProxy (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
	}

	ICoreProxy_ptr Core::GetProxy () const
	{
		return Proxy_;
	}

	EditorPage* Core::NewTabRequested ()
	{
		EditorPage *page = MakeEditorPage ();
		emit addNewTab ("Popishu", page);
		emit raiseTab (page);
		emit changeTabIcon (page, QIcon ("lcicons:/resources/images/popishu.svg"));

		return page;
	}

	void Core::Handle (const Entity& e)
	{
		EditorPage *page = NewTabRequested ();
		page->SetText (e.Entity_.toString ());

		QString language = e.Additional_ ["Language"].toString ();
		bool isTempDocumnet = e.Additional_ ["IsTemporaryDocument"].toBool ();
		if (!language.isEmpty ())
			page->SetLanguage (language);
		page->SetTemporaryDocument (isTempDocumnet);
	}

	EditorPage* Core::MakeEditorPage ()
	{
		EditorPage *result = new EditorPage ();
		connect (result,
				SIGNAL (removeTab (QWidget*)),
				this,
				SIGNAL (removeTab (QWidget*)));
		connect (result,
				SIGNAL (changeTabName (QWidget*, const QString&)),
				this,
				SIGNAL (changeTabName (QWidget*, const QString&)));
		connect (result,
				SIGNAL (couldHandle (const LeechCraft::Entity&, bool*)),
				this,
				SIGNAL (couldHandle (const LeechCraft::Entity&, bool*)));
		connect (result,
				SIGNAL (delegateEntity (const LeechCraft::Entity&,
						int*, QObject**)),
				this,
				SIGNAL (delegateEntity (const LeechCraft::Entity&,
						int*, QObject**)));
		connect (result,
				SIGNAL (gotEntity (const LeechCraft::Entity&)),
				this,
				SIGNAL (gotEntity (const LeechCraft::Entity&)));
		return result;
	}
}
}
