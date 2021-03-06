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

#include <memory>
#include <atomic>
#include <type_traits>
#include <functional>
#include <QThread>
#include <QFuture>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QFutureInterface>
#include <util/threads/futures.h>

namespace LeechCraft
{
namespace Liznoo
{
	namespace detail
	{
		class StartHandlersRotatorBase : public QObject
		{
			Q_OBJECT
		public:
			using QObject::QObject;
		public slots:
			virtual void rotate () = 0;
		};
	}

	template<typename ConnT>
	class DBusThread : public QThread
	{
		std::weak_ptr<ConnT> Conn_;

		std::atomic_bool IsConnInitialized_ { false };

		QMutex SHMutex_;
		QList<std::function<void (ConnT*)>> StartHandlers_;

		class Rotator : public detail::StartHandlersRotatorBase
		{
			DBusThread<ConnT> * const Thread_;
		public:
			Rotator (DBusThread<ConnT> *thread)
			: Thread_ { thread }
			{
			}

			void rotate () override
			{
				Thread_->RunHandlers ();
			}
		};

		std::unique_ptr<Rotator> Rotator_;
	public:
		using QThread::QThread;

		~DBusThread ()
		{
			if (!isRunning ())
				return;

			quit ();
			if (!wait (1000))
				terminate ();
		}

		template<typename F>
		QFuture<typename std::result_of<F (ConnT*)>::type> ScheduleOnStart (const F& f)
		{
			QFutureInterface<typename std::result_of<F (ConnT*)>::type> iface;
			iface.reportStarted ();

			auto handler = [f, iface] (ConnT *conn) mutable
			{
				Util::ReportFutureResult (iface, f, conn);
			};

			{
				QMutexLocker locker { &SHMutex_ };
				StartHandlers_ << handler;
			}

			if (IsConnInitialized_)
				QTimer::singleShot (0, Rotator_.get (), SLOT (rotate ()));

			return iface.future ();
		}

		std::shared_ptr<ConnT> GetConnector () const
		{
			return Conn_.lock ();
		}
	protected:
		void run () override
		{
			const auto conn = std::make_shared<ConnT> ();
			Conn_ = conn;

			Rotator_.reset (new Rotator { this });

			IsConnInitialized_ = true;
			RunHandlers ();

			QThread::run ();
		}
	private:
		void RunHandlers ()
		{
			decltype (StartHandlers_) handlers;

			{
				QMutexLocker locker { &SHMutex_ };
				handlers.swap (StartHandlers_);
			}

			const auto conn = GetConnector ().get ();
			for (const auto& f : handlers)
				f (conn);
		}
	};

	template<typename ConnT>
	using DBusThread_ptr = std::shared_ptr<DBusThread<ConnT>>;
}
}
