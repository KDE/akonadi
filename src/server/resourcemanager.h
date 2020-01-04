/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_RESOURCEMANAGER_H
#define AKONADI_RESOURCEMANAGER_H

#include <QObject>

namespace Akonadi
{
namespace Server
{

class AkonadiServer;

/**
  Listens to agent instance added/removed signals and creates/removes
  the corresponding data in the database.
*/
class ResourceManager : public QObject
{
    Q_OBJECT

public:
    explicit ResourceManager(AkonadiServer &akonadi);

public Q_SLOTS:
    void addResourceInstance(const QString &name, const QStringList &capabilities);
    void removeResourceInstance(const QString &name);
    QStringList resourceInstances() const;

private:
    AkonadiServer &mAkonadi;
};

} // namespace Server
} // namespace Akonadi

#endif
