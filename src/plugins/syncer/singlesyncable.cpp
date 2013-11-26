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

#include "singlesyncable.h"
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <QTimer>
#include <QTcpSocket>
#include <QSettings>
#include <QCoreApplication>
#include <QtDebug>
#include <laretz/item.h>
#include <laretz/operation.h>
#include <laretz/packetparser.h>
#include <laretz/packetgenerator.h>
#include <interfaces/isyncable.h>

namespace LeechCraft
{
namespace Syncer
{
	SingleSyncable::SingleSyncable (const QByteArray& id, ISyncProxy *proxy, QObject *parent)
	: QObject (parent)
	, ID_ (id)
	, Proxy_ (proxy)
	, Socket_ (new QTcpSocket (this))
	{
		connect (Socket_,
				SIGNAL (connected ()),
				this,
				SLOT (handleSocketConnected ()));
		connect (Socket_,
				SIGNAL (readyRead ()),
				this,
				SLOT (handleSocketRead ()));

		QTimer::singleShot (1000,
				this,
				SLOT (startSync ()));
	}

	std::shared_ptr<QSettings> SingleSyncable::GetSettings ()
	{
		std::shared_ptr<QSettings> settings (new QSettings (QCoreApplication::organizationName (),
					QCoreApplication::applicationName () + "_Syncer_State"),
				[] (QSettings *settings) -> void
				{
					settings->endGroup ();
					delete settings;
				});
		settings->beginGroup (ID_);
		return settings;
	}

	void SingleSyncable::HandleList (const Laretz::ParseResult& reply)
	{
		auto deletedList = std::find_if (reply.operations.begin (), reply.operations.end (),
				[] (const Laretz::Operation& op) { return op.getType () == Laretz::OpType::Delete; });
		auto knownList = std::find_if (reply.operations.begin (), reply.operations.end (),
				[] (const Laretz::Operation& op) { return op.getType () == Laretz::OpType::List; });

		if (deletedList != reply.operations.end ())
		{
			QList<Laretz::Operation> ourOps;
			Proxy_->Merge (ourOps, { *deletedList });
		}

		if (knownList != reply.operations.end () &&
				!knownList->getItems ().empty ())
		{
			const auto& str = Laretz::PacketGenerator {}
					({ Laretz::OpType::Fetch, { knownList->getItems () } })
					({ "Login", "d34df00d" })
					({ "Password", "shitfuck" })
					();

			Socket_->write (str.c_str (), str.size ());

			State_ = State::FetchRequested;
		}
		else
			HandleFetch ({});
	}

	namespace
	{
		template<typename T>
		QList<T> ToQList (const std::vector<T>& vector)
		{
			QList<T> result;
			result.reserve (vector.size ());
			std::copy (vector.begin (), vector.end (), std::back_inserter (result));
			return result;
		}

		template<typename T>
		std::vector<T> ToStdVector (const QList<T>& list)
		{
			std::vector<T> result;
			result.reserve (list.size ());
			std::copy (list.begin (), list.end (), std::back_inserter (result));
			return result;
		}
	}

	void SingleSyncable::HandleFetch (const Laretz::ParseResult& reply)
	{
		auto ourOps = Proxy_->GetNewOps ();
		Proxy_->Merge (ourOps, ToQList (reply.operations));

		if (ourOps.empty ())
		{
			State_ = State::Idle;
			return;
		}

		for (auto& op : ourOps)
			for (auto& item : op.getItems ())
				if (item.getParentId ().empty ())
					item.setParentId (ID_.constData ());

		State_ = State::Sent;

		const auto& str = Laretz::PacketGenerator {}
				[ToStdVector (ourOps)]
				({ "Login", "d34df00d" })
				({ "Password", "shitfuck" })
				();
		Socket_->write (str.c_str (), str.size ());
	}

	void SingleSyncable::HandleRootCreated (const Laretz::ParseResult&)
	{
		const auto lastSeq = GetSettings ()->value ("LastSyncID", 0).value<uint64_t> ();

		Laretz::Item parentItem;
		parentItem.setSeq (lastSeq);
		parentItem.setParentId (ID_.constData ());

		const auto& str = Laretz::PacketGenerator {}
				({ Laretz::OpType::List, { parentItem } })
				({ "Login", "d34df00d" })
				({ "Password", "shitfuck" })
				();

		Socket_->write (str.c_str (), str.size ());

		State_ = State::ListRequested;
	}

	void SingleSyncable::HandleSendResult (const Laretz::ParseResult& reply)
	{
	}

	void SingleSyncable::CreateRoot ()
	{
		Laretz::Item parentItem;
		parentItem.setSeq (0);
		parentItem.setId (ID_.constData ());

		const auto& str = Laretz::PacketGenerator {}
				({ Laretz::OpType::Append, { parentItem } })
				({ "Login", "d34df00d" })
				({ "Password", "shitfuck" })
				();

		Socket_->write (str.c_str (), str.size ());

		State_ = State::RootCreateRequested;
	}

	void SingleSyncable::handleSocketRead ()
	{
		const auto& data = Socket_->readAll ();
		const auto& reply = Laretz::Parse ({ data.constData (), static_cast<size_t> (data.size ()) });

		qDebug () << Q_FUNC_INFO << "results:" << reply.operations.size ();
		for (auto p : reply.fields)
			qDebug () << p.first.c_str () << "->" << p.second.c_str ();

		if (reply.fields.at ("Status") != "Success")
		{
			const auto reasonPos = reply.fields.find ("ErrorCode");
			const auto reason = reasonPos != reply.fields.end () ?
					boost::lexical_cast<int> (reasonPos->second) :
					-1;

			auto defErr = [this, reason]
			{
				qWarning () << Q_FUNC_INFO
						<< "unknown error"
						<< reason
						<< "in state"
						<< static_cast<int> (State_);
			};

			switch (State_)
			{
			case State::ListRequested:
			{
				switch (reason)
				{
				case Laretz::ErrorCode::UnknownParent:
					CreateRoot ();
					break;
				default:
					defErr ();
					break;
				}
				break;
			}
			default:
				defErr ();
				break;
			}
			return;
		}

		switch (State_)
		{
		case State::Idle:
			qWarning () << Q_FUNC_INFO
					<< "unexpected packet in Idle state";
			break;
		case State::ListRequested:
			HandleList (reply);
			break;
		case State::FetchRequested:
			HandleFetch (reply);
			break;
		case State::Sent:
			HandleSendResult (reply);
			break;
		case State::RootCreateRequested:
			HandleRootCreated (reply);
			break;
		}
	}

	void SingleSyncable::startSync ()
	{
		if (Socket_->isValid ())
			return;

		Socket_->connectToHost ("127.0.0.1", 54093);
	}

	void SingleSyncable::handleSocketConnected ()
	{
		qDebug () << Q_FUNC_INFO;

		HandleRootCreated ({});
	}
}
}
