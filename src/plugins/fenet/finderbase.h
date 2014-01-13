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

#pragma once

#include <QObject>
#include <QDir>
#include <QStandardItemModel>
#include <QtDebug>
#include <qjson/parser.h>
#include <util/sys/paths.h>

class QAbstractItemModel;
class QStandardItemModel;

namespace LeechCraft
{
namespace Fenet
{
	template<typename InfoT>
	class FinderBase : public QObject
	{
	protected:
		const QStringList Path_;

		QList<InfoT> Known_;
		QList<InfoT> Found_;

		QStandardItemModel * const FoundModel_;
	public:
		FinderBase (QObject *parent = 0)
		: QObject (parent)
		, Path_ (Util::GetSystemPaths ())
		, FoundModel_ (new QStandardItemModel (this))
		{
		}

		const QList<InfoT>& GetFound () const
		{
			return Found_;
		}

		QAbstractItemModel* GetFoundModel () const
		{
			return FoundModel_;
		}
	protected:
		void Find (const QString& subpath)
		{
			qDebug () << Q_FUNC_INFO << "searching for WMs...";

			const auto& cands = Util::GetPathCandidates (Util::SysPath::Share, "fenet/" + subpath);
			for (const auto& cand : cands)
				for (const auto& entry : QDir (cand).entryInfoList ({ "*.json" }))
					HandleDescr (entry.absoluteFilePath ());

			qDebug () << Known_.size () << "known WMs;"
					<< Found_.size () << "found WMs";
		}

		void HandleDescr (const QString& filePath)
		{
			QJson::Parser parser;

			QFile file (filePath);
			if (!file.open (QIODevice::ReadOnly))
			{
				qWarning () << Q_FUNC_INFO
						<< "cannot open file"
						<< file.fileName ()
						<< file.errorString ();
				return;
			}

			bool ok = false;
			const auto& varmap = parser.parse (&file, &ok).toMap ();
			if (!ok)
			{
				qWarning () << Q_FUNC_INFO
						<< "cannot parse file"
						<< file.fileName ();
				return;
			}

			QStringList execNames;
			for (const auto& var : varmap ["execNames"].toList ())
				execNames << var.toString ();

			const auto& info = GetInfo (filePath, execNames, varmap);
			Known_ << info;

			if (std::any_of (execNames.begin (), execNames.end (),
					[this] (const QString& name) { return this->IsAvailable (name); }))
			{
				qDebug () << Q_FUNC_INFO << info.Name_ << "available";
				Found_ << info;

				auto item = new QStandardItem (info.Name_);
				item->setEditable (false);
				item->setToolTip (info.Comment_);
				FoundModel_->appendRow (item);
			}
		}

		virtual InfoT GetInfo (const QString& filePath,
				const QStringList& execNames, const QVariantMap& varmap) const = 0;

		bool IsAvailable (const QString& executable) const
		{
			if (QFileInfo (executable).isExecutable ())
				return true;

			return !Util::FindInSystemPath (executable, Path_,
					[] (const QFileInfo& fi) { return fi.isExecutable (); }).isEmpty ();
		}
	};
}
}
