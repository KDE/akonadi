/*
    SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_AGENTSEARCHINSTANCE_H
#define AKONADI_AGENTSEARCHINSTANCE_H

#include <QObject>
#include <QString>

#include <memory>

class QDBusServiceWatcher;
class OrgFreedesktopAkonadiAgentSearchInterface;

namespace Akonadi
{
namespace Server
{

class SearchTaskManager;
class AgentSearchInstance : public QObject
{
    Q_OBJECT
public:
    explicit AgentSearchInstance(const QString &id, SearchTaskManager &manager);
    ~AgentSearchInstance() override;

    bool init();

    void search(const QByteArray &searchId, const QString &query, qlonglong collectionId);

    OrgFreedesktopAkonadiAgentSearchInterface *interface() const;

private:
    QString mId;
    OrgFreedesktopAkonadiAgentSearchInterface *mInterface;
    std::unique_ptr<QDBusServiceWatcher> mServiceWatcher;
    SearchTaskManager &mManager;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_AGENTSEARCHINSTANCE_H
