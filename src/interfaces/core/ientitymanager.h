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

namespace LeechCraft
{
	struct Entity;
}

/** @brief Proxy to core entity manager.
 *
 * Core entity manager is that very thing that routes entities between
 * different plugins and chooses which plugins will handle what entity.
 *
 * This class can be used instead more or less deprecated gotEntity()
 * and delegateEntity() signals of IInfo.
 *
 * @sa Entity, IInfo
 */
class Q_DECL_EXPORT IEntityManager
{
public:
	/** The result of delegating this entity to another plugin.
	 */
	struct DelegationResult
	{
		/** @brief The plugin instance object that handles this entity.
		 *
		 * If no object handles the entity, this is a nullptr.
		 */
		QObject *Handler_ = nullptr;

		/** The internal ID of the delegated entity local to the handling
		 * plugin.
		 */
		int ID_ = 0;

		DelegationResult () = default;

		DelegationResult (QObject *handler, int id)
		: Handler_ { handler }
		, ID_ { id }
		{
		}
	};

	virtual ~IEntityManager () {}

	/** @brief Delegates the given entity and returns the delegation result.
	 *
	 * Queries all plugins if they can handle the given entity, and
	 * chooses one of them. If the desired object is set, this method
	 * first tries to handle the entity with it. Returns a structure
	 * describing the delegation result.
	 *
	 * @param[in] entity The entity to handle.
	 * @param[in] desired The object to try first.
	 *
	 * @sa DelegationResult
	 */
	virtual DelegationResult DelegateEntity (LeechCraft::Entity entity, QObject *desired = 0) = 0;

	/** @brief Handles the given entity.
	 *
	 * Queries all plugins if they can handle the given entity, and
	 * chooses one of them (or all of them, according to entity flags and
	 * plugins' behavior). If the desired object is set, this method
	 * first tries to handle the entity with it.
	 *
	 * @param[in] entity The entity to handle.
	 * @param[in] desired The object to try first.
	 *
	 * @return If the entity has been handled successfully.
	 */
	virtual bool HandleEntity (LeechCraft::Entity entity, QObject *desired = 0) = 0;

	/** @brief Queries whether the given entity can be handled at all.
	 *
	 * @param[in] entity The entity to test.
	 *
	 * @return Whether there is at least one plugin to handle this entity.
	 */
	virtual bool CouldHandle (const LeechCraft::Entity& entity) = 0;

	/** @brief Queries what plugins can handle the given entity.
	 *
	 * @param[in] entity The entity to test.
	 *
	 * @return The list of plugin instances that can handle the given entity.
	 */
	virtual QList<QObject*> GetPossibleHandlers (const LeechCraft::Entity& entity) = 0;
};

Q_DECLARE_INTERFACE (IEntityManager, "org.Deviant.LeechCraft.IEntityManager/1.0");
