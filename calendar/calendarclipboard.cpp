/*
   Copyright (C) 2012 Sérgio Martins <iamsergio@gmail.com>

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

#include "calendarclipboard_p.h"
#include <kcalutils/dndfactory.h>
#include <kcalutils/icaldrag.h>

#include <KLocalizedString>
#include <KMessageBox>

#include <QApplication>
#include <QClipboard>

using namespace Akonadi;

CalendarClipboard::Private::Private(const Akonadi::CalendarBase::Ptr &calendar,
                                    Akonadi::IncidenceChanger *changer,
                                    CalendarClipboard *qq) : QObject(qq)
    , m_calendar(calendar)
    , m_changer(changer)
    , m_abortCurrentOperation(false)
    , q(qq)
{
    Q_ASSERT(m_calendar);
    if (!m_changer) {
        m_changer = new Akonadi::IncidenceChanger(this);
        m_changer->setHistoryEnabled(false);
        m_changer->setGroupwareCommunication(false);
    }

    m_dndfactory = new KCalUtils::DndFactory(m_calendar);

    connect(m_changer,
            SIGNAL(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(slotModifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));

    connect(m_changer,
            SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(slotDeleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)));
}

CalendarClipboard::Private::~Private()
{
    delete m_dndfactory;
}

void CalendarClipboard::Private::getIncidenceHierarchy(const KCalCore::Incidence::Ptr &incidence,
        QStringList &uids)
{
    // protecion against looping hierarchies
    if (incidence && !uids.contains(incidence->uid())) {
        KCalCore::Incidence::List immediateChildren = m_calendar->childIncidences(incidence->uid());

        foreach(const KCalCore::Incidence::Ptr &child, immediateChildren) {
            getIncidenceHierarchy(child, uids);
        }
        uids.append(incidence->uid());
    }
}

void CalendarClipboard::Private::cut(const KCalCore::Incidence::List &incidences)
{
    const bool result = m_dndfactory->copyIncidences(incidences);
    m_pendingChangeIds.clear();
    // Note: Don't use DndFactory::cutIncidences(), it doesn't use IncidenceChanger for deletion
    // we would loose async error handling and redo/undo features
    if (result) {
        Akonadi::Item::List items = m_calendar->itemList(incidences);
        const int result = m_changer->deleteIncidences(items);
        if (result == -1) {
            emit q->cutFinished(/**success=*/false, i18n("Error performing deletion."));
        } else {
            m_pendingChangeIds << result;
        }
    } else {
        emit q->cutFinished(/**success=*/false, i18n("Error performing copy."));
    }
}

void CalendarClipboard::Private::cut(const KCalCore::Incidence::Ptr &incidence)
{
    KCalCore::Incidence::List incidences;
    incidences << incidence;
    cut(incidences);
}

void CalendarClipboard::Private::makeChildsIndependent(const KCalCore::Incidence::Ptr &incidence)
{
    Q_ASSERT(incidence);
    const KCalCore::Incidence::List childs = m_calendar->childIncidences(incidence->uid());

    if (childs.isEmpty()) {
        cut(incidence);
    } else {
        m_pendingChangeIds.clear();
        m_abortCurrentOperation = false;
        foreach(const KCalCore::Incidence::Ptr &child, childs) {
            Akonadi::Item childItem = m_calendar->item(incidence);
            if (!childItem.isValid()) {
                emit q->cutFinished(/**success=*/ false, i18n("Can't find item: %1", childItem.id()));
                return;
            }

            KCalCore::Incidence::Ptr newIncidence(child->clone());
            newIncidence->setRelatedTo(QString());
            childItem.setPayload<KCalCore::Incidence::Ptr>(newIncidence);
            const int changeId = m_changer->modifyIncidence(childItem, /*originalPayload*/child);
            if (changeId == -1) {
                m_abortCurrentOperation = true;
                break;
            } else {
                m_pendingChangeIds << changeId;
            }
        }
        if (m_pendingChangeIds.isEmpty() && m_abortCurrentOperation) {
            emit q->cutFinished(/**success=*/false, i18n("Error while removing relations."));
        } // if m_pendingChangeIds isn't empty, we wait for all jobs to finish first.
    }
}

void CalendarClipboard::Private::slotModifyFinished(int changeId, const Akonadi::Item &item,
        IncidenceChanger::ResultCode resultCode,
        const QString &errorMessage)
{
    if (!m_pendingChangeIds.contains(changeId))
        return; // Not ours, someone else deleted something, not our business.

    m_pendingChangeIds.remove(changeId);
    const bool isLastChange = m_pendingChangeIds.isEmpty();

    Q_UNUSED(item);
    Q_UNUSED(errorMessage);
    if (m_abortCurrentOperation && isLastChange) {
        emit q->cutFinished(/**success=*/false, i18n("Error while removing relations."));
    } else if (!m_abortCurrentOperation) {
        if (resultCode == IncidenceChanger::ResultCodeSuccess) {
            if (isLastChange) {
                // All children are unparented, lets cut.
                Q_ASSERT(item.isValid() && item.hasPayload());
                cut(item.payload<KCalCore::Incidence::Ptr>());
            }
        } else {
            m_abortCurrentOperation = true;
        }
    }
}

void CalendarClipboard::Private::slotDeleteFinished(int changeId, const QVector<Akonadi::Item::Id> &ids,
        Akonadi::IncidenceChanger::ResultCode result,
        const QString &errorMessage)
{
    if (!m_pendingChangeIds.contains(changeId))
        return; // Not ours, someone else deleted something, not our business.

    m_pendingChangeIds.remove(changeId);

    Q_UNUSED(ids);
    if (result == IncidenceChanger::ResultCodeSuccess) {
        emit q->cutFinished(/**success=*/true, QString());
    } else {
        emit q->cutFinished(/**success=*/false, i18n("Error while deleting incidences: %1",
                                         errorMessage));
    }
}

CalendarClipboard::CalendarClipboard(const Akonadi::CalendarBase::Ptr &calendar,
                                     Akonadi::IncidenceChanger *changer,
                                     QObject *parent) : QObject(parent)
    , d(new Private(calendar, changer, this))
{

}

CalendarClipboard::~CalendarClipboard()
{
}

void CalendarClipboard::cutIncidence(const KCalCore::Incidence::Ptr &incidence,
                                     CalendarClipboard::Mode mode)
{
    const bool hasChildren = !d->m_calendar->childIncidences(incidence->uid()).isEmpty();
    if (mode == AskMode && hasChildren) {
        const int km = KMessageBox::questionYesNoCancel(0,
                       i18n("The item \"%1\" has sub-to-dos. "
                            "Do you want to cut just this item and "
                            "make all its sub-to-dos independent, or "
                            "cut the to-do with all its sub-to-dos?",
                            incidence->summary()),
                       i18n("KOrganizer Confirmation"),
                       KGuiItem(i18n("Cut Only This")),
                       KGuiItem(i18n("Cut All")));

        if (km == KMessageBox::Cancel) {
            emit cutFinished(/*success=*/true, QString());
            return;
        }
        mode = km == KMessageBox::Yes ? SingleMode : RecursiveMode;
    } else if (mode == AskMode) {
        mode = SingleMode; // Doesn't have children, don't ask
    }

    if (mode == SingleMode) {
        d->makeChildsIndependent(incidence);   // Will call d->cut(incidence) when it finishes.
    } else {
        QStringList uids;
        d->getIncidenceHierarchy(incidence, uids);
        Q_ASSERT(!uids.isEmpty());
        KCalCore::Incidence::List incidencesToCut;
        foreach(const QString &uid, uids) {
            KCalCore::Incidence::Ptr child = d->m_calendar->incidence(uid);
            if (child)
                incidencesToCut << child;
        }
        d->cut(incidencesToCut);
    }
}

bool CalendarClipboard::copyIncidence(const KCalCore::Incidence::Ptr &incidence,
                                      CalendarClipboard::Mode mode)
{
    const bool hasChildren = !d->m_calendar->childIncidences(incidence->uid()).isEmpty();
    if (mode == AskMode && hasChildren) {
        const int km = KMessageBox::questionYesNoCancel(0,
                       i18n("The item \"%1\" has sub-to-dos. "
                            "Do you want to copy just this item or "
                            "copy the to-do with all its sub-to-dos?",
                            incidence->summary()),
                       i18n("KOrganizer Confirmation"),
                       KGuiItem(i18n("Copy Only This")),
                       KGuiItem(i18n("Copy All")));
        if (km == KMessageBox::Cancel) {
            return true;
        }
        mode = km == KMessageBox::Yes ? SingleMode : RecursiveMode;
    } else if (mode == AskMode) {
        mode = SingleMode; // Doesn't have children, don't ask
    }

    KCalCore::Incidence::List incidencesToCopy;
    if (mode == SingleMode) {
        incidencesToCopy << incidence;
    } else {
        QStringList uids;
        d->getIncidenceHierarchy(incidence, uids);
        Q_ASSERT(!uids.isEmpty());
        foreach(const QString &uid, uids) {
            KCalCore::Incidence::Ptr child = d->m_calendar->incidence(uid);
            if (child)
                incidencesToCopy << child;
        }
    }

    return d->m_dndfactory->copyIncidences(incidencesToCopy);
}

bool CalendarClipboard::pasteAvailable() const
{
    return KCalUtils::ICalDrag::canDecode(QApplication::clipboard()->mimeData());
}
