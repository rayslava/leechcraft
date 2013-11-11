/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2013  Oleg Linkin <MaledictusDeMagog@gmail.com>
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

#include "previewpage.h"
#include <QtDebug>
#include "reportwizard.h"
#include "reporttypepage.h"
#include "bugreportpage.h"
#include "featurerequestpage.h"
#include "xmlgenerator.h"
#include "fileattachpage.h"
#include "chooseuserpage.h"

namespace LeechCraft
{
namespace Dolozhee
{
	PreviewPage::PreviewPage (QWidget *parent)
	: QWizardPage (parent)
	{
		Ui_.setupUi (this);

		setButtonText (QWizard::NextButton, tr ("Send request"));
	}

	namespace
	{
		QString PriorityToString (ReportTypePage::Priority priority)
		{
			switch (priority)
			{
			case ReportTypePage::Priority::Low:
				return QObject::tr ("Low");
			case ReportTypePage::Priority::Normal:
				return QObject::tr ("Normal");
			case ReportTypePage::Priority::High:
				return QObject::tr ("High");
			}

			qWarning () << Q_FUNC_INFO
					<< "unknown priority"
					<< static_cast<int> (priority);
			return "unknown";
		}
	}

	void PreviewPage::initializePage ()
	{
		auto wiz = qobject_cast<ReportWizard*> (wizard ());
		if (!wiz)
			return;

		const auto typePage = wiz->GetReportTypePage ();
		const auto type = typePage->GetReportType ();

		QString title;
		QList<QPair<QString, QString>> sections;
		QString typeText;
		switch (type)
		{
		case ReportTypePage::Type::Bug:
			title = wiz->GetBugReportPage ()->GetTitle ();
			sections = wiz->GetBugReportPage ()->GetReportSections ();
			typeText = tr ("Bug");
			break;
		case ReportTypePage::Type::Feature:
			title = wiz->GetFRPage ()->GetTitle ();
			sections = wiz->GetFRPage ()->GetReportSections ();
			typeText = tr ("Feature");
			break;
		}

		QString preview = "<strong>User:</strong><br/>" +
				((wiz->GetChooseUserPage ()->GetUser () == ChooseUserPage::User::Anonymous) ?
					"Anonymous" :
					wiz->GetChooseUserPage ()->GetLogin ()) + "<br/><br/>";
		preview += "<strong>Title:</strong><br/>" + title + "<br/><br/>";
		preview += "<strong>Type:</strong><br/>" + typeText + "<br/><br/>";
		preview += "<strong>Category:</strong><br/>" + wiz->GetReportTypePage ()->GetCategoryName () + "<br/><br/>";
		preview += "<strong>Priority:</strong><br/>" + PriorityToString (wiz->GetReportTypePage ()->GetPriority ()) + "<br/><br/>";
		for (const auto& section : sections)
			preview += QString ("<strong>%1:</strong><br/>%2<br/><br/>")
					.arg (section.first)
					.arg (Qt::escape (section.second));
		preview += "<strong>Attached files:</strong><br/>" + wiz->GetFilePage ()->GetFiles ().join ("<br/>");

		preview.remove ("\r");
		preview.replace ("\n", "<br/>");

		Ui_.Preview_->setHtml (preview);
	}

	int PreviewPage::nextId () const
	{
		return ReportWizard::PageID::Final;
	}
}
}
