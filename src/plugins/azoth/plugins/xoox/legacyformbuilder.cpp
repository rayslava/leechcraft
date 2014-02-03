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

#include "legacyformbuilder.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QVariant>
#include <QLineEdit>
#include <QtDebug>

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	namespace
	{
		void LineEditActorImpl (QWidget *form, const QXmppElement& elem,
				const QString& fieldLabel)
		{
			QLabel *label = new QLabel (fieldLabel);
			QLineEdit *edit = new QLineEdit (elem.value ());
			edit->setObjectName ("field");
			edit->setProperty ("FieldName", elem.tagName ());

			QHBoxLayout *lay = new QHBoxLayout (form);
			lay->addWidget (label);
			lay->addWidget (edit);
			qobject_cast<QVBoxLayout*> (form->layout ())->addLayout (lay);
		}

		void InstructionsActor (QWidget *form, const QXmppElement& elem)
		{
			QLabel *label = new QLabel (elem.value ());
			form->layout ()->addWidget (label);
		}
	}

	LegacyFormBuilder::LegacyFormBuilder ()
	: Widget_ (0)
	{
		Tag2Actor_ ["username"] = [this] (QWidget *f, const QXmppElement& e)
			{ LineEditActorImpl (f, e, tr ("Username:")); };
		Tag2Actor_ ["password"] = [this] (QWidget *f, const QXmppElement& e)
			{ LineEditActorImpl (f, e, tr ("Password:")); };
		Tag2Actor_ ["registered"] = [this] (QWidget*, const QXmppElement&) {};
		Tag2Actor_ ["first"] = [this] (QWidget *f, const QXmppElement& e)
			{ LineEditActorImpl (f, e, tr ("First name:")); };
		Tag2Actor_ ["last"] = [this] (QWidget *f, const QXmppElement& e)
			{ LineEditActorImpl (f, e, tr ("Last name:")); };
		Tag2Actor_ ["nick"] = [this] (QWidget *f, const QXmppElement& e)
			{ LineEditActorImpl (f, e, tr ("Nick:")); };
		Tag2Actor_ ["email"] = [this] (QWidget *f, const QXmppElement& e)
			{ LineEditActorImpl (f, e, tr ("E-Mail:")); };
		Tag2Actor_ ["instructions"] = [this] (QWidget *f, const QXmppElement& e)
			{ InstructionsActor (f, e); };
	}

	void LegacyFormBuilder::Clear ()
	{
		delete Widget_;
		Widget_ = nullptr;
	}

	QWidget* LegacyFormBuilder::CreateForm (const QXmppElement& containing,
			QWidget *parent)
	{
		Widget_ = new QWidget (parent);
		Widget_->setLayout (new QVBoxLayout ());

		QXmppElement element = containing.firstChildElement ();
		while (!element.isNull ())
		{
			const QString& tag = element.tagName ();

			if (!Tag2Actor_.contains (tag))
				qWarning () << Q_FUNC_INFO
						<< "unknown tag";
			else
				Tag2Actor_ [tag] (Widget_, element);

			element = element.nextSiblingElement ();
		}

		return Widget_;
	}

	QList<QXmppElement> LegacyFormBuilder::GetFilledChildren () const
	{
		QList<QXmppElement> result;
		if (!Widget_)
			return result;

		Q_FOREACH (QLineEdit *edit, Widget_->findChildren<QLineEdit*> ("field"))
		{
			QXmppElement elem;
			elem.setTagName (edit->property ("FieldName").toString ());
			elem.setValue (edit->text ());
			result << elem;
		}

		return result;
	}

	QString LegacyFormBuilder::GetUsername () const
	{
		if (!Widget_)
			return QString ();

		Q_FOREACH (QLineEdit *edit, Widget_->findChildren<QLineEdit*> ("field"))
			if (edit->property ("FieldName").toString () == "username")
				return edit->text ();

		return QString ();
	}

	QString LegacyFormBuilder::GetPassword () const
	{
		Q_FOREACH (QLineEdit *edit, Widget_->findChildren<QLineEdit*> ("field"))
			if (edit->property ("FieldName").toString () == "password")
				return edit->text ();

		return QString ();
	}
}
}
}
