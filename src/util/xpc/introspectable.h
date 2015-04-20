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

#include <functional>
#include <QHash>
#include <QMetaType>
#include <QVariantMap>
#include <util/sll/oldcppkludges.h>
#include "xpcconfig.h"

namespace LeechCraft
{
namespace Util
{
	using Introspect_f = std::function<QVariantMap (QVariant)>;

	class UTIL_XPC_API Introspectable
	{
		QHash<int, Introspect_f> Intros_;

		Introspectable () = default;
	public:
		Introspectable (const Introspectable&) = delete;
		Introspectable& operator= (const Introspectable&) = delete;

		static Introspectable& Instance ();

		template<typename T, typename U>
		void Register (const U& intro, decltype (Invoke (intro, std::declval<QVariant> ()))* = nullptr)
		{
			const auto id = qMetaTypeId<T> ();
			Intros_ [id] = intro;
		}

		template<typename T, typename U>
		void Register (const U& intro, decltype (Invoke (intro, std::declval<T> ()))* = nullptr)
		{
			Register<T> ([intro] (const QVariant& var) { return Invoke (intro, var.value<T> ()); });
		}

		template<typename T>
		QVariantMap operator() (const T& t) const
		{
			return (*this) (QVariant::fromValue<T> (t));
		}

		QVariantMap operator() (const QVariant&) const;
	};
}
}