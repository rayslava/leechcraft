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

#include "udisks2backend.h"
#include <memory>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <QStandardItemModel>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QTimer>
#include <QtDebug>
#include <QMetaMethod>
#include <util/util.h>
#include <interfaces/devices/deviceroles.h>
#include "udisks2types.h"

typedef std::shared_ptr<QDBusInterface> QDBusInterface_ptr;

namespace LeechCraft
{
namespace Vrooby
{
namespace UDisks2
{
	Backend::Backend (QObject *parent)
	: DevBackend (parent)
	, DevicesModel_ (new QStandardItemModel (this))
	, UDisksObj_ (0)
	{
	}

	QString Backend::GetBackendName () const
	{
		return "UDisks2";
	}

	bool Backend::IsAvailable ()
	{
		auto sb = QDBusConnection::systemBus ();
		auto iface = sb.interface ();

		auto services = iface->registeredServiceNames ()
				.value ().filter ("org.freedesktop.UDisks2");
		if (services.isEmpty ())
		{
			iface->startService ("org.freedesktop.UDisks2");
			services = iface->registeredServiceNames ()
					.value ().filter ("org.freedesktop.UDisks2");
			if (services.isEmpty ())
				return false;
		}

		return true;
	}

	void Backend::Start ()
	{
		qDBusRegisterMetaType<VariantMapMap_t> ();
		qDBusRegisterMetaType<EnumerationResult_t> ();
		qDBusRegisterMetaType<ByteArrayList_t> ();

		InitialEnumerate ();

		auto timer = new QTimer (this);
		connect (timer,
				SIGNAL (timeout ()),
				this,
				SLOT (updateDeviceSpaces ()));
		timer->start (10000);
	}

	bool Backend::SupportsDevType (DeviceType type) const
	{
		return type == DeviceType::MassStorage;
	}

	QAbstractItemModel* Backend::GetDevicesModel () const
	{
		return DevicesModel_;
	}

	namespace
	{
		QDBusInterface_ptr GetBlockInterface (const QString& path)
		{
			return QDBusInterface_ptr (new QDBusInterface ("org.freedesktop.UDisks2",
						path,
						"org.freedesktop.UDisks2.Block",
						QDBusConnection::systemBus ()));
		}

		QDBusInterface_ptr GetPartitionInterface (const QString& path)
		{
			return QDBusInterface_ptr (new QDBusInterface ("org.freedesktop.UDisks2",
						path,
						"org.freedesktop.UDisks2.Partition",
						QDBusConnection::systemBus ()));
		}

		QDBusInterface_ptr GetFSInterface (const QString& path)
		{
			return QDBusInterface_ptr (new QDBusInterface ("org.freedesktop.UDisks2",
						path,
						"org.freedesktop.UDisks2.Filesystem",
						QDBusConnection::systemBus ()));
		}

		QDBusInterface_ptr GetDevInterface (const QString& path)
		{
			return QDBusInterface_ptr (new QDBusInterface ("org.freedesktop.UDisks2",
						path,
						"org.freedesktop.UDisks2.Drive",
						QDBusConnection::systemBus ()));
		}

		QDBusInterface_ptr GetPropsInterface (const QString& path)
		{
			return QDBusInterface_ptr (new QDBusInterface ("org.freedesktop.UDisks2",
						path,
						"org.freedesktop.DBus.Properties",
						QDBusConnection::systemBus ()));
		}
	}

	void Backend::MountDevice (const QString& id)
	{
		auto iface = GetFSInterface (id);
		if (!iface)
			return;

		auto item = Object2Item_.value (id);
		if (!item)
			return;

		const bool isMounted = !item->data (MassStorageRole::MountPoints).toStringList ().isEmpty ();
		if (!isMounted)
		{
			auto async = iface->asyncCall ("Mount", QVariantMap ());
			connect (new QDBusPendingCallWatcher (async, this),
					SIGNAL (finished (QDBusPendingCallWatcher*)),
					this,
					SLOT (mountCallFinished (QDBusPendingCallWatcher*)));
		}
	}

	void Backend::InitialEnumerate ()
	{
		if (!IsAvailable ())
			return;

		auto sb = QDBusConnection::systemBus ();

		UDisksObj_ = new org::freedesktop::DBus::ObjectManager ("org.freedesktop.UDisks2", "/org/freedesktop/UDisks2", sb);
		auto reply = UDisksObj_->GetManagedObjects ();
		auto watcher = new QDBusPendingCallWatcher (reply, this);
		connect (watcher,
				SIGNAL (finished (QDBusPendingCallWatcher*)),
				this,
				SLOT (handleEnumerationFinished (QDBusPendingCallWatcher*)));

		connect (UDisksObj_,
				SIGNAL (InterfacesAdded (QDBusObjectPath, VariantMapMap_t)),
				this,
				SLOT (handleDeviceAdded (QDBusObjectPath, VariantMapMap_t)));
		connect (UDisksObj_,
				SIGNAL (InterfacesRemoved (QDBusObjectPath, QStringList)),
				this,
				SLOT (handleDeviceRemoved (QDBusObjectPath)));
	}

	bool Backend::AddPath (const QDBusObjectPath& path)
	{
		const auto& str = path.path ();
		if (Object2Item_.contains (str))
			return true;

		auto blockIface = GetBlockInterface (str);
		if (!blockIface->isValid ())
		{
			qWarning () << Q_FUNC_INFO
					<< "invalid interface for"
					<< str
					<< blockIface->lastError ().message ();
			return false;
		}

		const auto& driveId = blockIface->property ("Drive").value<QDBusObjectPath> ().path ();

		auto driveIface = driveId.isEmpty () ? QDBusInterface_ptr () : GetDevInterface (driveId);
		if (!driveIface || !driveIface->isValid ())
			return false;

		auto partitionIface = GetPartitionInterface (str);
		const bool isPartition = !partitionIface->property ("Type").toString ().isEmpty ();

		const auto& slaveTo = partitionIface->property ("Table").value<QDBusObjectPath> ();
		const bool isRemovable = driveIface->property ("Removable").toBool ();
		qDebug () << str << slaveTo.path () << isPartition << isRemovable;
		if ((!isPartition && !isRemovable) || Unremovables_.contains (slaveTo.path ()))
		{
			qDebug () << "detected as unremovable";
			Unremovables_ << str;
			return false;
		}

		QDBusConnection::systemBus ().connect ("org.freedesktop.UDisks2",
				path.path (), "org.freedesktop.DBus.Properties", "PropertiesChanged",
				this, SLOT (handleDeviceChanged (QDBusMessage)));

		auto item = new QStandardItem;
		Object2Item_ [str] = item;
		SetItemData ({ partitionIface, GetFSInterface (str), blockIface, driveIface, GetPropsInterface (str) }, item);
		if (slaveTo.path ().isEmpty ())
			DevicesModel_->appendRow (item);
		else
		{
			if (!Object2Item_.contains (slaveTo.path ()))
				if (!AddPath (slaveTo))
					return false;

			Object2Item_ [slaveTo.path ()]->appendRow (item);
		}
		return true;
	}

	void Backend::RemovePath (const QDBusObjectPath& pathObj)
	{
		const auto& path = pathObj.path ();
		auto item = Object2Item_.take (path);
		if (!item)
			return;

		auto getChildren = [] (QStandardItem *item)
		{
			QList<QStandardItem*> result;
			for (int i = 0; i < item->rowCount (); ++i)
				result << item->child (i);
			return result;
		};

		QList<QStandardItem*> toRemove = getChildren (item);
		for (int i = 0; i < toRemove.size (); ++i)
			toRemove += getChildren (toRemove [i]);

		for (QStandardItem *item : toRemove)
			Object2Item_.remove (Object2Item_.key (item));

		if (item->parent ())
			item->parent ()->removeRow (item->row ());
		else
			DevicesModel_->removeRow (item->row ());
	}

	namespace
	{
		QString GetPartitionName (QDBusInterface_ptr partition, QDBusInterface_ptr block)
		{
			auto result = block->property ("IdLabel").toString ().trimmed ();
			if (!result.isEmpty ())
				return result;

			result = partition->property ("Name").toString ().trimmed ();
			if (!result.isEmpty ())
				return result;

			return Backend::tr ("Partition %1").arg (partition->property ("Number").toInt ());
		}
	}

	void Backend::SetItemData (const ItemInterfaces& ifaces, QStandardItem *item)
	{
		if (!item)
			return;

		const bool isRemovable = ifaces.Drive_->property ("Removable").toBool ();
		const bool isPartition = ifaces.Block_->property ("IdUsage").toString () == "filesystem";

		const auto& vendor = ifaces.Drive_->property ("Vendor").toString () +
				" " +
				ifaces.Drive_->property ("Model").toString ();
		const auto& partName = GetPartitionName (ifaces.Partition_, ifaces.Block_);
		const auto& name = isPartition ? partName : vendor;
		const auto& fullName = isPartition ?
				QString ("%1: %2").arg (vendor, partName) :
				vendor;

		DevicesModel_->blockSignals (true);

		QStringList mountPaths;
		auto msg = ifaces.Props_->call ("Get",
				"org.freedesktop.UDisks2.Filesystem", "MountPoints");
		QDBusReply<QDBusVariant> reply (msg);

		if (reply.isValid ())
			for (const auto& point : qdbus_cast<ByteArrayList_t> (reply.value ().variant ()))
				mountPaths << QString::fromUtf8 (point);

		if (!mountPaths.isEmpty ())
		{
			qint64 space = -1;
			try
			{
				const auto& wstrMount = mountPaths.value (0).toStdWString ();
				space = static_cast<qint64> (boost::filesystem::space (wstrMount).free);
			}
			catch (const std::exception& e)
			{
				qWarning () << Q_FUNC_INFO
						<< "error obtaining free space info:"
						<< QString::fromUtf8 (e.what ());
			}
			item->setData (static_cast<qint64> (space), MassStorageRole::AvailableSize);
		}
		else
			item->setData (-1, MassStorageRole::AvailableSize);

		item->setText (name);
		item->setData (DeviceType::MassStorage, CommonDevRole::DevType);
		item->setData (ifaces.Block_->property ("Device").toByteArray (), MassStorageRole::DevFile);
		item->setData (ifaces.Partition_->property ("PartitionType").toInt (), MassStorageRole::PartType);
		item->setData (isRemovable, MassStorageRole::IsRemovable);
		item->setData (isPartition, MassStorageRole::IsPartition);
		item->setData (isPartition && isRemovable, MassStorageRole::IsMountable);
		item->setData (!mountPaths.isEmpty (), MassStorageRole::IsMounted);
		item->setData (ifaces.Drive_->property ("MediaAvailable"), MassStorageRole::IsMediaAvailable);
		item->setData (ifaces.Block_->path (), CommonDevRole::DevID);
		item->setData (ifaces.Block_->property ("IdUUID"), CommonDevRole::DevPersistentID);
		item->setData (fullName, MassStorageRole::VisibleName);
		item->setData (ifaces.Block_->property ("Size").toLongLong (), MassStorageRole::TotalSize);
		DevicesModel_->blockSignals (false);
		item->setData (mountPaths, MassStorageRole::MountPoints);
	}

	void Backend::toggleMount (const QString& id)
	{
		auto iface = GetFSInterface (id);
		if (!iface->isValid ())
			return;

		auto item = Object2Item_.value (id);
		if (!item)
			return;

		const bool isMounted = !item->data (MassStorageRole::MountPoints).toStringList ().isEmpty ();
		if (isMounted)
		{
			auto async = iface->asyncCall ("Unmount", QVariantMap ());
			connect (new QDBusPendingCallWatcher (async, this),
					SIGNAL (finished (QDBusPendingCallWatcher*)),
					this,
					SLOT (umountCallFinished (QDBusPendingCallWatcher*)));
		}
		else
		{
			auto async = iface->asyncCall ("Mount", QVariantMap ());
			connect (new QDBusPendingCallWatcher (async, this),
					SIGNAL (finished (QDBusPendingCallWatcher*)),
					this,
					SLOT (mountCallFinished (QDBusPendingCallWatcher*)));
		}
	}

	namespace
	{
		QString GetErrorText (const QString& errorCode)
		{
			QMap<QString, QString> texts;
			texts ["org.freedesktop.UDisks.Error.PermissionDenied"] = Backend::tr ("permission denied");
			texts ["org.freedesktop.PolicyKit.Error.NotAuthorized"] = Backend::tr ("not authorized");
			texts ["org.freedesktop.PolicyKit.Error.Busy"] = Backend::tr ("the device is busy");
			texts ["org.freedesktop.PolicyKit.Error.Failed"] = Backend::tr ("the operation has failed");
			texts ["org.freedesktop.PolicyKit.Error.Cancelled"] = Backend::tr ("the operation has been cancelled");
			texts ["org.freedesktop.PolicyKit.Error.InvalidOption"] = Backend::tr ("invalid mount options were given");
			texts ["org.freedesktop.PolicyKit.Error.FilesystemDriverMissing"] = Backend::tr ("unsupported filesystem");
			return texts.value (errorCode, Backend::tr ("unknown error"));
		}
	}

	void Backend::mountCallFinished (QDBusPendingCallWatcher *watcher)
	{
		qDebug () << Q_FUNC_INFO;
		watcher->deleteLater ();
		QDBusPendingReply<QString> reply = *watcher;

		if (!reply.isError ())
		{
			emit gotEntity (Util::MakeNotification ("Vrooby",
						tr ("Device has been successfully mounted at %1.")
							.arg (reply.value ()),
						PInfo_));
			return;
		}

		const auto& error = reply.error ();
		qWarning () << Q_FUNC_INFO
				<< error.name ()
				<< error.message ();
		emit gotEntity (Util::MakeNotification ("Vrooby",
					tr ("Failed to mount the device: %1 (%2).")
						.arg (GetErrorText (error.name ()))
						.arg (error.message ()),
					PCritical_));
	}

	void Backend::umountCallFinished (QDBusPendingCallWatcher *watcher)
	{
		qDebug () << Q_FUNC_INFO;
		watcher->deleteLater ();
		QDBusPendingReply<void> reply = *watcher;

		if (!reply.isError ())
		{
			emit gotEntity (Util::MakeNotification ("Vrooby",
						tr ("Device has been successfully unmounted."),
						PInfo_));
			return;
		}

		const auto& error = reply.error ();
		qWarning () << Q_FUNC_INFO
				<< error.name ()
				<< error.message ();
		emit gotEntity (Util::MakeNotification ("Vrooby",
					tr ("Failed to unmount the device: %1 (%2).")
						.arg (GetErrorText (error.name ()))
						.arg (error.message ()),
					PCritical_));
	}

	void Backend::handleEnumerationFinished (QDBusPendingCallWatcher *watcher)
	{
		watcher->deleteLater ();
		QDBusPendingReply<EnumerationResult_t> reply = *watcher;
		if (reply.isError ())
		{
			qWarning () << reply.error ().message ();
			return;
		}

		for (const QDBusObjectPath& path : reply.value ().keys ())
			AddPath (path);
	}

	void Backend::handleDeviceAdded (const QDBusObjectPath& path, const VariantMapMap_t&)
	{
		AddPath (path);
	}

	void Backend::handleDeviceRemoved (const QDBusObjectPath& path)
	{
		RemovePath (path);
	}

	void Backend::handleDeviceChanged (const QDBusMessage& msg)
	{
		const auto& path = msg.path ();

		auto item = Object2Item_.value (path);
		if (!item)
		{
			qWarning () << Q_FUNC_INFO
					<< "no item for path"
					<< path;
			return;
		}

		auto blockIface = GetBlockInterface (path);
		const ItemInterfaces faces =
		{
			GetPartitionInterface (path),
			GetFSInterface (path),
			blockIface,
			GetDevInterface (blockIface->property ("Drive").value<QDBusObjectPath> ().path ()),
			GetPropsInterface (path)
		};
		SetItemData (faces, item);
	}

	void Backend::updateDeviceSpaces ()
	{
		for (QStandardItem *item : Object2Item_.values ())
		{
			const auto& mountPaths = item->data (MassStorageRole::MountPoints).toStringList ();
			if (mountPaths.isEmpty ())
				continue;

			const auto& space = boost::filesystem::space (mountPaths.value (0).toStdWString ());
			const auto free = static_cast<qint64> (space.free);
			if (free != item->data (MassStorageRole::AvailableSize).value<qint64> ())
				item->setData (static_cast<qint64> (free), MassStorageRole::AvailableSize);
		}
	}
}
}
}
