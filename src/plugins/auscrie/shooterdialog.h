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

#ifndef PLUGINS_AUSCRIE_SHOOTERDIALOG_H
#define PLUGINS_AUSCRIE_SHOOTERDIALOG_H
#include <QDialog>
#include <interfaces/core/icoreproxy.h>
#include "ui_shooterdialog.h"

namespace LeechCraft
{
namespace Auscrie
{
	class ShooterDialog : public QDialog
	{
		Q_OBJECT

		Ui::ShooterDialog Ui_;
		const ICoreProxy_ptr Proxy_;
		QPixmap CurrentScreenshot_;
	public:
		struct FilterData
		{
			QObject *Object_;
			QByteArray Variant_;
		};
	private:
		QList<FilterData> Filters_;
	public:
		enum class Action
		{
			Upload,
			Save
		};

		enum class Mode
		{
			LCWindowOverlay,
			LCWindow,
			CurrentScreen,
			WholeDesktop
		};

		ShooterDialog (ICoreProxy_ptr, QWidget* = 0);

		Action GetAction () const;

		Mode GetMode () const;
		bool ShouldHide () const;

		int GetTimeout () const;
		QString GetFormat () const;
		int GetQuality () const;

		FilterData GetDFInfo () const;

		void SetScreenshot (const QPixmap&);
		QPixmap GetScreenshot () const;

		void resizeEvent (QResizeEvent*);
	private:
		void RescaleLabel ();
	private slots:
		void on_Format__currentIndexChanged (const QString&);
	signals:
		void screenshotRequested ();
	};
}
}

#endif
