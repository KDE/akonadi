/*
    Copyright (c) 2011 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2011 Stephen Kelly <steveire@gmail.com>

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

#ifndef AKONADI_CHANGEMEDIATOR_P_H
#define AKONADI_CHANGEMEDIATOR_P_H

#include <QList>
#include <QObject>

namespace Akonadi
{

class Job;
class JobPrivate;

class Collection;
class Item;
class Tag;

class ChangeMediator : public QObject
{
    Q_OBJECT
public:
    static ChangeMediator *instance();

    static void registerMonitor(QObject *monitor);
    static void unregisterMonitor(QObject *monitor);

    static void invalidateCollection(const Akonadi::Collection &collection);
    static void invalidateItem(const Akonadi::Item &item);
    static void invalidateTag(const Akonadi::Tag &tag);

protected:
    explicit ChangeMediator(QObject *parent = nullptr);

    QList<QObject *> m_monitors;
};

}

#endif
