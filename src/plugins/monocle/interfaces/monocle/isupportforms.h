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

#include <memory>
#include <QtPlugin>

namespace LeechCraft
{
namespace Monocle
{
	class IFormField;

	typedef std::shared_ptr<IFormField> IFormField_ptr;
	typedef QList<IFormField_ptr> IFormFields_t;

	/** @brief Interface for documents supporting inline forms.
	 * 
	 * If a document is of format that supports page forms that can be
	 * filled, it should implement this interface.
	 * 
	 * It also makes sense to implement ISaveableDocument so that changes
	 * to the forms could be saved.
	 * 
	 * There is no "Apply" method in either this interface or form field
	 * interfaces. Changes should be applied as soon as corresponding
	 * interface's setter method is called.
	 * 
	 * @sa ISaveableDocument, IFormField
	 */
	class ISupportForms
	{
	public:
		virtual ~ISupportForms () {}

		/** @brief Returns the list of fields for the given page.
		 * 
		 * This function should return the list of form fields found on
		 * the given \em page, or an empty list if no fields are present.
		 * 
		 * @param[in] page The zero-based index of the page to query.
		 * @return The list of form fields on the page.
		 */
		virtual IFormFields_t GetFormFields (int page) = 0;
	};
}
}

Q_DECLARE_INTERFACE (LeechCraft::Monocle::ISupportForms,
		"org.LeechCraft.Monocle.ISupportForms/1.0");
