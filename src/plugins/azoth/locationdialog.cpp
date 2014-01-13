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

#include "locationdialog.h"
#include <QUrl>

namespace LeechCraft
{
namespace Azoth
{
	LocationDialog::LocationDialog (QWidget *parent)
	: QDialog (parent)
	{
		Ui_.setupUi (this);
		
		Ui_.Timestamp_->setDateTime (QDateTime::currentDateTime ());
	}
	
	GeolocationInfo_t LocationDialog::GetInfo () const
	{
		GeolocationInfo_t result;

		if (Ui_.SetAccuracy_->checkState () == Qt::Checked)
			result ["accuracy"] = Ui_.Accuracy_->value ();
		
		if (Ui_.SetAltitude_->checkState () == Qt::Checked)
			result ["alt"] = Ui_.Altitude_->value ();

		result ["area"] = Ui_.Area_->text ();
		
		if (Ui_.SetBearing_->checkState () == Qt::Checked)
			result ["bearing"] = Ui_.Bearing_->value ();

		result ["building"] = Ui_.Building_->text ();
		result ["country"] = Ui_.Country_->text ();
		result ["countrycode"] = Ui_.CountryCode_->text ();
		result ["datum"] = Ui_.Datum_->text ();
		result ["description"] = Ui_.Description_->text ();
		result ["floor"] = Ui_.Floor_->text ();
		
		if (Ui_.SetLatitude_->checkState () == Qt::Checked)
			result ["lat"] = Ui_.Latitude_->value ();

		result ["locality"] = Ui_.Locality_->text ();
		
		if (Ui_.SetLongitude_->checkState () == Qt::Checked)
			result ["lon"] = Ui_.Longitude_->value ();

		result ["postalcode"] = Ui_.PostalCode_->text ();
		result ["region"] = Ui_.Region_->text ();
		result ["room"] = Ui_.Room_->text ();
		
		if (Ui_.SetSpeed_->checkState () == Qt::Checked)
			result ["speed"] = Ui_.Speed_->value ();

		result ["street"] = Ui_.Street_->text ();

		if (Ui_.SetTimestamp_->checkState () == Qt::Checked)
			result ["timestamp"] = Ui_.Timestamp_->dateTime ().toString (Qt::ISODate);

		if (!Ui_.URL_->text ().isEmpty ())
			result ["uri"] = QUrl::fromUserInput (Ui_.URL_->text ());
			
		Q_FOREACH (const QString& key, result.keys (QString ()) + result.keys (QString ("")))
			result.remove (key);
		
		return result;
	}
	
	void LocationDialog::SetInfo (const GeolocationInfo_t& info)
	{
		if (info.contains ("accuracy"))
			Ui_.Accuracy_->setValue (info ["accuracy"].toDouble ());
		else
			Ui_.SetAccuracy_->setCheckState (Qt::Unchecked);

		if (info.contains ("alt"))
			Ui_.Altitude_->setValue (info ["alt"].toDouble ());
		else
			Ui_.SetAltitude_->setCheckState (Qt::Unchecked);

		Ui_.Area_->setText (info ["area"].toString ());
		
		if (info.contains ("bearing"))
			Ui_.Bearing_->setValue (info ["bearing"].toDouble ());
		else
			Ui_.SetBearing_->setCheckState (Qt::Unchecked);

		Ui_.Building_->setText (info ["building"].toString ());
		Ui_.Country_->setText (info ["country"].toString ());
		Ui_.CountryCode_->setText (info ["countrycode"].toString ());
		Ui_.Datum_->setText (info ["datum"].toString ());
		Ui_.Description_->setText (info ["description"].toString ());
		Ui_.Floor_->setText (info ["floor"].toString ());
		
		if (info.contains ("lat"))
			Ui_.Latitude_->setValue (info ["lat"].toDouble ());
		else
			Ui_.SetLatitude_->setCheckState (Qt::Unchecked);

		Ui_.Locality_->setText (info ["locality"].toString ());
		
		if (info.contains ("lon"))
			Ui_.Longitude_->setValue (info ["lon"].toDouble ());
		else
			Ui_.SetLongitude_->setCheckState (Qt::Unchecked);

		Ui_.PostalCode_->setText (info ["postalcode"].toString ());
		Ui_.Region_->setText (info ["region"].toString ());
		Ui_.Room_->setText (info ["room"].toString ());
		
		if (info.contains ("speed"))
			Ui_.Speed_->setValue (info ["speed"].toDouble ());
		else
			Ui_.SetSpeed_->setCheckState (Qt::Unchecked);

		Ui_.Street_->setText (info ["street"].toString ());
		
		if (info.contains ("timestamp"))
			Ui_.Timestamp_->setDateTime (info ["timestamp"].toDateTime ());
		else
			Ui_.SetTimestamp_->setCheckState (Qt::Unchecked);

		Ui_.URL_->setText (info ["uri"].toUrl ().toString ());
	}
}
}

