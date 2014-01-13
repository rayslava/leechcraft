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

#include "accountregfirstpage.h"
#include <ProtocolInfo>

namespace LeechCraft
{
namespace Azoth
{
namespace Astrality
{
	AccountRegFirstPage::AccountRegFirstPage (const Tp::ProtocolInfo& info, bool regNew, QWidget *parent)
	: QWidget (parent)
	{
		Ui_.setupUi (this);

		auto visRow = [&info] (QString parm, QWidget *w1, QWidget *w2)
		{
			if (!info.hasParameter (parm))
			{
				w1->hide ();
				if (w2)
					w2->hide ();
			}
		};
		visRow ("account", Ui_.AccIDLabel_, Ui_.AccID_);
		visRow ("server", Ui_.ServerLabel_, Ui_.Server_);
		visRow ("port", Ui_.PortLabel_, Ui_.Port_);
		visRow ("require-encryption", Ui_.RequireEncryption_, 0);

		if (!regNew)
		{
			Ui_.PasswordLabel_->hide ();
			Ui_.Password_->hide ();
		}
	}

	void AccountRegFirstPage::SetParams (const QVariantMap& params)
	{
		auto setStr = [&params] (QLineEdit *edit, const QString& name)
		{
			edit->setText (params [name].toString ());
		};
		setStr (Ui_.AccID_, "account");
		setStr (Ui_.Server_, "server");

		Ui_.Port_->setValue (params ["port"].toInt ());

		Ui_.RequireEncryption_->setCheckState (params ["require-encryption"].toBool () ? Qt::Checked : Qt::Unchecked);
	}

	QString AccountRegFirstPage::GetAccountID () const
	{
		return Ui_.AccID_->text ();
	}

	QString AccountRegFirstPage::GetPassword () const
	{
		return Ui_.Password_->text ();
	}

	QString AccountRegFirstPage::GetServer () const
	{
		return Ui_.Server_->text ();
	}

	int AccountRegFirstPage::GetPort () const
	{
		return Ui_.Port_->value ();
	}

	bool AccountRegFirstPage::ShouldRequireEncryption () const
	{
		return Ui_.RequireEncryption_->isChecked ();
	}

	void AccountRegFirstPage::SetSettings (const AccountWrapper::Settings& s)
	{
		Ui_.Autodisconnect_->setCheckState (s.Autodisconnect_ ? Qt::Checked : Qt::Unchecked);
	}

	void AccountRegFirstPage::Augment (AccountWrapper::Settings& s) const
	{
		s.Autodisconnect_ = Ui_.Autodisconnect_->checkState () == Qt::Checked;
	}
}
}
}
