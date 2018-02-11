/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKONADI_AGENTSEARCHINSTANCE_H
#define AKONADI_AGENTSEARCHINSTANCE_H

#include <QObject>
#include <QString>

class QDBusServiceWatcher;
class OrgFreedesktopAkonadiAgentSearchInterface;

namespace Akonadi
{
namespace Server
{

class AgentSearchInstance : public QObject
{
    Q_OBJECT
public:
    explicit AgentSearchInstance(const QString &id);
    ~AgentSearchInstance() override;

    bool init();

    void search(const QByteArray &searchId, const QString &query, qlonglong collectionId);

    OrgFreedesktopAkonadiAgentSearchInterface *interface() const;

private Q_SLOTS:
    void serviceOwnerChanged(const QString &service, const QString &oldName, const QString &newName);

private:
    QString mId;
    OrgFreedesktopAkonadiAgentSearchInterface *mInterface;
    QDBusServiceWatcher *mServiceWatcher;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_AGENTSEARCHINSTANCE_H
