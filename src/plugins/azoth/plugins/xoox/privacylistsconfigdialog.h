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

#ifndef PLUGINS_AZOTH_PLUGINS_XOOX_PRIVACYLISTSCONFIGDIALOG_H
#define PLUGINS_AZOTH_PLUGINS_XOOX_PRIVACYLISTSCONFIGDIALOG_H
#include <QDialog>
#include "ui_privacylistsconfigdialog.h"
#include "privacylistsmanager.h"

class QStandardItemModel;
class QStandardItem;

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	class PrivacyListsManager;

	class PrivacyListsConfigDialog : public QDialog
	{
		Q_OBJECT

		Ui::PrivacyListsConfigDialog Ui_;
		PrivacyListsManager *Manager_;
		QMap<QString, PrivacyList> Lists_;
		
		QStandardItemModel *Model_;
	public:
		PrivacyListsConfigDialog (PrivacyListsManager*, QWidget* = 0);
	private:
		void QueryLists ();
		void QueryList (const QString&);
		void AddListToBoxes (const QString&);
		void ReinitModel ();
		QList<QStandardItem*> ToRow (const PrivacyListItem&) const;
	public slots:
		void accept ();
		void reject ();
	private slots:
		void on_ConfigureList__activated (int);
		void on_AddButton__released ();
		void on_RemoveButton__released ();
		void on_DefaultPolicy__currentIndexChanged (int);
		void on_AddRule__released ();
		void on_ModifyRule__released ();
		void on_RemoveRule__released ();
		void on_MoveUp__released ();
		void on_MoveDown__released ();
		void handleGotLists (const QStringList&, const QString&, const QString&);
		void handleGotList (const PrivacyList&);
	};
}
}
}

#endif
