/*
  SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "fakeserverdata.h"

#include "itemfetchjob.h"
#include "collectionfetchjob.h"

#include <QTimer>

FakeServerData::FakeServerData(EntityTreeModel *model, FakeSession *session, FakeMonitor *monitor, QObject *parent)
    : QObject(parent),
      m_model(model),
      m_session(session),
      m_monitor(monitor),
      m_nextCollectionId(1),
      m_nextItemId(0),
      m_nextTagId(1)
{
    // can't use QueuedConnection here, because the Job might self-deleted before
    // the slot gets called
    connect(session, &FakeSession::jobAdded,
            [this](Akonadi::Job * job) {
                Collection::Id fetchColId = job->property("FetchCollectionId").toULongLong();
                QTimer::singleShot(0, [this, fetchColId]() {
                    jobAdded(fetchColId);
                });
            });
}

FakeServerData::FakeServerData(TagModel *model, FakeSession *session, FakeMonitor *monitor, QObject *parent)
    : QObject(parent),
      m_model(model),
      m_session(session),
      m_monitor(monitor),
      m_nextCollectionId(1),
      m_nextItemId(0),
      m_nextTagId(1)
{
    connect(session, &FakeSession::jobAdded,
            [this](Akonadi::Job * /*unused*/) {
                QTimer::singleShot(0, [this]() {
                    jobAdded();
                });
            });
}


void FakeServerData::setCommands(const QList<FakeAkonadiServerCommand *> &list)
{
    m_communicationQueue.clear();
    for (FakeAkonadiServerCommand *command : list) {
        m_communicationQueue << command;
    }
}

void FakeServerData::processNotifications()
{
    while (!m_communicationQueue.isEmpty()) {
        FakeAkonadiServerCommand::Type respondTo = m_communicationQueue.head()->respondTo();
        if (respondTo == FakeAkonadiServerCommand::Notification) {
            FakeAkonadiServerCommand *command = m_communicationQueue.dequeue();
            command->doCommand();
            delete command;
        } else {
            return;
        }
    }
}

void FakeServerData::jobAdded(qint64 fetchColId)
{
    returnEntities(fetchColId);
}

void FakeServerData::jobAdded()
{
    while (!m_communicationQueue.isEmpty() && m_communicationQueue.head()->respondTo() == FakeAkonadiServerCommand::RespondToTagFetch) {
        returnTags();
    }

    processNotifications();
}

void FakeServerData::returnEntities(Collection::Id fetchColId)
{
    if (!returnCollections(fetchColId)) {
        while (!m_communicationQueue.isEmpty() && m_communicationQueue.head()->respondTo() == FakeAkonadiServerCommand::RespondToItemFetch) {
            returnItems(fetchColId);
        }
    }

    processNotifications();
}

bool FakeServerData::returnCollections(Collection::Id fetchColId)
{
    if (m_communicationQueue.isEmpty()) {
        return true;
    }
    FakeAkonadiServerCommand::Type commType = m_communicationQueue.head()->respondTo();

    Collection fetchCollection = m_communicationQueue.head()->fetchCollection();

    if (commType == FakeAkonadiServerCommand::RespondToCollectionFetch
            && fetchColId == fetchCollection.id()) {
        FakeAkonadiServerCommand *command = m_communicationQueue.dequeue();
        command->doCommand();
        if (!m_communicationQueue.isEmpty()) {
            returnEntities(fetchColId);
        }
        delete command;
        return true;
    }
    return false;
}

void FakeServerData::returnItems(Item::Id fetchColId)
{
    FakeAkonadiServerCommand::Type commType = m_communicationQueue.head()->respondTo();

    if (commType == FakeAkonadiServerCommand::RespondToItemFetch) {
        FakeAkonadiServerCommand *command = m_communicationQueue.dequeue();
        command->doCommand();
        if (!m_communicationQueue.isEmpty()) {
            returnEntities(fetchColId);
        }
        delete command;
    }
}

void FakeServerData::returnTags()
{
    FakeAkonadiServerCommand::Type commType = m_communicationQueue.head()->respondTo();

    if (commType == FakeAkonadiServerCommand::RespondToTagFetch) {
        FakeAkonadiServerCommand *command = m_communicationQueue.dequeue();
        command->doCommand();
        delete command;
    }
}
