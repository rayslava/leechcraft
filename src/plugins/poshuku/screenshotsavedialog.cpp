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

#include "screenshotsavedialog.h"
#include <algorithm>
#include <QImageWriter>
#include <QBuffer>
#include <QTimer>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QtDebug>
#include <util/util.h>
#include <util/xpc/util.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/idatafilter.h>
#include <interfaces/ientityhandler.h>
#include "core.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Poshuku
{
	ScreenShotSaveDialog::ScreenShotSaveDialog (const QPixmap& source,
			QWidget *parent)
	: QDialog (parent)
	, Source_ (source)
	, PixmapHolder_ (new QLabel ())
	, RenderScheduled_ (false)
	{
		PixmapHolder_->setAlignment (Qt::AlignTop | Qt::AlignLeft);
		Ui_.setupUi (this);

		QList<QByteArray> formats = QImageWriter::supportedImageFormats ();
		formats.removeAll ("ico");
		if (formats.contains ("jpg"))
			formats.removeAll ("jpeg");
		std::sort (formats.begin (), formats.end ());
		for (QList<QByteArray>::const_iterator i = formats.begin (),
				end = formats.end (); i != end; ++i)
			Ui_.FormatCombobox_->addItem (i->toUpper ());
		if (formats.contains ("png"))
			Ui_.FormatCombobox_->setCurrentIndex (formats.indexOf ("png"));

		Ui_.Scroller_->setWidget (PixmapHolder_);

		auto proxy = Core::Instance ().GetProxy ();
		const auto& dfs = Util::GetDataFilters (QVariant::fromValue (Source_.toImage ()),
				proxy->GetEntityManager ());
		for (auto df : dfs)
		{
			auto idf = qobject_cast<IDataFilter*> (df);
			for (const auto& var : idf->GetFilterVariants ())
			{
				Ui_.ActionBox_->addItem (var.Icon_, var.Name_);
				Filters_.append ({ df, var.ID_ });
			}
		}

		Ui_.ActionBox_->addItem (proxy->GetIcon ("document-save"), tr ("Save"));
	}

	void ScreenShotSaveDialog::accept ()
	{
		const auto idx = Ui_.ActionBox_->currentIndex ();
		if (idx >= Filters_.size ())
			Save ();
		else
		{
			const auto& filter = Filters_.value (idx);
			auto e = Util::MakeEntity (QVariant::fromValue (Source_.toImage ()),
					{},
					{},
					"x-leechcraft/data-filter-request");

			e.Additional_ ["Format"] = Ui_.FormatCombobox_->currentText ();
			e.Additional_ ["Quality"] = Ui_.QualitySlider_->value ();

			e.Additional_ ["FilterVariant"] = filter.ID_;

			auto ieh = qobject_cast<IEntityHandler*> (filter.Object_);
			ieh->Handle (e);
		}

		QDialog::accept ();
	}

	void ScreenShotSaveDialog::Save ()
	{
		const auto defLoc = QDesktopServices::storageLocation (QDesktopServices::DocumentsLocation);
		const auto& filename = QFileDialog::getSaveFileName (this,
				tr ("Save screenshot"),
				XmlSettingsManager::Instance ()->Property ("ScreenshotsLocation",
					defLoc).toString ());
		if (filename.isEmpty ())
			return;

		XmlSettingsManager::Instance ()->setProperty ("ScreenshotsLocation", filename);

		QFile file (filename);
		if (!file.open (QIODevice::WriteOnly | QIODevice::Truncate))
		{
			QMessageBox::critical (this,
					"LeechCraft",
					tr ("Could not open %1 for write")
						.arg (filename));
			return;
		}

		const auto& format = Ui_.FormatCombobox_->currentText ();
		int quality = Ui_.QualitySlider_->value ();

		if (!Source_.save (&file, qPrintable (format), quality))
			QMessageBox::critical (this,
					"LeechCraft",
					tr ("Could not write screenshot to %1")
						.arg (filename));
	}

	void ScreenShotSaveDialog::ScheduleRender ()
	{
		if (RenderScheduled_)
			return;

		QTimer::singleShot (500,
				this,
				SLOT (render ()));

		RenderScheduled_ = true;
	}

	void ScreenShotSaveDialog::render ()
	{
		RenderScheduled_ = false;
		if (!Ui_.PreviewBox_->isChecked ())
		{
			Ui_.FileSizeLabel_->setText (tr ("File size unknown"));
			PixmapHolder_->setPixmap (QPixmap ());
			PixmapHolder_->resize (1, 1);
			return;
		}

		QString format = Ui_.FormatCombobox_->currentText ();
		int quality = Ui_.QualitySlider_->value ();

		QBuffer renderBuffer;
		Source_.save (&renderBuffer, qPrintable (format), quality);
		QByteArray renderData = renderBuffer.data ();
		Rendered_.loadFromData (renderData);
		Ui_.FileSizeLabel_->setText (Util::MakePrettySize (renderData.size ()));
		PixmapHolder_->setPixmap (Rendered_);
		PixmapHolder_->resize (Rendered_.size ());
	}

	void ScreenShotSaveDialog::on_QualitySlider__valueChanged ()
	{
		ScheduleRender ();
	}

	void ScreenShotSaveDialog::on_FormatCombobox__currentIndexChanged ()
	{
		ScheduleRender ();
	}
}
}
