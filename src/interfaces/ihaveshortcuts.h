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

#ifndef INTERFACES_IHAVESHORTCUTS_H
#define INTERFACES_IHAVESHORTCUTS_H
#include <QtPlugin>
#include <QMultiMap>
#include <QString>
#include <QKeySequence>
#include <QIcon>
#include <QMetaType>

class QAction;

typedef QList<QKeySequence> QKeySequences_t;
Q_DECLARE_METATYPE (QKeySequences_t);

namespace LeechCraft
{
	/** @brief Describes an action exposed in shortcut manager.
	 *
	 * This structure contains information about the action that could be
	 * handled by the shortcut manager, like action icon, default key
	 * sequences and human-readable description text.
	 */
	struct ActionInfo
	{
		/// User-visible name of the action.
		QString UserVisibleText_;
		/// List of key sequences for this action.
		QKeySequences_t Seqs_;
		/// Icon of the action.
		QIcon Icon_;

		/** @brief Default-constructs an action info.
		 */
		ActionInfo ()
		{
		}

		/** @brief Constructs an action info.
		 *
		 * Constructs an info object for the given user-visible text
		 * \em uvt, default key sequence \em seq and action icon
		 * \em icon.
		 */
		ActionInfo (const QString& uvt,
				const QKeySequence& seq,
				const QIcon& icon)
		: UserVisibleText_ (uvt)
		, Icon_ (icon)
		{
			Seqs_ << seq;
		}

		/** @brief Constructs an action info.
		 *
		 * Constructs an info object for the given user-visible text
		 * \em uvt, default key sequence list \em seqs and action icon
		 * \em icon.
		 */
		ActionInfo (const QString& uvt,
				const QKeySequences_t& seqs,
				const QIcon& icon)
		: UserVisibleText_ (uvt)
		, Seqs_ (seqs)
		, Icon_ (icon)
		{
		}
	};
};

Q_DECLARE_METATYPE (LeechCraft::ActionInfo);

/** @brief Proxy for requesting shortcuts from the shortcut manager in
 * the Core.
 *
 * The plugin can communicate with the shortcut manager via this proxy.
 *
 * @sa IHaveShortcuts::SetShortcutProxy().
 */
class Q_DECL_EXPORT IShortcutProxy
{
public:
	/** @brief Checks whether a given object has been registered already.
	 *
	 * @return Returns whether the \em object has been already registered.
	 */
	virtual bool HasObject (QObject *object) const = 0;

	/** @brief Returns a QKeySequence for the given action.
	 *
	 * Returns a list of key sequences for the action with given id for
	 * the given object which is currently set in the shortcut manager.
	 * The id is the same as in return value of
	 * IHaveShortcuts::GetActionInfo().
	 *
	 * The object is used to distinguish between ids of different
	 * plugins. It can be said that object defines the context for the
	 * id.
	 *
	 * @param[in] object The object that should be checked.
	 * @param[in] id ID of the action.
	 * @return The key sequences for the passed action.
	 */
	virtual QKeySequences_t GetShortcuts (QObject *object, const QString& id) = 0;

	virtual ~IShortcutProxy () { }
};

/** @brief Interface for plugins that support configuring shortcuts.
 *
 * LeechCraft::Util::ShortcutManager class can help in forming the
 * GetActionInfo() map and in keeping track of created actions.
 *
 * @sa LeechCraft::Util::ShortcutManager
 */
class Q_DECL_EXPORT IHaveShortcuts
{
public:
	/** @brief Sets shortcut's list of key sequences if it has been changed.
	 *
	 * The id is the same as in the return value of GetActionInfo().
	 *
	 * @param[in] id The id of the action.
	 * @param[in] sequences The new key sequences.
	 */
	virtual void SetShortcut (const QString& id, const QKeySequences_t& sequences) = 0;

	/** @brief Returns information about all the shortcuts.
	 *
	 * Returns a QMap from action id to the ActionInfo. Action id would
	 * be further used in SetShortcut and IShortcutProxy::GetShortcut(),
	 * for example.
	 *
	 * @return Shortcut IDs mapped to the corresponding ActionInfo.
	 */
	virtual QMap<QString, LeechCraft::ActionInfo> GetActionInfo () const = 0;

	virtual ~IHaveShortcuts () { }
};

Q_DECLARE_INTERFACE (IShortcutProxy, "org.Deviant.LeechCraft.IShortcutProxy/1.0");
Q_DECLARE_INTERFACE (IHaveShortcuts, "org.Deviant.LeechCraft.IHaveShortcuts/1.0");

#endif

