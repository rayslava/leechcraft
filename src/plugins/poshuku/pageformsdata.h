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

#ifndef PLUGINS_POSHUKU_PLUGINS_POSHUKU_PAGEFORMSDATA_H
#define PLUGINS_POSHUKU_PLUGINS_POSHUKU_PAGEFORMSDATA_H
#include <QMap>
#include <QUrl>
#include <QString>
#include <QVariant>

class QDebug;

namespace LeechCraft
{
namespace Poshuku
{
	struct ElementData
	{
		QUrl PageURL_;
		QString FormID_;
		QString Name_;
		QString Type_;
		QString Value_;
	};

	bool operator== (const ElementData&, const ElementData&);
	bool operator< (const ElementData&, const ElementData&);

	QDataStream& operator<< (QDataStream&, const ElementData&);
	QDataStream& operator>> (QDataStream&, ElementData&);

	QDebug& operator<< (QDebug&, const ElementData&);

	typedef QList<ElementData> ElementsData_t;

	/** Holds information about all the forms on a page.
	 *
	 * The key of the map is the name of the `input' element, whereas
	 * value is the ElementData structure with the information about
	 * that element.
	 */
	typedef QMap<QString, ElementsData_t> PageFormsData_t;

	struct ElemFinder
	{
		const QString& ElemName_;
		const QString& ElemType_;

		ElemFinder (const QString& en, const QString& et)
		: ElemName_ (en)
		, ElemType_ (et)
		{
		}

		inline bool operator() (const ElementData& ed) const
		{
			return ed.Name_ == ElemName_ &&
					ed.Type_ == ElemType_;
		}
	};
}
}

Q_DECLARE_METATYPE (LeechCraft::Poshuku::ElementData);
Q_DECLARE_METATYPE (LeechCraft::Poshuku::ElementsData_t);
Q_DECLARE_METATYPE (LeechCraft::Poshuku::PageFormsData_t);

#endif
