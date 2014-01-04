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

#pragma once

#include <QWidget>
#include <QHash>
#include <interfaces/ihavetabs.h>
#include <interfaces/structures.h>
#include "ui_editorpage.h"

class QMenu;

namespace LeechCraft
{
namespace Popishu
{
	class EditorPage : public QWidget
						, public ITabWidget
	{
		Q_OBJECT
		Q_INTERFACES (ITabWidget)

		Ui::EditorPage Ui_;

		static QObject* S_MultiTabsParent_;

		std::unique_ptr<QToolBar> Toolbar_;
		QMenu *DoctypeMenu_;
		QMenu *RecentFilesMenu_;
		QString Filename_;
		bool Modified_;
		QMap<QString, QList<QAction*>> WindowMenus_;
		QHash<QString, QString> Extension2Lang_;

		bool DoctypeDetected_;

		QtMsgHandler DefaultMsgHandler_;
		QObject *WrappedObject_;
		bool TemporaryDocument_;
	public:
		static void SetParentMultiTabs (QObject*);

		EditorPage (QWidget* = 0);
		virtual ~EditorPage ();

		void Remove ();
		QToolBar* GetToolBar () const;
		QObject* ParentMultiTabs ();
		QList<QAction*> GetTabBarContextMenuActions () const;
		QMap<QString, QList<QAction*>> GetWindowMenus () const;
		TabClassInfo GetTabClassInfo () const;

		void SetText (const QString&);
		void SetLanguage (const QString&);

		void SetTemporaryDocument (bool);
		QsciScintilla* GetTextEditor () const;
	private slots:
		void selectDoctype (QAction*);
		void on_ActionNew__triggered ();
		void on_ActionOpen__triggered ();
		void on_ActionSave__triggered ();
		void on_ActionSaveAs__triggered ();
		void on_ActionWSInvisible__triggered ();
		void on_ActionWSVisible__triggered ();
		void on_ActionWSVisibleAfterIndent__triggered ();
		void on_ActionShowLineNumbers__toggled (bool);
		void on_ActionEnableFolding__toggled (bool);
		void on_ActionWrapNone__triggered ();
		void on_ActionWrapWords__triggered ();
		void on_ActionWrapCharacters__triggered ();
		void on_ActionReplace__triggered ();
		void on_TextEditor__textChanged ();
		void on_Inject__released ();
		void on_Release__released ();

		void handleMonoFontChanged ();
		void handleVisualWrapFlags ();
		void handleOtherPrefs ();

		void checkInterpreters (QString language);
		void checkProperDoctypeAction (const QString& language);

		void handleRecentFileOpen ();
	private:
		void SetWhitespaceVisibility (QsciScintilla::WhitespaceVisibility);
		bool Save ();
		void Open (const QString&);
		QsciLexer* GetLexerByLanguage (const QString&) const;
		QString GetLanguage (const QString& filename) const;
		QString FixLanguage (const QString&) const;
		void ShowConsole (bool);
		void GroupActions (const QList<QAction*>&);
		void RestoreRecentFiles ();
		void PrependRecentFile (const QString&, bool = true);
	signals:
		void removeTab (QWidget*);
		void changeTabName (QWidget*, const QString&);
		void changeTabIcon (QWidget*, const QIcon&);
		void changeTooltip (QWidget*, QWidget*);
		void statusBarChanged (QWidget*, const QString&);
		void couldHandle (const LeechCraft::Entity&, bool*);
		void delegateEntity (const LeechCraft::Entity&,
				int*, QObject**);
		void gotEntity (const LeechCraft::Entity&);

		void languageChanged (const QString& language);
	};
}
}
