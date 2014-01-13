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

#ifndef PLUGINS_AGGREGATOR_EXPORT2FB2DIALOG_H
#define PLUGINS_AGGREGATOR_EXPORT2FB2DIALOG_H
#include <QDialog>
#include "ui_export2fb2dialog.h"

namespace LeechCraft
{
struct Entity;

namespace Util
{
	class CategorySelector;
};

namespace Aggregator
{
	struct WriteInfo;

	class Export2FB2Dialog : public QDialog
	{
		Q_OBJECT

		Ui::Export2FB2Dialog Ui_;
		Util::CategorySelector *Selector_;
		QStringList CurrentCategories_;

		bool HasBeenTextModified_;
	public:
		Export2FB2Dialog (QWidget* = 0);
	private:
		void WriteFB2 (const WriteInfo&);
		void WritePDF (const WriteInfo&);
	private slots:
		void on_Browse__released ();
		void on_File__textChanged (const QString&);
		void on_Name__textEdited ();
		void handleChannelsSelectionChanged (const QItemSelection&, const QItemSelection&);
		void handleAccepted ();
	signals:
		void gotEntity (const LeechCraft::Entity&);
	};
}
}

#endif
