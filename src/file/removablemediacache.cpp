/*
   This file is part of the KDE Baloo project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>
   Copyright (C) 2014 Vishesh Handa <vhanda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "removablemediacache.h"

#include <Solid/DeviceNotifier>
#include <Solid/DeviceInterface>
#include <Solid/Block>
#include <Solid/Device>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>
#include <Solid/StorageAccess>
#include <Solid/NetworkShare>
#include <Solid/OpticalDisc>
#include <Solid/Predicate>

#include <QDebug>

#include <QtCore/QMutexLocker>


namespace
{
bool isUsableVolume(const Solid::Device& dev)
{
    if (dev.is<Solid::StorageAccess>()) {
        if (dev.is<Solid::StorageVolume>() &&
                dev.parent().is<Solid::StorageDrive>() &&
                (dev.parent().as<Solid::StorageDrive>()->isRemovable() ||
                 dev.parent().as<Solid::StorageDrive>()->isHotpluggable())) {
            const Solid::StorageVolume* volume = dev.as<Solid::StorageVolume>();
            if (!volume->isIgnored() && volume->usage() == Solid::StorageVolume::FileSystem)
                return true;
        } else if (dev.is<Solid::NetworkShare>()) {
            return !dev.as<Solid::NetworkShare>()->url().isEmpty();
        }
    }

    // fallback
    return false;
}
}

using namespace Baloo;

RemovableMediaCache::RemovableMediaCache(QObject* parent)
    : QObject(parent)
{
    initCacheEntries();

    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)),
            this, SLOT(slotSolidDeviceAdded(QString)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)),
            this, SLOT(slotSolidDeviceRemoved(QString)));
}


RemovableMediaCache::~RemovableMediaCache()
{
}


void RemovableMediaCache::initCacheEntries()
{
    QList<Solid::Device> devices
        = Solid::Device::listFromQuery(QLatin1String("StorageVolume.usage=='FileSystem'"))
          + Solid::Device::listFromType(Solid::DeviceInterface::NetworkShare);
    Q_FOREACH (const Solid::Device& dev, devices) {
        createCacheEntry(dev);
    }
}

QList<RemovableMediaCache::Entry> RemovableMediaCache::allMedia() const
{
    return m_metadataCache.values();
}

RemovableMediaCache::Entry* RemovableMediaCache::createCacheEntry(const Solid::Device& dev)
{
    Entry entry(dev);
    if (dev.udi().isEmpty())
        return 0;

    auto it = m_metadataCache.insert(dev.udi(), entry);

    const Solid::StorageAccess* storage = dev.as<Solid::StorageAccess>();
    connect(storage, SIGNAL(accessibilityChanged(bool,QString)),
            this, SLOT(slotAccessibilityChanged(bool,QString)));
    //connect(storage, SIGNAL(teardownRequested(QString)),
    //        this, SLOT(slotTeardownRequested(QString)));
    return &it.value();
}

bool RemovableMediaCache::isEmpty() const
{
    return m_metadataCache.isEmpty();
}


void RemovableMediaCache::slotSolidDeviceAdded(const QString& udi)
{
    qDebug() << udi;
    Entry* e = createCacheEntry(Solid::Device(udi));
    if (e) {
        Q_EMIT deviceAdded(e);
    }
}


void RemovableMediaCache::slotSolidDeviceRemoved(const QString& udi)
{
    QHash< QString, Entry >::iterator it = m_metadataCache.find(udi);
    if (it != m_metadataCache.end()) {
        qDebug() << "Found removable storage volume for Baloo undocking:" << udi;
        Q_EMIT deviceRemoved(&it.value());
        m_metadataCache.erase(it);
    }
}


void RemovableMediaCache::slotAccessibilityChanged(bool accessible, const QString& udi)
{
    qDebug() << accessible << udi;
    Q_UNUSED(accessible);

    //
    // cache new mount path
    //
    Entry* entry = &m_metadataCache[udi];
    Q_ASSERT(entry != 0);
    Q_EMIT deviceAccessibilityChanged(entry);
}

RemovableMediaCache::Entry::Entry()
{
}

RemovableMediaCache::Entry::Entry(const Solid::Device& device)
    : m_device(device)
{
}

QString RemovableMediaCache::Entry::mountPath() const
{
    if (const Solid::StorageAccess* sa = m_device.as<Solid::StorageAccess>()) {
        return sa->filePath();
    } else {
        return QString();
    }
}

bool RemovableMediaCache::Entry::isMounted() const
{
    if (const Solid::StorageAccess* sa = m_device.as<Solid::StorageAccess>()) {
        return sa->isAccessible();
    } else {
        return false;
    }
}

bool RemovableMediaCache::Entry::isUsable() const
{
    bool usable = false;

    if (m_device.is<Solid::StorageVolume>()) {
        if (m_device.is<Solid::OpticalDisc>() || m_device.is<Solid::NetworkShare>()) {
            usable = false;
        }
        // FIXME: We need to check for removable as well?
        //else if (m_device.parent().as<Solid::StorageDrive>.isHotpluggable()) {
        //    usable = false;
        //}
    } else {
        usable = false;
    }

    if (const Solid::StorageAccess* sa = m_device.as<Solid::StorageAccess>()) {
        usable = sa->isAccessible();
    }

    return usable;
}

