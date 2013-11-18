/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2011  Georg Rudoy
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

#include "tabslist.h"
#include <QIcon>
#include <QAction>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QApplication>
#include <QMainWindow>
#include <QKeyEvent>
#include <QTimer>
#include <util/util.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/irootwindowsmanager.h>
#include <interfaces/core/icoretabwidget.h>

Q_DECLARE_METATYPE (ICoreTabWidget*)

namespace LeechCraft
{
namespace TabsList
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;

		Util::InstallTranslator ("tabslist");

		ShowList_ = new QAction (tr ("List of tabs"), this);
		ShowList_->setProperty ("ActionIcon", "view-list-details");
		ShowList_->setShortcut (QString ("Ctrl+Shift+L"));
		ShowList_->setProperty ("Action/ID", GetUniqueID () + "_showlist");
		connect (ShowList_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleShowList ()));
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.TabsList";
	}

	void Plugin::Release ()
	{
	}

	QString Plugin::GetName () const
	{
		return "TabsList";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Displays the list of current tabs and allows one to select one of them.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/tabslist/resources/images/tabslist.svg");
		return icon;
	}

	QList<QAction*> Plugin::GetActions (ActionsEmbedPlace aep) const
	{
		QList<QAction*> actions;
		if (aep == ActionsEmbedPlace::QuickLaunch)
			actions << ShowList_;
		return actions;
	}

	QMap<QString, ActionInfo> Plugin::GetActionInfo () const
	{
		QMap<QString, ActionInfo> result;
		result ["ShowList"] = ActionInfo (ShowList_->text (),
				ShowList_->shortcut (),
				Proxy_->GetIcon (ShowList_->property ("ActionIcon").toString ()));
		return result;
	}

	void Plugin::SetShortcut (const QString&, const QKeySequences_t& seqs)
	{
		ShowList_->setShortcuts (seqs);
	}

	namespace
	{
		class ListEventFilter : public QObject
		{
			QList<QToolButton*> AllButtons_;
			QString SearchText_;
			
			QTimer NumSelectTimer_;
		public:
			ListEventFilter (const QList<QToolButton*>& buttons, QObject *parent = 0)
			: QObject (parent)
			, AllButtons_ (buttons)
			{
				NumSelectTimer_.setSingleShot (true);
			}
		protected:
			bool eventFilter (QObject *obj, QEvent *event)
			{
				if (event->type () != QEvent::KeyPress)
					return false;

				auto key = static_cast<QKeyEvent*> (event);
				switch (key->key ())
				{
				case Qt::Key_Escape:
					obj->deleteLater ();
					return true;
				case Qt::Key_Backspace:
					SearchText_.chop (1);
					FocusSearch ();
					return true;
				case Qt::Key_Enter:
				case Qt::Key_Return:
					for (auto button : AllButtons_)
						if (button->hasFocus ())
							button->animateClick ();
					return true;
				case Qt::Key_Home:
					AllButtons_.first ()->setFocus ();
					break;
				case Qt::Key_End:
					AllButtons_.last ()->setFocus ();
					break;
				default:
					break;
				}

				if (!key->text ().isEmpty ())
				{
					SearchText_ += key->text ();
					FocusSearch ();
					return true;
				}

				return false;
			}
		private:
			void FocusSearch ()
			{
				bool isNum = false;
				const auto srcNum = SearchText_.toInt (&isNum);
				const auto num = srcNum - 1;
				if (isNum && srcNum >= 0 && srcNum <= AllButtons_.size ())
				{
					if (!srcNum && !AllButtons_.isEmpty ())
						AllButtons_.last ()->animateClick ();
					else if (srcNum * 10 - 1 >= AllButtons_.size ())
						AllButtons_ [num]->animateClick ();
					else
					{
						if (NumSelectTimer_.isActive ())
						{
							NumSelectTimer_.stop ();
							disconnect (&NumSelectTimer_,
									0,
									0,
									0);
						}
						
						NumSelectTimer_.start (QApplication::keyboardInputInterval ());
						connect (&NumSelectTimer_,
								SIGNAL (timeout ()),
								AllButtons_ [num],
								SLOT (animateClick ()));
						
						AllButtons_ [num]->setFocus ();
					}
					
					return;
				}
				
				for (auto butt : AllButtons_)
					if (butt->property ("OrigText").toString ()
							.startsWith (SearchText_, Qt::CaseInsensitive))
					{
						butt->setFocus ();
						break;
					}
			}
		};
	}

	void Plugin::handleShowList ()
	{
		auto rootWM = Proxy_->GetRootWindowsManager ();

		ICoreTabWidget *tw = rootWM->GetTabWidget (rootWM->GetPreferredWindowIndex ());

		if (tw->WidgetCount () < 2)
			return;

		QWidget *widget = new QWidget (nullptr,
				Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
		widget->setAttribute (Qt::WA_TranslucentBackground);
		widget->setWindowModality (Qt::ApplicationModal);

		QVBoxLayout *layout = new QVBoxLayout ();
		layout->setSpacing (1);
		layout->setContentsMargins (1, 1, 1, 1);

		const int currentIdx = tw->CurrentIndex ();
		QToolButton *toFocus = 0;
		QList<QToolButton*> allButtons;
		for (int i = 0, count = tw->WidgetCount (); i < count; ++i)
		{
			const QString& origText = tw->TabText (i);
			QString title = QString ("[%1] ").arg (i + 1) + origText;
			if (title.size () > 100)
				title = title.left (100) + "...";
			QAction *action = new QAction (tw->TabIcon (i),
					title, this);
			action->setProperty ("TabIndex", i);
			action->setProperty ("ICTW", QVariant::fromValue<ICoreTabWidget*> (tw));
			connect (action,
					SIGNAL (triggered ()),
					this,
					SLOT (navigateToTab ()));
			connect (action,
					SIGNAL (triggered ()),
					widget,
					SLOT (deleteLater ()));

			auto button = new QToolButton ();
			button->setDefaultAction (action);
			button->setToolButtonStyle (Qt::ToolButtonTextBesideIcon);
			button->setSizePolicy (QSizePolicy::Expanding,
					button->sizePolicy ().verticalPolicy ());
			button->setProperty ("OrigText", origText);
			layout->addWidget (button);

			if (currentIdx == i)
				toFocus = button;

			allButtons << button;
		}

		widget->installEventFilter (new ListEventFilter (allButtons, widget));
		widget->setLayout (layout);
		layout->update ();
		layout->activate ();

		const QRect& rect = QApplication::desktop ()->
				screenGeometry (rootWM->GetPreferredWindow ());
		QPoint pos = rect.center ();

		const QSize& size = widget->sizeHint () / 2;
		pos -= QPoint (size.width (), size.height ());

		widget->move (pos);
		widget->show ();

		if (toFocus)
			toFocus->setFocus ();
	}

	void Plugin::navigateToTab ()
	{
		const int idx = sender ()->property ("TabIndex").toInt ();
		auto ictw = sender ()->property ("ICTW").value<ICoreTabWidget*> ();
		ictw->setCurrentTab (idx);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_tabslist, LeechCraft::TabsList::Plugin);
