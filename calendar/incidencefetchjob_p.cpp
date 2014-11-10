/*
  Copyright (C) 2011 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Volker Krause <vkrause@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "incidencefetchjob_p.h"
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <kcalcore/event.h>
#include <kcalcore/journal.h>
#include <kcalcore/todo.h>

using namespace Akonadi;

Akonadi::IncidenceFetchJob::IncidenceFetchJob(QObject* parent): Job(parent), m_jobCount(0)
{
    m_mimeTypeChecker.addWantedMimeType("text/calendar");
}

Item::List Akonadi::IncidenceFetchJob::items() const
{
    return m_items;
}

void Akonadi::IncidenceFetchJob::doStart()
{
    CollectionFetchJob *job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, this);
    job->fetchScope().setContentMimeTypes(QStringList() << QLatin1String("text/calendar")
                                          << KCalCore::Event::eventMimeType()
                                          << KCalCore::Todo::todoMimeType()
                                          << KCalCore::Journal::journalMimeType());
    connect(job, SIGNAL(result(KJob*)), SLOT(collectionFetchResult(KJob*)));
}

void Akonadi::IncidenceFetchJob::collectionFetchResult(KJob* job)
{
    if (job->error())   // handled in base class
        return;
    CollectionFetchJob *fetch = qobject_cast<CollectionFetchJob*>(job);
    Q_ASSERT(fetch);

    if (fetch->collections().isEmpty()) {
        emitResult();
        return;
    }

    foreach(const Collection &col, fetch->collections()) {
        if (!m_mimeTypeChecker.isWantedCollection(col) || col.isVirtual())
            continue;
        ItemFetchJob *itemFetch = new ItemFetchJob(col, this);
        itemFetch->fetchScope().fetchFullPayload(true);
        connect(itemFetch, SIGNAL(result(KJob*)), SLOT(itemFetchResult(KJob*)));
        ++m_jobCount;
    }
}

void Akonadi::IncidenceFetchJob::itemFetchResult(KJob* job)
{
    if (job->error())   // handled in base class
        return;
    --m_jobCount;
    ItemFetchJob *fetch = qobject_cast<ItemFetchJob*>(job);
    foreach(const Item &item, fetch->items()) {
        if (!m_mimeTypeChecker.isWantedItem(item))
            continue;
        m_items.push_back(item);
    }

    if (m_jobCount <= 0)
        emitResult();
}

