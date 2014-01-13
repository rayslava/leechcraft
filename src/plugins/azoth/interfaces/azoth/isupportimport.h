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

#ifndef PLUGINS_AZOTH_INTERFACES_ISUPPORTIMPORT_H
#define PLUGINS_AZOTH_INTERFACES_ISUPPORTIMPORT_H
#include <QMetaType>
#include <QVariantMap>

namespace LeechCraft
{
namespace Azoth
{
	/** @brief Interface for protocols supporting import.
	 *
	 * This interface should be implemented by the protocols supporting
	 * import of accounts or IM history.
	 *
	 * @sa IProtocol
	 */
	class ISupportImport
	{
	public:
		~ISupportImport () {}

		/** @brief Returns the "import-style" ID of the protocol.
		 *
		 * The following should be used, if possible:
		 * - xmpp for XMPP.
		 * - irc for IRC.
		 * - icq for ICQ.
		 *
		 * @return The "import-style" ID of the protocol.
		 */
		virtual QString GetImportProtocolID () const = 0;

		/** @brief Should try to import the account from data.
		 *
		 * The following keys are used globally, where possible:
		 * - "Name" string for account name.
		 * - "Jid" string for bare account ID (like JID in XMPP or UIN
		 *   in ICQ).
		 * - "Host" string for custom connection host, or empty.
		 * - "Port" int for custom connection port, or 0.
		 * - "Nick" string for user's nickname.
		 *
		 * @param[in] data The variant map with account settings.
		 * @return True if import was successful, false otherwise.
		 */
		virtual bool ImportAccount (const QVariantMap& data) = 0;

		/** @brief Returns the unique entry ID for given entry and account.
		 *
		 * This function is used to retrieve the unique entry ID from
		 * the given human-readable ID (hrID) and account object.
		 *
		 * The entry identified by human-readable ID may be absent in
		 * the account's contact list.
		 *
		 * @param[in] hrID The human-readable ID of the entry.
		 * @param[in] account The account object where the entry should
		 * belong.
		 * @return The would-be unique entry ID.
		 */
		virtual QString GetEntryID (const QString& hrID, QObject *account) = 0;
	};
}
}

Q_DECLARE_INTERFACE (LeechCraft::Azoth::ISupportImport,
		"org.Deviant.LeechCraft.Azoth.ISupportImport/1.0");

#endif
