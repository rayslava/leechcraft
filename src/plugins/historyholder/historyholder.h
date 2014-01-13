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

#ifndef PLUGINS_HISTORYHOLDER_HISTORYHOLDER_H
#define PLUGINS_HISTORYHOLDER_HISTORYHOLDER_H
#include <QObject>
#include <QStringList>
#include <interfaces/iinfo.h>
#include <interfaces/ifinder.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/ihaveshortcuts.h>

class QModelIndex;

namespace LeechCraft
{
	namespace Plugins
	{
		namespace HistoryHolder
		{
			class Plugin : public QObject
						 , public IInfo
						 , public IFinder
						 , public IEntityHandler
						 , public IHaveShortcuts
			{
				Q_OBJECT
				Q_INTERFACES (IInfo IFinder IEntityHandler IHaveShortcuts)
			public:
				void Init (ICoreProxy_ptr);
				void SecondInit ();
				void Release ();
				QByteArray GetUniqueID () const;
				QString GetName () const;
				QString GetInfo () const;
				QIcon GetIcon () const;
				QStringList Provides () const;

				QStringList GetCategories () const;
				QList<IFindProxy_ptr> GetProxy (const LeechCraft::Request&);

				EntityTestHandleResult CouldHandle (const LeechCraft::Entity&) const;
				void Handle (LeechCraft::Entity);

				void SetShortcut (const QString&, const QKeySequences_t&);
				QMap<QString, ActionInfo> GetActionInfo () const;
			public slots:
				void handleTasksTreeActivated (const QModelIndex&);
			signals:
				void gotEntity (const LeechCraft::Entity&);
				void categoriesChanged (const QStringList&, const QStringList&);
			};
		};
	};
};

#endif

