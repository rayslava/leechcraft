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

#include <thread>
#include <QThread>
#include <QFuture>
#include <util/sll/either.h>
#include <util/sll/typelist.h>
#include <util/sll/typelevel.h>
#include <util/threads/futures.h>
#include "accountthreadfwd.h"

namespace LeechCraft
{
namespace Snails
{
	class Account;
	class AccountThreadWorker;
	class TaskQueueManager;

	struct Task
	{
		std::function<void (AccountThreadWorker*)> Executor_;
	};

	class GenericExceptionWrapper
	{
		std::string Msg_;
	public:
		GenericExceptionWrapper (const std::exception_ptr&);

		const char* what () const noexcept;
	};

	namespace detail
	{
		const auto MaxRecLevel = 3;

		template<typename Result, typename Ex,
				typename = std::enable_if_t<Util::HasType<std::decay_t<Ex>> (Util::AsTypelist_t<typename Result::L_t> {})>>
		Result ReturnException (const Ex& ex, int)
		{
			return Result::Left (ex);
		}

		template<typename Result, typename Ex>
		Result ReturnException (const Ex&, float)
		{
			return Result::Left (std::current_exception ());
		}

		template<typename Result, typename F, typename Ex = std::exception>
		Result HandleExceptions (F&& f, int recLevel = 0, const Ex& ex = {})
		{
			if (recLevel)
			{
				const auto timeout = recLevel * 3000 + 1000;
				qWarning () << Q_FUNC_INFO
						<< "sleeping for"
						<< timeout
						<< "and retrying for the"
						<< recLevel
						<< "time after getting an exception:"
						<< ex.what ();

				std::this_thread::sleep_for (std::chrono::milliseconds { timeout });
			}

			if (recLevel == MaxRecLevel)
			{
				qWarning () << Q_FUNC_INFO
						<< "giving up after"
						<< recLevel
						<< "retries:"
						<< ex.what ();

				return ReturnException<Result> (ex, 0);
			}

			try
			{
				return f ();
			}
			catch (const vmime::exceptions::authentication_error& err)
			{
				const auto& respStr = QString::fromUtf8 (err.response ().c_str ());

				qWarning () << Q_FUNC_INFO
						<< "caught auth error:"
						<< respStr;

				return Result::Left (err);
			}
			catch (const vmime::exceptions::connection_error& e)
			{
				return HandleExceptions<Result> (f, ++recLevel, e);
			}
			catch (const vmime::exceptions::invalid_response& e)
			{
				return HandleExceptions<Result> (f, ++recLevel, e);
			}
			catch (const vmime::exceptions::operation_timed_out& e)
			{
				return HandleExceptions<Result> (f, ++recLevel, e);
			}
			catch (const vmime::exceptions::not_connected& e)
			{
				return HandleExceptions<Result> (f, ++recLevel, e);
			}
			catch (const vmime::exceptions::socket_exception& e)
			{
				return HandleExceptions<Result> (f, ++recLevel, e);
			}
			catch (const std::exception&)
			{
				return Result::Left (std::current_exception ());
			}
		}

		template<typename Right>
		struct WrapFunctionTypeImpl
		{
			using Result_t = Util::Either<InvokeError_t<>, Right>;

			template<typename F>
			static auto WrapFunction (const F& f)
			{
				return [f] (auto... args)
				{
					return HandleExceptions<Result_t> ([&]
							{
								return Result_t::Right (Util::Invoke (f, args...));
							});
				};
			}
		};

		template<>
		struct WrapFunctionTypeImpl<void>
		{
			using Result_t = Util::Either<InvokeError_t<>, boost::none_t>;

			template<typename F>
			static auto WrapFunction (const F& f)
			{
				return [f] (auto... args)
				{
					return HandleExceptions<Result_t> ([&]
							{
								Util::Invoke (f, args...);
								return Result_t::Right ({});
							});
				};
			}
		};

		template<typename T>
		using IsVoid_t = std::is_same<T, boost::detail::variant::void_>;

		template<typename... Lefts, typename Right>
		struct WrapFunctionTypeImpl<Util::Either<boost::variant<Lefts...>, Right>>
		{
			using LeftTypes_t = Util::Filter_t<Util::Not<IsVoid_t>::Result_t, Util::Typelist<Lefts...>>;

			template<typename>
			struct BuildErrorList;

			template<typename... Types>
			struct BuildErrorList<Util::Typelist<Types...>>
			{
				using Result_t = InvokeError_t<Types...>;
			};

			using Result_t = Util::Either<typename BuildErrorList<LeftTypes_t>::Result_t, Right>;

			template<typename F>
			static auto WrapFunction (const F& f)
			{
				return [f] (auto... args)
				{
					return HandleExceptions<Result_t> ([&] { return Util::Invoke (f, args...); });
				};
			}
		};

		template<typename... Args, typename F>
		auto WrapFunction (const F& f)
		{
			return WrapFunctionTypeImpl<Util::ResultOf_t<F (AccountThreadWorker*, Args...)>>::WrapFunction (f);
		}
	}

	template<typename T>
	using WrapReturnType_t = typename detail::WrapFunctionTypeImpl<T>::Result_t;

	template<typename F, typename... Args>
	using WrapFunctionType_t = WrapReturnType_t<Util::ResultOf_t<F (AccountThreadWorker*, Args...)>>;

	class AccountThread : public QThread
	{
		Q_OBJECT

		Account * const A_;
		const bool IsListening_;
		const QString Name_;
		const CertList_t Certs_;

		QMutex FunctionsMutex_;
		QList<Task> Functions_;
	public:
		AccountThread (bool isListening, const QString& name,
				const CertList_t& certs, Account *acc);

		template<typename F, typename... Args>
		QFuture<WrapFunctionType_t<F, Args...>> Schedule (const F& func, const Args&... args)
		{
			QFutureInterface<WrapFunctionType_t<F, Args...>> iface;
			return Schedule (iface, func, args...);
		}

		template<typename F, typename... Args>
		QFuture<WrapFunctionType_t<F, Args...>> Schedule (QFutureInterface<WrapFunctionType_t<F, Args...>> iface, const F& func, const Args&... args)
		{
			auto reporting = [func, iface, args...] (AccountThreadWorker *w) mutable
			{
				iface.reportStarted ();
				Util::ReportFutureResult (iface, detail::WrapFunction<Args...> (func), w, args...);
			};

			{
				QMutexLocker locker { &FunctionsMutex_ };
				Functions_ << Task { reporting };
			}

			emit rotateFuncs ();

			return iface.future ();
		}
	protected:
		void run ();
	private:
		void RotateFuncs (AccountThreadWorker*);
		void ConnectSignals (AccountThreadWorker*);
	signals:
		void rotateFuncs ();
	};
}
}
