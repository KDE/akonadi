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

#ifndef AKONADI_CONTACT_IMPROTOCOLS_H
#define AKONADI_CONTACT_IMPROTOCOLS_H

#include <kplugininfo.h>

#include "akonadi-contact_export.h"

class AKONADI_CONTACT_EXPORT IMProtocols
{
  public:
    ~IMProtocols();

    static IMProtocols* self();

    /**
     * Returns the protocol identifiers in a sorted order.
     */
    QStringList protocols() const;

    QString name( const QString &protocol ) const;
    QString icon( const QString &protocol ) const;

  private:
    IMProtocols();

    static IMProtocols* mSelf;

    QMap<QString, KPluginInfo> mPluginInfos;
    QStringList mSortedProtocols;
};

#endif
