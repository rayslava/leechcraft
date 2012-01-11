/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2012  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#ifndef PLUGINS_POSHUKU_URLFRAME_H
#define PLUGINS_POSHUKU_URLFRAME_H
#include <QFrame>
#include "ui_urlframe.h"

namespace LeechCraft
{
namespace Poshuku
{
	class URLFrame : public QFrame
	{
		Q_OBJECT

		Ui::URLFrame Ui_;
	public:
		URLFrame (QWidget* = 0);

		QLineEdit* GetEdit () const;
		void SetFavicon (const QIcon&);
		void AddWidget (QWidget*);
		void RemoveWidget (QWidget*);
	private slots:
		void on_URLEdit__returnPressed ();
	signals:
		void load (const QString&);
	};
}
}

#endif
