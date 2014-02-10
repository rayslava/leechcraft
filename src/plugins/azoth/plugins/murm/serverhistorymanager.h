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
#include <QHash>
#include <QModelIndex>
#include <interfaces/azoth/ihaveserverhistory.h>

class QNetworkReply;
class QAbstractItemModel;
class QStandardItemModel;

namespace LeechCraft
{
namespace Azoth
{
namespace Murm
{
	class VkAccount;

	class ServerHistoryManager : public QObject
	{
		Q_OBJECT

		VkAccount * const Acc_;
		QStandardItemModel * const ContactsModel_;

		int MsgCount_ = -1;
		int LastOffset_ = 0;

		bool IsRefreshing_ = false;

		struct RequestState
		{
			QModelIndex Index_;
			int Offset_;
		};
		QHash<QNetworkReply*, RequestState> MsgRequestState_;
	public:
		ServerHistoryManager (VkAccount*);

		QAbstractItemModel* GetModel () const;
		void RequestHistory (const QModelIndex&, int, int);
	private:
		void Request (int);
	public slots:
		void refresh ();
	private slots:
		void handleGotHistory ();
		void handleGotMessagesList ();
	signals:
		void serverHistoryFetched (const QModelIndex&, int, const SrvHistMessages_t&);
	};
}
}
}
