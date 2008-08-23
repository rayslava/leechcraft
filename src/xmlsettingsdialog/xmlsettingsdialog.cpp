/*
    Copyright (c) 2008 by Rudoy Georg <0xd34df00d@gmail.com>

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/
#include <QFile>
#include <QtGui/QtGui>
#include <QtXml/QtXml>
#include <QtDebug>
#include <QUrl>
#include <QDomNodeList>
#include "xmlsettingsdialog.h"
#include "rangewidget.h"
#include "filepicker.h"
#include "radiogroup.h"

XmlSettingsDialog::XmlSettingsDialog (QWidget *parent)
: QDialog (parent)
, Document_ (new QDomDocument)
{
    Pages_ = new QStackedWidget;
    Sections_ = new QListWidget;
    Sections_->setMinimumWidth (100);
    Sections_->setMaximumWidth (150);

    connect (Sections_, SIGNAL (currentRowChanged (int)), Pages_, SLOT (setCurrentIndex (int)));

    OK_ = new QPushButton (tr ("OK"));
    Cancel_ = new QPushButton (tr ("Cancel"));

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch (1);
    buttons->addWidget (OK_);
    buttons->addWidget (Cancel_);
    connect (OK_, SIGNAL (released ()), this, SLOT (accept ()));
    connect (Cancel_, SIGNAL (released ()), this, SLOT (reject ()));

    QVBoxLayout *rightLay = new QVBoxLayout;
    QHBoxLayout *mainLay = new QHBoxLayout (this);
    mainLay->addWidget (Sections_);
    rightLay->addWidget (Pages_);
    rightLay->addStretch (1);
    mainLay->addLayout (rightLay);
    rightLay->addLayout (buttons);
    setLayout (mainLay);

    DefaultLang_ = "en";
}

XmlSettingsDialog::~XmlSettingsDialog ()
{
}

void XmlSettingsDialog::RegisterObject (QObject* obj, const QString& filename)
{
    WorkingObject_ = obj;
    QFile file (filename);
    if (!file.open (QIODevice::ReadOnly))
    {
        qWarning () << "cannot open file";
        return;
    }
    QByteArray data = file.readAll ();
    file.close ();

    if (!Document_->setContent (data))
    {
        qWarning () << "Could not parse file";
        return;
    }
    QDomElement root = Document_->documentElement ();
    if (root.tagName () != "settings")
    {
        qWarning () << "Bad settings file";
        return;
    }

    QDomElement declaration = root.firstChildElement ("declare");
    while (!declaration.isNull ())
    {
        HandleDeclaration (declaration);
        declaration = declaration.nextSiblingElement ("declare");
    }

    QDomElement pageChild = root.firstChildElement ("page");
    while (!pageChild.isNull ())
    {
        ParsePage (pageChild);
        pageChild = pageChild.nextSiblingElement ("page");
    }

	UpdateXml (true);
}

void XmlSettingsDialog::HandleDeclaration (const QDomElement& decl)
{
    if (decl.hasAttribute ("defaultlang"))
        DefaultLang_ = decl.attribute ("defaultlang");
}

void XmlSettingsDialog::ParsePage (const QDomElement& page)
{
    QString sectionTitle = GetLabel (page);
    Sections_->addItem (sectionTitle);

    QWidget *baseWidget = new QWidget;
    Pages_->addWidget (baseWidget);
    QFormLayout *lay = new QFormLayout;
	lay->setRowWrapPolicy (QFormLayout::WrapLongRows);
	lay->setFieldGrowthPolicy (QFormLayout::FieldsStayAtSizeHint);
    baseWidget->setLayout (lay);

    ParseEntity (page, baseWidget);
}

void XmlSettingsDialog::ParseEntity (const QDomElement& entity, QWidget *baseWidget)
{
    QDomElement item = entity.firstChildElement ("item");
    while (!item.isNull ())
    {
        ParseItem (item, baseWidget);
        item = item.nextSiblingElement ("item");
    }

    QDomElement gbox = entity.firstChildElement ("groupbox");
    while (!gbox.isNull ())
    {
        QGroupBox *box = new QGroupBox (GetLabel (gbox));
		QFormLayout *groupLayout = new QFormLayout ();
		groupLayout->setRowWrapPolicy (QFormLayout::WrapLongRows);
		groupLayout->setFieldGrowthPolicy (QFormLayout::FieldsStayAtSizeHint);
		box->setLayout (groupLayout);
        ParseEntity (gbox, box);
        
        QFormLayout *lay = qobject_cast<QFormLayout*> (baseWidget->layout ());
        lay->addRow (box);

        gbox = gbox.nextSiblingElement ("groupbox");
    }

    QDomElement tab = entity.firstChildElement ("tab");
    if (!tab.isNull ())
    {
        QTabWidget *tabs = new QTabWidget;
        QFormLayout *lay = qobject_cast<QFormLayout*> (baseWidget->layout ());
        lay->addRow (tabs);
        while (!tab.isNull ())
        {
            QWidget *page = new QWidget;
            QFormLayout *widgetLay = new QFormLayout;
			widgetLay->setRowWrapPolicy (QFormLayout::WrapLongRows);
			widgetLay->setFieldGrowthPolicy (QFormLayout::FieldsStayAtSizeHint);
            page->setLayout (widgetLay);
            tabs->addTab (page, GetLabel (tab));
            ParseEntity (tab, page);
            tab = tab.nextSiblingElement ("tab");
        }
    }
}

void XmlSettingsDialog::ParseItem (const QDomElement& item, QWidget *baseWidget)
{
    QString type = item.attribute ("type");

    QFormLayout *lay = qobject_cast<QFormLayout*> (baseWidget->layout ());

    QString property = item.attribute ("property");
    QVariant value = WorkingObject_->property (property.toLatin1 ().constData ());
    if (!value.isValid () || value.isNull () && item.hasAttribute ("default"))
    {
        value = item.attribute ("default");
        WorkingObject_->setProperty (property.toLatin1 ().constData (), value);
    }

    if (type.isEmpty () || type.isNull ())
        return;
    else if (type == "lineedit")
        DoLineedit (item, lay, value);
    else if (type == "checkbox")
        DoCheckbox (item, lay, value);
    else if (type == "spinbox")
        DoSpinbox (item, lay, value);
    else if (type == "groupbox" && item.attribute ("checkable") == "true")
        DoGroupbox (item, lay, value);
    else if (type == "spinboxrange")
        DoSpinboxRange (item, lay, value);
    else if (type == "path")
        DoPath (item, lay, value);
    else if (type == "radio")
        DoRadio (item, lay, value);
    else if (type == "combobox")
        DoCombobox (item, lay, value);
    else
        qWarning () << Q_FUNC_INFO << "unhandled type" << type;

    WorkingObject_->setProperty (property.toLatin1 ().constData (), value);
}

QString XmlSettingsDialog::GetLabel (const QDomElement& item) const
{
    QString locale = QString(::getenv ("LANG")).left (2);
    if (locale.isNull () || locale.isEmpty ())
        locale = QLocale::system ().name ().toLower ();
    if (locale == "c")
        locale = "en";

    locale = locale.left (2);

    QString result = "<no label>";
    QDomElement label = item.firstChildElement ("label");
    while (!label.isNull ())
    {
        if (label.attribute ("lang").toLower () == locale)
        {
            result = label.attribute ("value");
            break;
        }
        label = label.nextSiblingElement ("label");
    }
    if (result == "<no label>")
    {
        label = item.firstChildElement ("label");
        while (!label.isNull ())
        {
            if (label.attribute ("lang").toLower () == DefaultLang_)
            {
                result = label.attribute ("value");
                break;
            }
            label = label.nextSiblingElement ("label");
        }
    }
    return result;
}

XmlSettingsDialog::LangElements XmlSettingsDialog::GetLangElements (const QDomElement& parent) const
{
    QString locale = QString(::getenv ("LANG")).left (2);
    if (locale.isNull () || locale.isEmpty ())
        locale = QLocale::system ().name ().toLower ();
    if (locale == "c")
        locale = "en";

    locale = locale.left (2);
    LangElements returning;
    returning.Valid_ = false;

    bool found = false;

    QDomElement result = parent.firstChildElement ("lang");
    while (!result.isNull ())
    {
        if (result.attribute ("value").toLower () == locale)
        {
            found = true;
            break;
        }
        result = result.nextSiblingElement ("lang");
    }
    if (!found)
    {
        result = parent.firstChildElement ("lang");
        while (!result.isNull ())
        {
            if (result.attribute ("value").toLower () == DefaultLang_)
            {
                found = true;
                break;
            }
            result = result.nextSiblingElement ("lang");
        }
    }
    if (!found)
    {
        result = parent.firstChildElement ("lang");
        while (!result.isNull ())
        {
            if (result.attribute ("value").toLower () == "en" || !result.hasAttribute ("value"))
            {
                found = true;
                break;
            }
            result = result.nextSiblingElement ("lang");
        }
    }
    if (result.isNull ())
        return returning;

    returning.Valid_ = true;

    QDomElement label = result.firstChildElement ("label");
    if (!label.isNull () && label.hasAttribute ("value"))
    {
        returning.Label_.first = true;
        returning.Label_.second = label.attribute ("value");
    }
    else
        returning.Label_.first = false;

    QDomElement suffix = result.firstChildElement ("suffix");
    if (!suffix.isNull () && suffix.hasAttribute ("value"))
    {
        returning.Suffix_.first = true;
        returning.Suffix_.second = suffix.attribute ("value");
    }
    else
        returning.Suffix_.first = false;

    return returning;
}

void XmlSettingsDialog::DoLineedit (const QDomElement& item, QFormLayout *lay, QVariant& value)
{
    QLabel *label = new QLabel (GetLabel (item));
    label->setWordWrap (false);

    QLineEdit *edit = new QLineEdit (value.toString ());
    edit->setObjectName (item.attribute ("property"));
    edit->setMinimumWidth (QApplication::fontMetrics ().width ("thisismaybeadefaultsetting"));
    if (item.hasAttribute ("password"))
        edit->setEchoMode (QLineEdit::Password);
	if (item.hasAttribute ("inputMask"))
		edit->setInputMask (item.attribute ("inputMask"));
    connect (edit, SIGNAL (textChanged (const QString&)), this, SLOT (updatePreferences ()));

	lay->addRow (label, edit);
}

void XmlSettingsDialog::DoCheckbox (const QDomElement& item, QFormLayout *lay, QVariant& value)
{
    QCheckBox *box = new QCheckBox (GetLabel (item));
    box->setObjectName (item.attribute ("property"));
    if (!value.isValid () ||
			value.isNull () ||
			value.toString () == "on" ||
			value.toString () == "off")
	{
		if (item.hasAttribute ("default"))
			value = (item.attribute ("default") == "on");
		else
			value = (item.attribute ("state") == "on");
	}
    box->setCheckState (value.toBool () ? Qt::Checked : Qt::Unchecked);
    connect (box, SIGNAL (stateChanged (int)), this, SLOT (updatePreferences ()));

    lay->addRow (box);
}

void XmlSettingsDialog::DoSpinbox (const QDomElement& item, QFormLayout *lay, QVariant& value)
{
    QLabel *label = new QLabel (GetLabel (item));
    label->setWordWrap (false);
    QSpinBox *box = new QSpinBox;
    box->setObjectName (item.attribute ("property"));
    if (item.hasAttribute ("minimum"))
        box->setMinimum (item.attribute ("minimum").toInt ());
    if (item.hasAttribute ("maximum"))
        box->setMaximum (item.attribute ("maximum").toInt ());
    if (item.hasAttribute ("step"))
        box->setSingleStep (item.attribute ("step").toInt ());
    if (item.hasAttribute ("suffix"))
        box->setSuffix (item.attribute ("suffix"));
    LangElements langs = GetLangElements (item);
    if (langs.Valid_)
    {
        if (langs.Label_.first)
            label->setText (langs.Label_.second);
        if (langs.Suffix_.first)
            box->setSuffix (langs.Suffix_.second);
    }
    box->setValue (value.toInt ());
    connect (box, SIGNAL (valueChanged (int)), this, SLOT (updatePreferences ()));

	lay->addRow (label, box);
}

void XmlSettingsDialog::DoGroupbox (const QDomElement& item, QFormLayout *lay, QVariant& value)
{
    QGroupBox *box = new QGroupBox (GetLabel (item));
    box->setObjectName (item.attribute ("property"));
	QFormLayout *groupLayout = new QFormLayout ();
	groupLayout->setRowWrapPolicy (QFormLayout::WrapLongRows);
	groupLayout->setFieldGrowthPolicy (QFormLayout::FieldsStayAtSizeHint);
    box->setLayout (groupLayout);
    box->setCheckable (true);
    if (!value.isValid () ||
			value.isNull () ||
			value.toString () == "on" ||
			value.toString () == "off")
	{
		if (item.hasAttribute ("default"))
			value = (item.attribute ("default") == "on");
		else
			value = (item.attribute ("state") == "on");
	}
    box->setChecked (value.toBool ());
    connect (box, SIGNAL (toggled (bool)), this, SLOT (updatePreferences ()));
    ParseEntity (item, box);
    
    lay->addRow (box);
}

void XmlSettingsDialog::DoSpinboxRange (const QDomElement& item, QFormLayout *lay, QVariant& value)
{
    if (!value.isValid () || value.isNull () || !value.canConvert<QList<QVariant> > ())
    {
        QStringList parts = item.attribute ("default").split (":");
        QList<QVariant> result;
        if (parts.size () != 2)
        {
            qWarning () << "spinboxrange parse error, wrong default value";
            return;
        }
        result << parts.at (0).toInt () << parts.at (1).toInt ();
        value = result;
    }

    QLabel *label = new QLabel (GetLabel (item));
    label->setWordWrap (false);
    RangeWidget *widget = new RangeWidget ();
    widget->setObjectName (item.attribute ("property"));
    widget->SetMinimum (item.attribute ("minimum").toInt ());
    widget->SetMaximum (item.attribute ("maximum").toInt ());
    widget->SetRange (value);
    connect (widget, SIGNAL (changed ()), this, SLOT (updatePreferences ()));

	lay->addRow (label, widget);
}

void XmlSettingsDialog::DoPath (const QDomElement& item, QFormLayout *lay, QVariant& value)
{
    if (value.isNull () || value.toString ().isEmpty ())
        if (item.hasAttribute ("defaultHomePath") && item.attribute ("defaultHomePath") == "true")
            value = QDir::homePath ();
    QLabel *label = new QLabel (GetLabel (item));
    label->setWordWrap (false);
    FilePicker *picker = new FilePicker (this);
    picker->SetText (value.toString ());
    picker->setObjectName (item.attribute ("property"));
    connect (picker, SIGNAL (textChanged (const QString&)), this, SLOT (updatePreferences ()));

	lay->addRow (label, picker);
}

void XmlSettingsDialog::DoRadio (const QDomElement& item, QFormLayout *lay, QVariant& value)
{
    RadioGroup *group = new RadioGroup (this);
    group->setObjectName (item.attribute ("property"));

    QDomElement option = item.firstChildElement ("option");
    while (!option.isNull ())
    {
        QRadioButton *button = new QRadioButton (GetLabel (option));
        button->setObjectName (option.attribute ("name"));
		group->AddButton (button,
				option.hasAttribute ("default") &&
				option.attribute ("default") == "true");
        if (option.attribute ("default") == "true")
            value = option.attribute ("name");
        option = option.nextSiblingElement ("option");
    }
    connect (group, SIGNAL (valueChanged ()), this, SLOT (updatePreferences ()));

    QGroupBox *box = new QGroupBox (GetLabel (item));
    QVBoxLayout *layout = new QVBoxLayout ();
    box->setLayout (layout);
    layout->addWidget (group);

    lay->addRow (box);
}

void XmlSettingsDialog::DoCombobox (const QDomElement& item, QFormLayout *lay, QVariant& value)
{
    QComboBox *box = new QComboBox (this);
    box->setObjectName (item.attribute ("property"));
	if (item.hasAttribute ("maxVisibleItems"))
		box->setMaxVisibleItems (item.attribute ("maxVisibleItems").toInt ());

    QDomElement option = item.firstChildElement ("option");
    while (!option.isNull ())
    {
		QList<QImage> images = GetImages (option);
		if (images.size ())
		{
			QIcon icon = QIcon (QPixmap::fromImage (images.at (0)));
			box->addItem (icon, GetLabel (option), option.attribute ("name"));
		}
		else
			box->addItem (GetLabel (option), option.attribute ("name"));

        if (option.attribute ("default") == "true")
        {
            box->setCurrentIndex (box->count () - 1);
            value = option.attribute ("name");
        }
        option = option.nextSiblingElement ("option");
    }
    connect (box, SIGNAL (currentIndexChanged (int)), this, SLOT (updatePreferences ()));

    QLabel *label = new QLabel (GetLabel (item));
	label->setWordWrap (false);

	lay->addRow (label, box);
}

QList<QImage> XmlSettingsDialog::GetImages (const QDomElement& item) const
{
	QList<QImage> result;
	QDomElement binary = item.firstChildElement ("binary");
	while (!binary.isNull ())
	{
		QByteArray data;
		if (binary.attribute ("place") == "rcc")
		{
			QFile file (binary.text ());
			if (!file.open (QIODevice::ReadOnly))
			{
				qWarning () << Q_FUNC_INFO
					<< "could not open file"
					<< binary.text ()
					<< ", because"
					<< file.errorString ();
				continue;
			}
			data = file.readAll ();
		}
		else
		{
			QByteArray base64 = binary.text ().toLatin1 ();
			data = QByteArray::fromBase64 (base64);
		}
		if (binary.attribute ("type") == "image")
		{
			QImage image = QImage::fromData (data);
			if (!image.isNull ())
				result << image;
		}
		binary = binary.nextSiblingElement ("binary");
	}
	return result;
}

void XmlSettingsDialog::UpdateXml (bool whole)
{
	QDomNodeList nodes = Document_->elementsByTagName ("item");
	if (whole)
		for (int i = 0; i < nodes.size (); ++i)
		{
			QDomElement elem = nodes.at (i).toElement ();
			if (!elem.hasAttribute ("property"))
				continue;

			QString name = elem.attribute ("property");
			QVariant value = WorkingObject_->property (name.toLatin1 ().constData ());

			UpdateSingle (name, value, elem);
		}
	else
		for (Property2Value_t::const_iterator i = Prop2NewValue_.begin (),
				end = Prop2NewValue_.end (); i != end; ++i)
		{
			QDomElement element;
			QString name = i.key ();
			qDebug () << nodes.size ();
			for (int j = 0, size = nodes.size (); j < size; ++j)
			{
				QDomElement e = nodes.at (j).toElement ();
				qDebug () << e.tagName ();
				if (e.isNull ())
					continue;
				if (e.attribute ("property") == name)
				{
					element = e;
					break;
				}
			}
			if (element.isNull ())
			{
				qWarning () << Q_FUNC_INFO << "element for property" << name << "not found";
				return;
			}

			UpdateSingle (name, i.value (), element);
		}
}

void XmlSettingsDialog::UpdateSingle (const QString& name,
		const QVariant& value, QDomElement& element)
{
	QString type = element.attribute ("type");
	if (type == "lineedit" ||
			type == "checkbox" ||
			type == "spinbox" ||
			type == "groupbox" ||
			type == "path")
		element.setAttribute ("default", value.toString ());
	else if (type == "spinboxrange")
	{
		QStringList vals = value.toStringList ();
		if (vals.size () != 2)
		{
			qWarning () << Q_FUNC_INFO << "spinboxrange value error, not 2 elems in list";
			return;
		}
		element.setAttribute ("default", vals.at (0) + ':' + vals.at (1));
	}
	else if (type == "radio" ||
			type == "combobox")
	{
		QDomNodeList options = element.elementsByTagName ("option");
		for (int i = 0; i < options.size (); ++i)
			options.at (i).toElement ().removeAttribute ("default");

		for (int i = 0; i < options.size (); ++i)
		{
			QDomElement option = options.at (i).toElement ();
			QString optName = value.toString ();
			if (option.attribute ("name") == optName)
			{
				option.setAttribute ("default", "true");
				break;
			}
		}
	}
}

QString XmlSettingsDialog::GetXml () const
{
	return Document_->toString ();
}

void XmlSettingsDialog::updatePreferences ()
{
    QString propertyName = sender ()->objectName ();
    if (propertyName.isEmpty ())
    {
        qWarning () << Q_FUNC_INFO << "property name is empty for object" << sender ();
        return;
    }
    QVariant value;

    QLineEdit *edit = qobject_cast<QLineEdit*> (sender ());
    QCheckBox *checkbox = qobject_cast<QCheckBox*> (sender ());
    QSpinBox *spinbox = qobject_cast<QSpinBox*> (sender ());
    QGroupBox *groupbox = qobject_cast<QGroupBox*> (sender ());
    RangeWidget *rangeWidget = qobject_cast<RangeWidget*> (sender ());
    FilePicker *picker = qobject_cast<FilePicker*> (sender ());
    RadioGroup *radiogroup = qobject_cast<RadioGroup*> (sender ());
    QComboBox *combobox = qobject_cast<QComboBox*> (sender ());
    if (edit)
        value = edit->text ();
    else if (checkbox)
        value = checkbox->checkState ();
    else if (spinbox)
        value = spinbox->value ();
    else if (groupbox)
        value = groupbox->isChecked ();
    else if (rangeWidget)
        value = rangeWidget->GetRange ();
    else if (picker)
        value = picker->GetText ();
    else if (radiogroup)
        value = radiogroup->GetValue ();
    else if (combobox)
        value = combobox->itemData (combobox->currentIndex ());
    else
    {
        qWarning () << Q_FUNC_INFO << "unhandled sender" << sender ();
        return;
    }

    Prop2NewValue_ [propertyName] = value;
}

void XmlSettingsDialog::accept ()
{
    for (Property2Value_t::const_iterator i = Prop2NewValue_.begin (),
			end = Prop2NewValue_.end (); i != end; ++i)
        WorkingObject_->setProperty (i.key ().toLatin1 ().constData (), i.value ());

	UpdateXml ();

	Prop2NewValue_.clear ();

    QDialog::accept ();
}

void XmlSettingsDialog::reject ()
{
    for (Property2Value_t::iterator i = Prop2NewValue_.begin (); i != Prop2NewValue_.end (); ++i)
    {
        QWidget *object = findChild<QWidget*> (i.key ());
        if (!object)
        {
            qWarning () << Q_FUNC_INFO << "could not find object for property" << i.key ();
            continue;
        }

        QVariant oldValue = WorkingObject_->property (i.key ().toLatin1 ().constData ());

        QLineEdit *edit = qobject_cast<QLineEdit*> (object);
        QCheckBox *checkbox = qobject_cast<QCheckBox*> (object);
        QSpinBox *spinbox = qobject_cast<QSpinBox*> (object);
        QGroupBox *groupbox = qobject_cast<QGroupBox*> (object);
        RangeWidget *rangeWidget = qobject_cast<RangeWidget*> (object);
        FilePicker *picker = qobject_cast<FilePicker*> (object);
        RadioGroup *radiogroup = qobject_cast<RadioGroup*> (object);
        QComboBox *combobox = qobject_cast<QComboBox*> (object);
        if (edit)
            edit->setText (oldValue.toString ());
        else if (checkbox)
            checkbox->setCheckState (oldValue.toBool () ? Qt::Checked : Qt::Unchecked);
        else if (spinbox)
            spinbox->setValue (oldValue.toLongLong ());
        else if (groupbox)
            groupbox->setChecked (oldValue.toBool ());
        else if (rangeWidget)
            rangeWidget->SetRange (oldValue);
        else if (picker)
            picker->SetText (oldValue.toString ());
        else if (radiogroup)
            radiogroup->SetValue (oldValue.toString ());
        else if (combobox)
            combobox->setCurrentIndex (combobox->findText (oldValue.toString ()));
        else
        {
            qWarning () << Q_FUNC_INFO << "unhandled object" << object << "for" << i.key ();
            continue;
        }
    }
	
	Prop2NewValue_.clear ();

    QDialog::reject ();
}

