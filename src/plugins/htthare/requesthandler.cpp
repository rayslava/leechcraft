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

#include "requesthandler.h"
#include <QList>
#include <QString>
#include <QtDebug>
#include "connection.h"

namespace LeechCraft
{
namespace HttThare
{
	RequestHandler::RequestHandler (const Connection_ptr& conn)
	: Conn_ (conn)
	{
	}

	void RequestHandler::operator() (QByteArray data)
	{
		data.replace ("\r", "");

		auto lines = data.split ('\n');
		for (auto& line : lines)
			line = line.trimmed ();
		lines.removeAll ({});

		if (lines.size () < 0)
			return ErrorResponse (400, "Bad Request");

		const auto& req = lines.takeAt (0).split (' ');
		if (req.size () < 2)
			return ErrorResponse (400, "Bad Request");

		const auto& verb = req.at (0).toLower ();
		Url_ = QUrl::fromEncoded (req.at (1));

		for (const auto& line : lines)
		{
			const auto colonPos = line.indexOf (':');
			if (colonPos <= 0)
				return ErrorResponse (400, "Bad Request");
			Headers_ [line.left (colonPos)] = line.mid (colonPos + 1).trimmed ();
		}

		if (verb == "head")
			HandleHead ();
		else if (verb == "get")
			HandleGet ();
		else
			return ErrorResponse (405, "Method Not Allowed",
					"Method " + verb + " not supported by this server.");
	}

	void RequestHandler::ErrorResponse (int code,
			const QByteArray& reason, const QByteArray& full)
	{
		ResponseLine_ = "HTTP/1.1 " + QByteArray::number (code) + " " + reason + "\r\n";

		ResponseBody_ = QString (R"delim(<html>
				<head><title>%1 %2</title></head>
				<body>
					<h1>%1 %2</h1>
					%3
				</body>
			</html>
			)delim")
				.arg (code)
				.arg (reason.data ())
				.arg (full.data ()).toUtf8 ();

		ResponseHeaders_ = "Content-Length: " +
				QByteArray::number (ResponseBody_.size ()) + "\r\n\r\n";

		auto c = Conn_;
		boost::asio::async_write (c->GetSocket (),
				ToBuffers (),
				c->GetStrand ().wrap ([c] (const boost::system::error_code& ec, ulong)
					{
						if (ec)
							qWarning () << Q_FUNC_INFO
									<< ec.message ().c_str ();

						boost::system::error_code iec;
						c->GetSocket ().shutdown (boost::asio::socket_base::shutdown_both, iec);
					}));
	}

	void RequestHandler::HandleGet ()
	{
		ResponseLine_ = "HTTP/1.1 200 OK\r\n";
		ResponseBody_ = QString (R"delim(<html>
				<head><title>Test</title></head>
				<body>
					<h1>Test</h1>
				</body>
			</html>
			)delim").toUtf8 ();
		ResponseHeaders_ = "Content-Length: " +
				QByteArray::number (ResponseBody_.size ()) + "\r\n\r\n";

		auto c = Conn_;
		boost::asio::async_write (c->GetSocket (),
				ToBuffers (),
				c->GetStrand ().wrap ([c] (const boost::system::error_code& ec, ulong)
					{
						if (ec)
							qWarning () << Q_FUNC_INFO
									<< ec.message ().c_str ();

						boost::system::error_code iec;
						c->GetSocket ().shutdown (boost::asio::socket_base::shutdown_both, iec);
					}));
	}

	void RequestHandler::HandleHead ()
	{
	}

	namespace
	{
		boost::asio::const_buffer BA2Buffer (const QByteArray& ba)
		{
			return { ba.constData (), static_cast<size_t> (ba.size ()) };
		}
	}

	std::vector<boost::asio::const_buffer> RequestHandler::ToBuffers () const
	{
		std::vector<boost::asio::const_buffer> result;

		result.push_back (BA2Buffer (ResponseLine_));
		result.push_back (BA2Buffer (ResponseHeaders_));
		result.push_back (BA2Buffer (ResponseBody_));

		return result;
	}
}
}
