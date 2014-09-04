/*
   Copyright (C) 2011-2013 Sérgio Martins <iamsergio@gmail.com>

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

#include "etmcalendar.h"
#include "etmcalendar_p.h"
#include "blockalarmsattribute.h"
#include "calendarmodel_p.h"
#include "kcolumnfilterproxymodel_p.h"
#include "calfilterproxymodel_p.h"
#include "calfilterpartstatusproxymodel_p.h"
#include "utils_p.h"

#include <akonadi/item.h>
#include <akonadi/session.h>
#include <akonadi/collection.h>
#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/entitymimetypefiltermodel.h>
#include <akonadi/collectionfilterproxymodel.h>
#include <KSelectionProxyModel>
#include <KDescendantsProxyModel>

#include <QItemSelectionModel>
#include <QTreeView>

#include <recursivecollectionfilterproxymodel.h>
#include <mimetypechecker.h>

using namespace Akonadi;
using namespace KCalCore;

class CollectionFilter : public QSortFilterProxyModel
{
public:
    explicit CollectionFilter(QObject *parent = 0): QSortFilterProxyModel (parent) {};

    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        const Collection collection = sourceModel()->index(sourceRow, 0, sourceParent).data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        return collection.isValid();
    }

};

//TODO: implement batchAdding

ETMCalendarPrivate::ETMCalendarPrivate(ETMCalendar *qq) : CalendarBasePrivate(qq)
    , mETM(0)
    , mFilteredETM(0)
    , mCheckableProxyModel(0)
    , mCollectionProxyModel(0)
    , mCalFilterProxyModel(0)
    , mCalFilterPartStatusProxyModel(0)
    , mSelectionProxy(0)
    , mCollectionFilteringEnabled(true)
    , q(qq)
{
    mListensForNewItems = true;
}

void ETMCalendarPrivate::init()
{
    if (!mETM) {
        Akonadi::Session *session = new Akonadi::Session("ETMCalendar", q);
        Akonadi::ChangeRecorder *monitor = new Akonadi::ChangeRecorder(q);
        connect(monitor, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)),
                SLOT(onCollectionChanged(Akonadi::Collection,QSet<QByteArray>)));

        Akonadi::ItemFetchScope scope;
        scope.fetchFullPayload(true);
        scope.fetchAttribute<Akonadi::EntityDisplayAttribute>();

        monitor->setSession(session);
        monitor->setCollectionMonitored(Akonadi::Collection::root());
        monitor->fetchCollection(true);
        monitor->setItemFetchScope(scope);
        monitor->setAllMonitored(true);

        QStringList allMimeTypes;
        allMimeTypes << KCalCore::Event::eventMimeType() << KCalCore::Todo::todoMimeType()
                     << KCalCore::Journal::journalMimeType();

        foreach(const QString &mimetype, allMimeTypes) {
            monitor->setMimeTypeMonitored(mimetype, mMimeTypes.isEmpty() || mMimeTypes.contains(mimetype));
        }

        mETM = CalendarModel::create(monitor);
        mETM->setObjectName("ETM");
        mETM->setListFilter(Akonadi::CollectionFetchScope::Display);
    }

    setupFilteredETM();

    connect(q, SIGNAL(filterChanged()), SLOT(onFilterChanged()));

    connect(mETM.data(), SIGNAL(collectionPopulated(Akonadi::Collection::Id)),
            SLOT(onCollectionPopulated(Akonadi::Collection::Id)));
    connect(mETM.data(), SIGNAL(rowsInserted(QModelIndex,int,int)),
            SLOT(onRowsInserted(QModelIndex,int,int)));
    connect(mETM.data(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            SLOT(onDataChanged(QModelIndex,QModelIndex)));
    connect(mETM.data(), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
            SLOT(onRowsMoved(QModelIndex,int,int,QModelIndex,int)));
    connect(mETM.data(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
            SLOT(onRowsRemoved(QModelIndex,int,int)));

    connect(mFilteredETM, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            SLOT(onDataChangedInFilteredModel(QModelIndex,QModelIndex)));
    connect(mFilteredETM, SIGNAL(layoutChanged()),
            SLOT(onLayoutChangedInFilteredModel()));
    connect(mFilteredETM, SIGNAL(modelReset()),
            SLOT(onModelResetInFilteredModel()));
    connect(mFilteredETM, SIGNAL(rowsInserted(QModelIndex,int,int)),
            SLOT(onRowsInsertedInFilteredModel(QModelIndex,int,int)));
    connect(mFilteredETM, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            SLOT(onRowsAboutToBeRemovedInFilteredModel(QModelIndex,int,int)));

    loadFromETM();
}

void ETMCalendarPrivate::onCollectionChanged(const Akonadi::Collection &collection,
        const QSet<QByteArray> &attributeNames)
{
    Q_ASSERT(collection.isValid());
    // Is the collection changed to read-only, we update all Incidences
    if (attributeNames.contains("AccessRights")) {
        Akonadi::Item::List items = q->items();
        foreach(const Akonadi::Item &item, items) {
            if (item.storageCollectionId() == collection.id()) {
                KCalCore::Incidence::Ptr incidence = CalendarUtils::incidence(item);
                if (incidence)
                    incidence->setReadOnly(!(collection.rights() & Akonadi::Collection::CanChangeItem));
            }
        }
    }

    emit q->collectionChanged(collection, attributeNames);
}

void ETMCalendarPrivate::setupFilteredETM()
{
    // We're only interested in the CollectionTitle column
    KColumnFilterProxyModel *columnFilterProxy = new KColumnFilterProxyModel(this);
    columnFilterProxy->setSourceModel(mETM.data());
    columnFilterProxy->setVisibleColumn(CalendarModel::CollectionTitle);
    columnFilterProxy->setObjectName("Remove columns");

    CollectionFilter *mCollectionProxyModel = new CollectionFilter(this);
    mCollectionProxyModel->setObjectName("Only show collections");
    mCollectionProxyModel->setDynamicSortFilter(true);
    mCollectionProxyModel->setSourceModel(columnFilterProxy);

    // Keep track of selected items.
    QItemSelectionModel* selectionModel = new QItemSelectionModel(mCollectionProxyModel);
    selectionModel->setObjectName("Calendar Selection Model");

    // Make item selection work by means of checkboxes.
    mCheckableProxyModel = new CheckableProxyModel(this);
    mCheckableProxyModel->setSelectionModel(selectionModel);
    mCheckableProxyModel->setSourceModel(mCollectionProxyModel);
    mCheckableProxyModel->setObjectName("Add checkboxes");

    mSelectionProxy = new KSelectionProxyModel(selectionModel, /**parent=*/this);
    mSelectionProxy->setObjectName("Only show items of selected collection");
    mSelectionProxy->setFilterBehavior(KSelectionProxyModel::ChildrenOfExactSelection);
    mSelectionProxy->setSourceModel(mETM.data());

    mCalFilterProxyModel = new CalFilterProxyModel(this);
    mCalFilterProxyModel->setFilter(q->filter());
    mCalFilterProxyModel->setSourceModel(mSelectionProxy);
    mCalFilterProxyModel->setObjectName("KCalCore::CalFilter filtering");

    mCalFilterPartStatusProxyModel = new CalFilterPartStatusProxyModel(this);
    mCalFilterPartStatusProxyModel->setFilterVirtual(false);
    QList<KCalCore::Attendee::PartStat> blockedStatusList;
    blockedStatusList << KCalCore::Attendee::NeedsAction;
    blockedStatusList << KCalCore::Attendee::Declined;
    mCalFilterPartStatusProxyModel->setDynamicSortFilter(true);
    mCalFilterPartStatusProxyModel->setBlockedStatusList(blockedStatusList);
    mCalFilterPartStatusProxyModel->setSourceModel(mCalFilterProxyModel);
    mCalFilterPartStatusProxyModel->setObjectName("PartStatus filtering");

    mFilteredETM = new Akonadi::EntityMimeTypeFilterModel(this);
    mFilteredETM->setSourceModel(mCalFilterPartStatusProxyModel);
    mFilteredETM->setHeaderGroup(Akonadi::EntityTreeModel::ItemListHeaders);
    mFilteredETM->setSortRole(CalendarModel::SortRole);
    mFilteredETM->setObjectName("Show headers");

#ifdef AKONADI_CALENDAR_DEBUG_MODEL
    QTreeView *view = new QTreeView;
    view->setModel(mFilteredETM);
    view->show();
#endif
}

ETMCalendarPrivate::~ETMCalendarPrivate()
{
}

void ETMCalendarPrivate::loadFromETM()
{
    itemsAdded(itemsFromModel(mFilteredETM));
}

void ETMCalendarPrivate::clear()
{
    mCollectionMap.clear();
    mItemsByCollection.clear();

    itemsRemoved(mItemById.values());

    if (!mItemById.isEmpty()) {
        // This never happens
        kDebug() << "This shouldnt happen: !mItemById.isEmpty()";
        foreach(Akonadi::Item::Id id, mItemById.keys()) {
            kDebug() << "Id = " << id;
        }

        mItemById.clear();
        //Q_ASSERT(false); // TODO: discover why this happens
    }

    if (!mItemIdByUid.isEmpty()) {
        // This never happens
        kDebug() << "This shouldnt happen: !mItemIdByUid.isEmpty()";
        foreach(const QString &uid, mItemIdByUid.keys()) {
            kDebug() << "uid: " << uid;
        }
        mItemIdByUid.clear();
        //Q_ASSERT(false);
    }
    mParentUidToChildrenUid.clear();
    //m_virtualItems.clear();
}

Akonadi::Item::List ETMCalendarPrivate::itemsFromModel(const QAbstractItemModel *model,
        const QModelIndex &parentIndex,
        int start, int end)
{
    const int endRow = end >= 0 ? end : model->rowCount(parentIndex) - 1;
    Akonadi::Item::List items;
    int row = start;
    QModelIndex i = model->index(row, 0, parentIndex);
    while (row <= endRow) {
        const Akonadi::Item item = itemFromIndex(i);
        if (item.hasPayload<KCalCore::Incidence::Ptr>()) {
            items << item;
        } else {
            const QModelIndex childIndex = i.child(0, 0);
            if (childIndex.isValid()) {
                items << itemsFromModel(model, i);
            }
        }
        ++row;
        i = i.sibling(row, 0);
    }
    return items;
}

Akonadi::Collection::List ETMCalendarPrivate::collectionsFromModel(const QAbstractItemModel *model,
        const QModelIndex &parentIndex,
        int start, int end)
{
    const int endRow = end >= 0 ? end : model->rowCount(parentIndex) - 1;
    Akonadi::Collection::List collections;
    int row = start;
    QModelIndex i = model->index(row, 0, parentIndex);
    while (row <= endRow) {
        const Akonadi::Collection collection = collectionFromIndex(i);
        if (collection.isValid()) {
            collections << collection;
            QModelIndex childIndex = i.child(0, 0);
            if (childIndex.isValid()) {
                collections << collectionsFromModel(model, i);
            }
        }
        ++row;
        i = i.sibling(row, 0);
    }
    return collections;
}

Akonadi::Item ETMCalendarPrivate::itemFromIndex(const QModelIndex &idx)
{
    Akonadi::Item item = idx.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    item.setParentCollection(
        idx.data(Akonadi::EntityTreeModel::ParentCollectionRole).value<Akonadi::Collection>());
    return item;
}

void ETMCalendarPrivate::itemsAdded(const Akonadi::Item::List &items)
{
    if (!items.isEmpty()) {
        foreach(const Akonadi::Item &item, items) {
            internalInsert(item);
        }

        Akonadi::Collection::Id id = items.first().storageCollectionId();
        if (mPopulatedCollectionIds.contains(id)) {
            // If the collection isn't populated yet, it will be sent later
            // Saves some cpu cycles
            emit q->calendarChanged();
        }
    }
}

void ETMCalendarPrivate::itemsRemoved(const Akonadi::Item::List &items)
{
    foreach(const Akonadi::Item &item, items) {
        internalRemove(item);
    }
    emit q->calendarChanged();
}

Akonadi::Collection ETMCalendarPrivate::collectionFromIndex(const QModelIndex &index)
{
    return index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
}

void ETMCalendarPrivate::onRowsInserted(const QModelIndex &index,
                                        int start, int end)
{
    Akonadi::Collection::List collections = collectionsFromModel(mETM.data(), index,
                                            start, end);

    foreach(const Akonadi::Collection &collection, collections) {
        mCollectionMap[collection.id()] = collection;
    }

    if (!collections.isEmpty())
        emit q->collectionsAdded(collections);
}

void ETMCalendarPrivate::onCollectionPopulated(Akonadi::Collection::Id id)
{
    mPopulatedCollectionIds.insert(id);
    emit q->calendarChanged();
}

void ETMCalendarPrivate::onRowsRemoved(const QModelIndex &index, int start, int end)
{
    Akonadi::Collection::List collections = collectionsFromModel(mETM.data(), index, start, end);
    foreach(const Akonadi::Collection &collection, collections) {
        mCollectionMap.remove(collection.id());
    }

    if (!collections.isEmpty())
        emit q->collectionsRemoved(collections);
}

void ETMCalendarPrivate::onDataChanged(const QModelIndex &topLeft,
                                       const QModelIndex &bottomRight)
{
    // We only update collections, because items are handled in the filtered model
    Q_ASSERT(topLeft.row() <= bottomRight.row());
    const int endRow = bottomRight.row();
    for (int row = topLeft.row(); row <= endRow; row++) {
        const Akonadi::Collection col = collectionFromIndex(topLeft.sibling(row, 0));
        if (col.isValid()) {
            // Attributes might have changed, store the new collection and discard the old one
            mCollectionMap.insert(col.id(), col);
        }
        ++row;
    }
}

void ETMCalendarPrivate::onRowsMoved(const QModelIndex &sourceParent,
                                     int sourceStart,
                                     int sourceEnd,
                                     const QModelIndex &destinationParent,
                                     int destinationRow)
{
    //TODO
    Q_UNUSED(sourceParent);
    Q_UNUSED(sourceStart);
    Q_UNUSED(sourceEnd);
    Q_UNUSED(destinationParent);
    Q_UNUSED(destinationRow);
}

void ETMCalendarPrivate::onLayoutChangedInFilteredModel()
{
    clear();
    loadFromETM();
}

void ETMCalendarPrivate::onModelResetInFilteredModel()
{
    clear();
    loadFromETM();
}

void ETMCalendarPrivate::onDataChangedInFilteredModel(const QModelIndex &topLeft,
        const QModelIndex &bottomRight)
{
    Q_ASSERT(topLeft.row() <= bottomRight.row());
    const int endRow = bottomRight.row();
    QModelIndex i(topLeft);
    int row = i.row();
    while (row <= endRow) {
        const Akonadi::Item item = itemFromIndex(i);
        if (item.isValid() && item.hasPayload<KCalCore::Incidence::Ptr>())
            updateItem(item);

        ++row;
        i = i.sibling(row, topLeft.column());
    }

    emit q->calendarChanged();
}

void ETMCalendarPrivate::updateItem(const Akonadi::Item &item)
{
    Incidence::Ptr newIncidence = CalendarUtils::incidence(item);
    Q_ASSERT(newIncidence);
    Q_ASSERT(!newIncidence->uid().isEmpty());
    newIncidence->setCustomProperty("VOLATILE", "AKONADI-ID", QString::number(item.id()));
    IncidenceBase::Ptr existingIncidence = q->incidence(newIncidence->uid(), newIncidence->recurrenceId());

    if (!existingIncidence && !mItemById.contains(item.id())) {
        // We don't know about this one because it was discarded, for example because of not having DTSTART
        return;
    }

    mItemsByCollection.insert(item.storageCollectionId(), item);
    Akonadi::Item oldItem = mItemById.value(item.id());

    if (existingIncidence) {
        // We set the payload so that the internal incidence pointer and the one in mItemById stay the same
        Akonadi::Item updatedItem = item;
        updatedItem.setPayload<KCalCore::Incidence::Ptr>(existingIncidence.staticCast<KCalCore::Incidence>());
        mItemById.insert(item.id(), updatedItem);   // The item needs updating too, revision changed.

        // Check if RELATED-TO changed, updating parenting information
        handleParentChanged(newIncidence);
        *(existingIncidence.data()) = *(newIncidence.data());
    } else {
        mItemById.insert(item.id(), item);   // The item needs updating too, revision changed.
        // The item changed it's UID, update our maps, the Google resource changes the UID when we create incidences.
        handleUidChange(oldItem, item, newIncidence->instanceIdentifier());
    }
}

void ETMCalendarPrivate::onRowsInsertedInFilteredModel(const QModelIndex &index,
        int start, int end)
{
    itemsAdded(itemsFromModel(mFilteredETM, index, start, end));
}

void ETMCalendarPrivate::onRowsAboutToBeRemovedInFilteredModel(const QModelIndex &index,
        int start, int end)
{
    itemsRemoved(itemsFromModel(mFilteredETM, index, start, end));
}

void ETMCalendarPrivate::onFilterChanged()
{
    mCalFilterProxyModel->setFilter(q->filter());
}

ETMCalendar::ETMCalendar(QObject *parent) : CalendarBase(new ETMCalendarPrivate(this), parent)
{
    Q_D(ETMCalendar);
    d->init();
}

ETMCalendar::ETMCalendar(const QStringList &mimeTypes, QObject *parent) : CalendarBase(new ETMCalendarPrivate(this), parent)
{
    Q_D(ETMCalendar);
    d->mMimeTypes = mimeTypes;
    d->init();
}

ETMCalendar::ETMCalendar(ETMCalendar *other, QObject *parent)
    : CalendarBase(new ETMCalendarPrivate(this), parent)
{
    Q_D(ETMCalendar);

    CalendarModel *model = qobject_cast<Akonadi::CalendarModel*>(other->entityTreeModel());
    if (model) {
        d->mETM = model->weakPointer().toStrongRef();
    }

    d->init();
}

ETMCalendar::~ETMCalendar()
{
}

//TODO: move this up?
Akonadi::Collection ETMCalendar::collection(Akonadi::Collection::Id id) const
{
    Q_D(const ETMCalendar);
    return d->mCollectionMap.value(id);
}

bool ETMCalendar::hasRight(const QString &uid, Akonadi::Collection::Right right) const
{
    return hasRight(item(uid), right);
}

bool ETMCalendar::hasRight(const Akonadi::Item &item, Akonadi::Collection::Right right) const
{
    // if the users changes the rights, item.parentCollection()
    // can still have the old rights, so we use call collection()
    // which returns the updated one
    const Akonadi::Collection col = collection(item.storageCollectionId());
    return col.rights() & right;
}

QAbstractItemModel *ETMCalendar::model() const
{
    Q_D(const ETMCalendar);
    return d->mFilteredETM;
}

KCheckableProxyModel *ETMCalendar::checkableProxyModel() const
{
    Q_D(const ETMCalendar);
    return d->mCheckableProxyModel;
}

KCalCore::Alarm::List ETMCalendar::alarms(const KDateTime &from,
        const KDateTime &to,
        bool excludeBlockedAlarms) const
{
    Q_D(const ETMCalendar);
    KCalCore::Alarm::List alarmList;
    QHashIterator<Akonadi::Item::Id, Akonadi::Item> i(d->mItemById);
    while (i.hasNext()) {
        const Akonadi::Item item = i.next().value();

        BlockAlarmsAttribute *blockedAttr = 0;

        if (excludeBlockedAlarms) {
            // take the collection from m_collectionMap, because we need the up-to-date collection attrs
            Akonadi::Collection parentCollection = d->mCollectionMap.value(item.storageCollectionId());
            if (parentCollection.isValid() && parentCollection.hasAttribute<BlockAlarmsAttribute>()) {
                blockedAttr = parentCollection.attribute<BlockAlarmsAttribute>();
                if (blockedAttr->isAlarmTypeBlocked(Alarm::Audio) && blockedAttr->isAlarmTypeBlocked(Alarm::Display)
                        && blockedAttr->isAlarmTypeBlocked(Alarm::Email) && blockedAttr->isAlarmTypeBlocked(Alarm::Procedure))
                {
                    continue;
                }
            }
        }

        KCalCore::Incidence::Ptr incidence;
        if (item.isValid() && item.hasPayload<KCalCore::Incidence::Ptr>()) {
            incidence = KCalCore::Incidence::Ptr(item.payload<KCalCore::Incidence::Ptr>()->clone());
        } else {
            continue;
        }

        if (!incidence) {
            continue;
        }

        if (blockedAttr) {
            // Remove all blocked types of alarms
            Q_FOREACH(const KCalCore::Alarm::Ptr &alarm, incidence->alarms()) {
                if (blockedAttr->isAlarmTypeBlocked(alarm->type())) {
                    incidence->removeAlarm(alarm);
                }
            }
        }

        if (incidence->alarms().isEmpty()) {
            continue;
        }

        Alarm::List tmpList;
        if (incidence->recurs()) {
            appendRecurringAlarms(tmpList, incidence, from, to);
        } else {
            appendAlarms(tmpList, incidence, from, to);
        }

        // We need to tag them with the incidence uid in case
        // the caller will need it, because when we get out of
        // this scope the incidence will be destroyed.
        QVectorIterator<Alarm::Ptr> a(tmpList);
        while (a.hasNext()) {
            a.next()->setCustomProperty("ETMCalendar", "parentUid", incidence->uid());
        }
        alarmList += tmpList;
    }
    return alarmList;
}

Akonadi::EntityTreeModel *ETMCalendar::entityTreeModel() const
{
    Q_D(const ETMCalendar);
    return d->mETM.data();
}

void ETMCalendar::setCollectionFilteringEnabled(bool enable)
{
    Q_D(ETMCalendar);
    if (d->mCollectionFilteringEnabled != enable) {
        d->mCollectionFilteringEnabled = enable;
        if (enable) {
            d->mSelectionProxy->setSourceModel(d->mETM.data());
            QAbstractItemModel *oldModel = d->mCalFilterProxyModel->sourceModel();
            d->mCalFilterProxyModel->setSourceModel(d->mSelectionProxy);
            delete qobject_cast<KDescendantsProxyModel *>(oldModel);
        } else {
            KDescendantsProxyModel *flatner = new KDescendantsProxyModel(this);
            flatner->setSourceModel(d->mETM.data());
            d->mCalFilterProxyModel->setSourceModel(flatner);
        }
    }
}

bool ETMCalendar::collectionFilteringEnabled() const
{
    Q_D(const ETMCalendar);
    return d->mCollectionFilteringEnabled;
}

bool ETMCalendar::isLoaded() const
{
    Q_D(const ETMCalendar);

    if (!entityTreeModel()->isCollectionTreeFetched())
        return false;

    Akonadi::Collection::List collections = d->mCollectionMap.values();
    foreach (const Akonadi::Collection &collection, collections) {
        if (!entityTreeModel()->isCollectionPopulated(collection.id()))
            return false;
    }

    return true;
}

#include "moc_etmcalendar.cpp"
#include "moc_etmcalendar_p.cpp"
