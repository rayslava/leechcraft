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

#ifndef PLUGINS_AZOTH_PLUGINS_XOOX_CORE_H
#define PLUGINS_AZOTH_PLUGINS_XOOX_CORE_H
#include <QObject>
#include <interfaces/structures.h>
#include <interfaces/core/icoreproxy.h>

namespace LeechCraft
{
namespace Azoth
{
class IProxyObject;

namespace Xoox
{
	class GlooxProtocol;
	class GlooxCLEntry;
	class CapsDatabase;
	class AvatarsStorage;

	class Core : public QObject
	{
		Q_OBJECT

		ICoreProxy_ptr Proxy_;
		std::shared_ptr<GlooxProtocol> GlooxProtocol_;
		QObject *PluginProxy_;
		bool SaveRosterScheduled_;

		CapsDatabase *CapsDB_;
		AvatarsStorage *Avatars_;

		Core ();
	public:
		static Core& Instance ();

		void SecondInit ();
		void Release ();
		QList<QObject*> GetProtocols () const;

		void SetPluginProxy (QObject*);
		IProxyObject* GetPluginProxy () const;
		void SetProxy (ICoreProxy_ptr);
		ICoreProxy_ptr GetProxy () const;

		CapsDatabase* GetCapsDatabase () const;
		AvatarsStorage* GetAvatarsStorage () const;

		void SendEntity (const Entity&);

		void ScheduleSaveRoster (int = 2000);
	private:
		void LoadRoster ();
	public slots:
		void saveRoster ();
	private slots:
		void saveAvatarFor (GlooxCLEntry* = 0);
		void handleItemsAdded (const QList<QObject*>&);
	signals:
		void gotEntity (const LeechCraft::Entity&);
		void delegateEntity (const LeechCraft::Entity&, int*, QObject**);
	};
}
}
}

#endif
