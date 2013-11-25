/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include <memory>
#include <boost/asio/buffer.hpp>
#include <QByteArray>
#include <QUrl>
#include <QMap>
#include <QCoreApplication>

class QFileInfo;

namespace LeechCraft
{
namespace HttHare
{
	class Connection;
	typedef std::shared_ptr<Connection> Connection_ptr;

	class RequestHandler
	{
		Q_DECLARE_TR_FUNCTIONS (LeechCraft::HttHare::RequestHandler)

		const Connection_ptr Conn_;

		QUrl Url_;
		QMap<QString, QString> Headers_;

		QByteArray ResponseLine_;
		QList<QPair<QByteArray, QByteArray>> ResponseHeaders_;
		QByteArray CookedRH_;
		QByteArray ResponseBody_;

		enum class Verb
		{
			Get,
			Head
		};
	public:
		RequestHandler (const Connection_ptr&);

		void operator() (QByteArray);
	private:
		QString Tr (const char*);

		void ErrorResponse (int, const QByteArray&, const QByteArray& = QByteArray ());
		QByteArray MakeDirResponse (const QFileInfo&, const QString&, const QUrl&);

		void HandleRequest (Verb);
		void WriteDir (const QString&, const QFileInfo&, Verb);
		void WriteFile (const QString&, const QFileInfo&, Verb);
		void DefaultWrite (Verb);
		std::vector<boost::asio::const_buffer> ToBuffers (Verb);
	};
}
}
