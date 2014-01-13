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

#ifndef PLUGINS_AZOTH_PLUGINS_XOOX_MEDIACALL_H
#define PLUGINS_AZOTH_PLUGINS_XOOX_MEDIACALL_H
#include <QObject>
#include <QXmppCallManager.h>
#include <interfaces/azoth/imediacall.h>

#ifndef ENABLE_MEDIACALLS
#error Dont include this if media calls are disabled.
#endif

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	class GlooxAccount;

	class MediaCall : public QObject
					, public IMediaCall
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Azoth::IMediaCall)

		QXmppCall *Call_;
		GlooxAccount *Account_;
	public:
		MediaCall (GlooxAccount*, QXmppCall*);

		Direction GetDirection () const;
		QString GetSourceID () const;
		void Accept ();
		void Hangup ();
		QIODevice* GetAudioDevice ();
		QAudioFormat GetAudioFormat ();
		QIODevice* GetVideoDevice ();
	private slots:
		void handleStateChanged (QXmppCall::State);
	signals:
		void stateChanged (LeechCraft::Azoth::IMediaCall::State);
		void audioModeChanged (QIODevice::OpenMode);
	};
}
}
}

#endif
