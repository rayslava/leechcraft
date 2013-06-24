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

#include <QtPlugin>
#include <QList>

class QString;
class QIcon;
class QWidget;

namespace LeechCraft
{
namespace Blasq
{
	class IAccount;

	class IService
	{
	public:
		virtual ~IService () {}

		virtual QObject* GetQObject () = 0;

		virtual QString GetServiceName () const = 0;

		virtual QIcon GetServiceIcon () const = 0;

		virtual QList<IAccount*> GetRegisteredAccounts () const = 0;

		virtual QList<QWidget*> GetAccountRegistrationWidgets () const = 0;

		virtual void RegisterAccount (const QString& name, const QList<QWidget*>& widgets) = 0;

		virtual void RemoveAccount (IAccount*) = 0;
	protected:
		virtual void accountAdded (QObject*) = 0;

		virtual void accountRemoved (QObject*) = 0;
	};
}
}

Q_DECLARE_INTERFACE (LeechCraft::Blasq::IService, "org.LeechCraft.Blasq.IService/1.0");
