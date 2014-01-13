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

#ifndef INTERFACES_IHAVESETTINGS_H
#define INTERFACES_IHAVESETTINGS_H
#include <memory>
#include <QtPlugin>

namespace LeechCraft
{
	namespace Util
	{
		class XmlSettingsDialog;
		typedef std::shared_ptr<XmlSettingsDialog> XmlSettingsDialog_ptr;
	}
}

/** @brief Interface for plugins that have user-configurable settings.
 *
 * Plugins that have user-configurable settings should implement this
 * interface if they want to appear in a common settings configuration
 * dialog.
 */
class Q_DECL_EXPORT IHaveSettings
{
public:
	/** @brief Gets the settings dialog manager object from the plugin.
	 *
	 * The returned XmlSettingsDialog would be integrated into common
	 * settings dialog where user can configure all the plugins that
	 * provide this interface.
	 *
	 * @return The XmlSettingsDialog object that manages the settings
	 * dialog of the plugin.
	 */
	virtual LeechCraft::Util::XmlSettingsDialog_ptr GetSettingsDialog () const = 0;

	/** @brief Virtual destructor.
	 */
	virtual ~IHaveSettings () {}
};

Q_DECLARE_INTERFACE (IHaveSettings, "org.Deviant.LeechCraft.IHaveSettings/1.0");

#endif

