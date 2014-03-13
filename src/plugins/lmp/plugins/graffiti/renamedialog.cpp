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

#include "renamedialog.h"
#include <QStandardItemModel>
#include <QMessageBox>
#include <QDir>
#include <QtDebug>

namespace LeechCraft
{
namespace LMP
{
namespace Graffiti
{
	RenameDialog::RenameDialog (ILMPProxy_Ptr proxy, QWidget *parent)
	: QDialog (parent)
	, Proxy_ (proxy)
	, Getters_ (proxy->GetSubstGetters ())
	, PreviewModel_ (new QStandardItemModel (this))
	{
		Ui_.setupUi (this);

		const auto& helpText = tr ("The following variables are allowed in the pattern: %1.")
				.arg (QStringList (Getters_.keys ()).join ("; "));
		Ui_.PatternDescLabel_->setText (helpText);

		Ui_.Preview_->setModel (PreviewModel_);

		connect (Ui_.Pattern_,
				SIGNAL (editTextChanged (QString)),
				this,
				SLOT (updatePreview ()));
	}

	void RenameDialog::SetInfos (const QList<MediaInfo>& infos)
	{
		Infos_.clear ();
		for (const auto& info : infos)
			Infos_.push_back ({ info, QFileInfo (info.LocalPath_).fileName () });

		PreviewModel_->clear ();
		PreviewModel_->setHorizontalHeaderLabels ({ tr ("Source name"), tr ("Target name") });

		for (const auto& info : Infos_)
		{
			auto sourceItem = new QStandardItem;
			sourceItem->setText (info.second);
			PreviewModel_->appendRow ({ sourceItem, new QStandardItem });
		}

		updatePreview ();
	}

	QList<QPair<QString, QString>> RenameDialog::GetRenames () const
	{
		QList<QPair<QString, QString>> result;
		for (const auto& info : Infos_)
			if (QFileInfo (info.first.LocalPath_).fileName () != info.second)
				result.push_back ({ info.first.LocalPath_, info.second });
		return result;
	}

	void RenameDialog::Rename (const QList<QPair<QString, QString>>& pairs)
	{
		for (const auto& pair : pairs)
		{
			const QFileInfo sourceInfo (pair.first);
			auto sourceDir = sourceInfo.absoluteDir ();
			if (!sourceDir.rename (sourceInfo.fileName (), pair.second))
				qWarning () << Q_FUNC_INFO
						<< "failed to rename"
						<< sourceInfo.fileName ()
						<< "to"
						<< pair.second;
		}
	}

	void RenameDialog::accept ()
	{
		std::shared_ptr<void> guard (nullptr, [this] (void*) { QDialog::accept (); });

		const auto& toRename = GetRenames ();
		if (toRename.isEmpty ())
			return;

		if (QMessageBox::question (this,
				"LMP Graffiti",
				tr ("Are you sure you want to rename %n file(s)?", 0, toRename.size ()),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			Rename (toRename);
	}

	void RenameDialog::updatePreview ()
	{
		const auto& pattern = Ui_.Pattern_->currentText ();
		const bool hasExtension = pattern.contains ('.');

		int row = 0;
		for (auto& info : Infos_)
		{
			info.second = Proxy_->PerformSubstitutions (pattern,
					info.first, SubstitutionFlag::SFSafeFilesystem);
			if (!hasExtension)
				info.second += '.' + QFileInfo (info.first.LocalPath_).suffix ();

			auto item = PreviewModel_->item (row++, 1);
			item->setText (info.second);
		}
	}
}
}
}
