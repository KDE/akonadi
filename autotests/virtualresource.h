/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef VIRTUALRESOURCE_H
#define VIRTUALRESOURCE_H

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/session.h>

namespace Akonadi {

/**
 * For testing only.
 *
 */
class AKONADI_EXPORT VirtualResource : public QObject
{
    Q_OBJECT
public:
    VirtualResource(const QString &name, QObject *parent = 0);
    ~VirtualResource();

    Akonadi::Collection createCollection(const Akonadi::Collection &collection);
    Akonadi::Collection createRootCollection(const Akonadi::Collection &collection);
    Akonadi::Item createItem(const Akonadi::Item &item, const Akonadi::Collection &parent);

    void reset();
private:
    Akonadi::Collection mRootCollection;
    QString mResourceName;
    Akonadi::Session *mSession;
};

}

#endif
