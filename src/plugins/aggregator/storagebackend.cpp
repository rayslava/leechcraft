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

#include "storagebackend.h"
#include <stdexcept>
#include <QFile>
#include <QDebug>
#include "sqlstoragebackend.h"
#include "sqlstoragebackend_mysql.h"

namespace LeechCraft
{
namespace Aggregator
{
	QString StorageBackend::LoadQuery (const QString& engine, const QString& name)
	{
		QFile file (QString (":/resources/sql/%1/%2.sql")
				.arg (engine)
				.arg (name));
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open file"
					<< name
					<< "for engine"
					<< engine
					<< "for reading";
			return QString ();
		}
		return file.readAll ();
	}

	StorageBackend::StorageBackend (QObject *parent)
	: QObject (parent)
	{
	}

	StorageBackend::~StorageBackend ()
	{
	}

	std::shared_ptr<StorageBackend> StorageBackend::Create (const QString& strType, const QString& id)
	{
		StorageBackend::Type type;
		if (strType == "SQLite")
			type = StorageBackend::SBSQLite;
		else if (strType == "PostgreSQL")
			type = StorageBackend::SBPostgres;
		else if (strType == "MySQL")
			type = StorageBackend::SBMysql;
		else
			throw std::runtime_error (qPrintable (QString ("Unknown storage type %1")
						.arg (strType)));

		return Create (type, id);
	}

	std::shared_ptr<StorageBackend> StorageBackend::Create (Type type, const QString& id)
	{
		std::shared_ptr<StorageBackend> result;
		switch (type)
		{
			case SBSQLite:
			case SBPostgres:
				result.reset (new SQLStorageBackend (type, id));
				break;
			case SBMysql:
				result.reset (new SQLStorageBackendMysql (type, id));
		}
		return result;
	}
}
}
