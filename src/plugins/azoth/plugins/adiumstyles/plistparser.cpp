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

#include "plistparser.h"
#include <QFile>
#include <QDomDocument>
#include <QtDebug>

namespace LeechCraft
{
namespace Azoth
{
namespace AdiumStyles
{
	PListParseError::PListParseError (const QString& str)
	: runtime_error (str.toUtf8 ().constData ())
	, Str_ (str)
	{
	}

	PListParseError::~PListParseError () throw ()
	{
	}

	QString PListParseError::GetStr() const
	{
		return Str_;
	}

	void PListParser::Parse (const QString& filename)
	{
		QFile file (filename);
		if (!file.open (QIODevice::ReadOnly))
			throw PListParseError (QObject::tr ("Unable to open file: %2.")
					.arg (file.errorString ()));

		QDomDocument doc;

		QString error;
		int line = 0;
		int col = 0;
		if (!doc.setContent (&file, &error, &line, &col))
			throw PListParseError (QObject::tr ("Parse error: %1 (%2:%3).")
					.arg (error)
					.arg (line)
					.arg (col));

		const auto& dict = doc.documentElement ().firstChildElement ("dict");
		auto elem = dict.firstChildElement ();
		QString currentKey;
		while (!elem.isNull ())
		{
			if (elem.tagName ().toLower () == "key")
				currentKey = elem.text ();
			else if (currentKey.isEmpty ())
				throw PListParseError (QObject::tr ("State mismatch: expected key tag."));
			else
			{
				KeyVals_ [currentKey] = elem.text ();
				currentKey.clear ();
			}

			elem = elem.nextSiblingElement ();
		}
	}

	QString PListParser::operator[] (const QString& key) const
	{
		return KeyVals_ [key];
	}
}
}
}
