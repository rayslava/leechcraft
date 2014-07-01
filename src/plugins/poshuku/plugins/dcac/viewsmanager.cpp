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

#include "viewsmanager.h"
#include <QAction>
#include <qwebview.h>
#include "inverteffect.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Poshuku
{
namespace DCAC
{
	ViewsManager::ViewsManager (QObject *parent)
	: QObject { parent }
	{
		XmlSettingsManager::Instance ().RegisterObject ("NightModeThreshold",
				this, "handleThresholdChanged");
	}

	void ViewsManager::AddView (QWebView *view)
	{
		const auto effect = new InvertEffect { view };
		view->setGraphicsEffect (effect);

		View2Effect_ [view] = effect;

		connect (view,
				SIGNAL (destroyed (QObject*)),
				this,
				SLOT (handleViewDestroyed (QObject*)));

		const auto enable = XmlSettingsManager::Instance ()
				.property ("EnableNightModeByDefault").toBool ();
		effect->setEnabled (enable);

		const auto enableAct = new QAction { tr ("Night mode"), view };
		view->addAction (enableAct);
		enableAct->setShortcut (QString { "Ctrl+Shift+I" });
		enableAct->setCheckable (true);
		enableAct->setChecked (enable);
		connect (enableAct,
				SIGNAL (toggled (bool)),
				effect,
				SLOT (setEnabled (bool)));
		View2EnableAction_ [view] = enableAct;

		const auto threshold = XmlSettingsManager::Instance ()
				.property ("NightModeThreshold").toInt ();
		effect->SetThreshold (threshold);
	}

	QAction* ViewsManager::GetEnableAction (QWebView *view) const
	{
		return View2EnableAction_.value (view);
	}

	void ViewsManager::handleViewDestroyed (QObject *view)
	{
		View2Effect_.remove (view);
		View2EnableAction_.remove (view);
	}

	void ViewsManager::handleThresholdChanged ()
	{
		const auto threshold = XmlSettingsManager::Instance ()
				.property ("NightModeThreshold").toInt ();
		for (const auto effect : View2Effect_)
			effect->SetThreshold (threshold);
	}
}
}
}