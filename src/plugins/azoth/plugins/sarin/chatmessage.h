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

#include <QObject>
#include <interfaces/azoth/imessage.h>
#include <interfaces/azoth/iadvancedmessage.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Sarin
{
	class ToxContact;

	class ChatMessage : public QObject
					  , public IMessage
					  , public IAdvancedMessage
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Azoth::IMessage
				LeechCraft::Azoth::IAdvancedMessage)

		ToxContact * const Contact_;
		const Direction Dir_;

		QString Body_;
		QDateTime TS_ = QDateTime::currentDateTime ();

		bool IsDelivered_ = false;
	public:
		ChatMessage (const QString&, Direction, ToxContact*);

		QObject* GetQObject () override;
		void Send () override;
		void Store () override;

		Direction GetDirection () const override;
		Type GetMessageType () const override;
		SubType GetMessageSubType () const override;
		QObject* OtherPart () const override;
		QString GetOtherVariant () const override;

		QString GetBody () const override;
		void SetBody (const QString& body) override;
		QDateTime GetDateTime () const override;
		void SetDateTime (const QDateTime& timestamp) override;

		bool IsDelivered () const override;

		void SetDelivered ();
	signals:
		void messageDelivered () override;
	};
}
}
}