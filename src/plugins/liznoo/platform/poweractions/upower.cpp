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

#include "upower.h"
#include <QtDBus/QtDBus>

namespace LeechCraft
{
namespace Liznoo
{
namespace PowerActions
{
	namespace
	{
		QByteArray State2Method (Platform::State state)
		{
			switch (state)
			{
			case Platform::State::Suspend:
				return "Suspend";
			case Platform::State::Hibernate:
				return "Hibernate";
			}

			qWarning () << Q_FUNC_INFO
					<< "unknown state"
					<< static_cast<int> (state);
			return {};
		}
	}

	QFuture<Platform::QueryChangeStateResult> UPower::CanChangeState (State state)
	{
		return QtConcurrent::run ([state] () -> QueryChangeStateResult
				{
					QDBusInterface face ("org.freedesktop.UPower",
							"/org/freedesktop/UPower",
							"org.freedesktop.UPower",
							QDBusConnection::systemBus ());
					if (!face.isValid ())
						return { false, tr ("Cannot connect to UPower daemon.") };

					return { face.property ("Can" + State2Method (state)).toBool (), {} };
				});
	}

	void UPower::ChangeState (State state)
	{
		QDBusInterface face ("org.freedesktop.UPower",
				"/org/freedesktop/UPower",
				"org.freedesktop.UPower",
				QDBusConnection::systemBus ());

		face.call (QDBus::NoBlock, State2Method (state));
	}
}
}
}
