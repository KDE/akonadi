/*
  SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QQueue>
#include <QSharedPointer>

#include "entitytreemodel.h"
#include "job.h"

#include "akonaditestfake_export.h"
#include "fakeakonadiservercommand.h"
#include "fakemonitor.h"
#include "fakesession.h"

using namespace Akonadi;

class AKONADITESTFAKE_EXPORT FakeServerData : public QObject
{
    Q_OBJECT
public:
    FakeServerData(EntityTreeModel *model, FakeSession *session, FakeMonitor *monitor, QObject *parent = nullptr);
    FakeServerData(TagModel *model, FakeSession *session, FakeMonitor *monitor, QObject *parent = nullptr);

    void setCommands(const QList<FakeAkonadiServerCommand *> &list);

    Collection::Id nextCollectionId() const
    {
        return m_nextCollectionId++;
    }
    Item::Id nextItemId() const
    {
        return m_nextItemId++;
    }
    Tag::Id nextTagId() const
    {
        return m_nextTagId++;
    }

    QAbstractItemModel *model() const
    {
        return m_model;
    }

    void processNotifications();

private Q_SLOTS:
    void jobAdded(qint64 fetchCollectionId);
    void jobAdded();

private:
    bool returnCollections(Collection::Id fetchColId);
    void returnItems(Item::Id fetchColId);
    void returnEntities(Collection::Id fetchColId);
    void returnTags();

private:
    QAbstractItemModel *m_model = nullptr;
    FakeSession *m_session = nullptr;
    FakeMonitor *m_monitor = nullptr;

    QList<FakeAkonadiServerCommand *> m_commandList;
    QQueue<FakeAkonadiServerCommand *> m_communicationQueue;

    mutable Collection::Id m_nextCollectionId;
    mutable Item::Id m_nextItemId;
    mutable Tag::Id m_nextTagId;
};

