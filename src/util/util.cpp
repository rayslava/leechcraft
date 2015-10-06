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

#include "util.h"
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <QString>
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QFile>
#include <QDir>
#include <QTime>
#include <QSettings>
#include <QTextCodec>
#include <QUrl>
#include <QAction>
#include <QBuffer>
#include <QPainter>
#include <QAction>
#include <QtDebug>

#if QT_VERSION < 0x050500
Q_DECLARE_METATYPE (QList<QModelIndex>);
#endif

QString LeechCraft::Util::GetAsBase64Src (const QImage& pix)
{
	QBuffer buf;
	buf.open (QIODevice::ReadWrite);
	pix.save (&buf, "PNG", 100);
	return QString ("data:image/png;base64,%1")
			.arg (QString (buf.buffer ().toBase64 ()));
}

QString LeechCraft::Util::GetUserText (const Entity& p)
{
	QString string = QObject::tr ("Too long to show");
	if (p.Additional_.contains ("UserVisibleName") &&
			p.Additional_ ["UserVisibleName"].canConvert<QString> ())
		string = p.Additional_ ["UserVisibleName"].toString ();
	else if (p.Entity_.canConvert<QByteArray> ())
	{
		QByteArray entity = p.Entity_.toByteArray ();
		if (entity.size () < 100)
			string = QTextCodec::codecForName ("UTF-8")->toUnicode (entity);
	}
	else if (p.Entity_.canConvert<QUrl> ())
	{
		string = p.Entity_.toUrl ().toString ();
		if (string.size () > 100)
			string = string.left (97) + "...";
	}
	else
		string = QObject::tr ("Binary entity");

	if (!p.Mime_.isEmpty ())
		string += QObject::tr ("<br /><br />of type <code>%1</code>").arg (p.Mime_);

	if (!p.Additional_ ["SourceURL"].toUrl ().isEmpty ())
	{
		QString urlStr = p.Additional_ ["SourceURL"].toUrl ().toString ();
		if (urlStr.size () > 63)
			urlStr = urlStr.left (60) + "...";
		string += QObject::tr ("<br />from %1")
			.arg (urlStr);
	}

	return string;
}

QString LeechCraft::Util::MakePrettySize (qint64 sourcesize)
{
	int strNum = 0;
	long double size = sourcesize;

	for (; strNum < 3 && size >= 1024; ++strNum, size /= 1024)
		;

	static QStringList units
	{
		QObject::tr (" b"),
		QObject::tr (" KiB"),
		QObject::tr (" MiB"),
		QObject::tr (" GiB")
	};

	return QString::number (size, 'f', 1) + units.value (strNum);
}

QString LeechCraft::Util::MakeTimeFromLong (ulong time)
{
	int d = time / 86400;
	time -= d * 86400;
	QString result;
	if (d)
		result += QObject::tr ("%n day(s), ", "", d);
	result += QTime (0, 0, 0).addSecs (time).toString ();
	return result;
}

QTranslator* LeechCraft::Util::LoadTranslator (const QString& baseName,
		const QString& localeName,
		const QString& prefix,
		const QString& appName)
{
	auto filename = prefix;
	filename.append ("_");
	if (!baseName.isEmpty ())
		filename.append (baseName).append ("_");
	filename.append (localeName);

	auto transl = new QTranslator;
#ifdef Q_OS_WIN32
	if (transl->load (filename, ":/") ||
			transl->load (filename,
					QCoreApplication::applicationDirPath () + "/translations"))
#elif defined (Q_OS_MAC)
	const auto tryLocal = QApplication::arguments ().contains ("-nobundle");
	if (transl->load (filename, ":/") ||
			(tryLocal &&
				transl->load (filename,
						QString ("/usr/local/share/%1/translations").arg (appName))) ||
			transl->load (filename,
					QCoreApplication::applicationDirPath () + "/../Resources/translations"))
#elif defined (INSTALL_PREFIX)
	if (transl->load (filename, ":/") ||
			transl->load (filename,
					QString (INSTALL_PREFIX "/share/%1/translations").arg (appName)))
#else
	if (transl->load (filename, ":/") ||
			transl->load (filename,
					QString ("/usr/local/share/%1/translations").arg (appName)) ||
			transl->load (filename,
					QString ("/usr/share/%1/translations").arg (appName)))
#endif
		return transl;

	delete transl;

	return nullptr;
}

QTranslator* LeechCraft::Util::InstallTranslator (const QString& baseName,
		const QString& prefix,
		const QString& appName)
{
	const auto& localeName = GetLocaleName ();
	if (auto transl = LoadTranslator (baseName, localeName, prefix, appName))
	{
		qApp->installTranslator (transl);
		return transl;
	}

	qWarning () << Q_FUNC_INFO
			<< "could not load translation file for locale"
			<< localeName
			<< baseName
			<< prefix
			<< appName;
	return nullptr;
}

QString LeechCraft::Util::GetLocaleName ()
{
	QSettings settings (QCoreApplication::organizationName (),
			QCoreApplication::applicationName ());
	QString localeName = settings.value ("Language", "system").toString ();

	if (localeName == "system")
	{
		localeName = QString (::getenv ("LANG")).left (5);

		if (localeName == "C" || localeName.isEmpty ())
			localeName = "en_US";

		if (localeName.isEmpty () || localeName.size () != 5)
			localeName = QLocale::system ().name ();
		localeName = localeName.left (5);
	}

	if (localeName.size () == 2)
	{
		auto lang = QLocale (localeName).language ();
		const auto& cs = QLocale::countriesForLanguage (lang);
		if (cs.isEmpty ())
			localeName += "_00";
		else
			localeName = QLocale (lang, cs.at (0)).name ();
	}

	return localeName;
}

QString LeechCraft::Util::GetInternetLocaleName (const QLocale& locale)
{
	if (locale.language () == QLocale::AnyLanguage)
		return "*";

	auto locStr = locale.name ();
	locStr.replace ('_', '-');
	return locStr;
}

QString LeechCraft::Util::GetLanguage ()
{
	return GetLocaleName ().left (2);
}

QModelIndexList LeechCraft::Util::GetSummarySelectedRows (QObject *sender)
{
	QAction *senderAct = qobject_cast<QAction*> (sender);
	if (!senderAct)
	{
		QString debugString;
		{
			QDebug d (&debugString);
			d << "sender is not a QAction*"
					<< sender;
		}
		throw std::runtime_error (qPrintable (debugString));
	}

	return senderAct->
			property ("SelectedRows").value<QList<QModelIndex>> ();
}

QAction* LeechCraft::Util::CreateSeparator (QObject *parent)
{
	QAction *result = new QAction (parent);
	result->setSeparator (true);
	return result;
}

QPixmap LeechCraft::Util::DrawOverlayText (QPixmap px,
		const QString& text, QFont font, const QPen& pen, const QBrush& brush)
{
	const auto& iconSize = px.size ();

	const auto fontHeight = px.height () * 0.45;
	font.setPixelSize (std::max (6., fontHeight));

	const QFontMetrics fm (font);
	const auto width = fm.width (text) + 2. * px.width () / 10.;
	const auto height = fm.height () + 2. * px.height () / 10.;
	const bool tooSmall = width > iconSize.width ();

	const QRect textRect (iconSize.width () - width, iconSize.height () - height, width, height);

	QPainter p (&px);
	p.setBrush (brush);
	p.setFont (font);
	p.setPen (pen);
	p.setRenderHint (QPainter::Antialiasing);
	p.setRenderHint (QPainter::TextAntialiasing);
	p.setRenderHint (QPainter::HighQualityAntialiasing);
	p.drawRoundedRect (textRect, 4, 4);
	p.drawText (textRect,
			Qt::AlignCenter,
			tooSmall ? "#" : text);
	p.end ();

	return px;
}
