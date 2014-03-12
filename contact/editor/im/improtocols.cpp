/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "improtocols.h"

#include <kiconloader.h>
#include <kservicetypetrader.h>

IMProtocols *IMProtocols::mSelf = 0;

IMProtocols::IMProtocols()
{
    KIconLoader::global()->addAppDir(QLatin1String("akonadi/contact"));

    const QList<KPluginInfo> list = KPluginInfo::fromServices(KServiceTypeTrader::self()->query(QString::fromLatin1("KABC/IMProtocol")));

    // sort the protocol information by user visible name
    QMap<QString, KPluginInfo> sortingMap;
    foreach (const KPluginInfo &info, list) {
        sortingMap.insert(info.name(), info);

        mPluginInfos.insert(info.property(QLatin1String("X-KDE-InstantMessagingKABCField")).toString(), info);
    }

    QMapIterator<QString, KPluginInfo> it(sortingMap);
    while (it.hasNext()) {
        it.next();
        mSortedProtocols.append(it.value().property(QLatin1String("X-KDE-InstantMessagingKABCField")).toString());
    }
}

IMProtocols::~IMProtocols()
{
}

IMProtocols *IMProtocols::self()
{
    if (!mSelf) {
        mSelf = new IMProtocols;
    }

    return mSelf;
}

QStringList IMProtocols::protocols() const
{
    return mSortedProtocols;
}

QString IMProtocols::name(const QString &protocol) const
{
    if (!mPluginInfos.contains(protocol)) {
        return QString();
    }

    return mPluginInfos.value(protocol).name();
}

QString IMProtocols::icon(const QString &protocol) const
{
    if (!mPluginInfos.contains(protocol)) {
        return QString();
    }

    return mPluginInfos.value(protocol).icon();
}
