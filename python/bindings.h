/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <nde@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

// Make "signals:", "slots:" visible as access specifiers
#define QT_ANNOTATE_ACCESS_SPECIFIER(a) __attribute__((annotate(#a)))

#include "pysidesignal.h"

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QMetaMethod>
#include <QModelIndex>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QVariant>

// types in global namespace

#include <Akonadi/ColorProxyModel>

// /core/attributes types

#include <Akonadi/Attribute>

#include <Akonadi/CollectionAnnotationsAttribute>
#include <Akonadi/CollectionColorAttribute>
#include <Akonadi/CollectionIdentificationAttribute>
#include <Akonadi/CollectionQuotaAttribute>

#include <Akonadi/EntityAnnotationsAttribute>
#include <Akonadi/EntityDeletedAttribute>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/EntityHiddenAttribute>

#include <Akonadi/FavoriteCollectionAttribute>

#include <Akonadi/IndexPolicyAttribute>

#include <Akonadi/PersistentSearchAttribute>

#include <Akonadi/SpecialCollectionAttribute>

#include <Akonadi/TagAttribute>

// /core/jobs types

#include <Akonadi/AgentInstanceCreateJob>

#include <Akonadi/CollectionAttributesSynchronizationJob>
#include <Akonadi/CollectionCopyJob>
#include <Akonadi/CollectionCreateJob>
#include <Akonadi/CollectionDeleteJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/CollectionMoveJob>
#include <Akonadi/CollectionStatisticsJob>

#include <Akonadi/ItemCopyJob>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemMoveJob>
#include <Akonadi/ItemSearchJob>

#include <Akonadi/Job>

#include <Akonadi/RecursiveItemFetchJob>

#include <Akonadi/ResourceSynchronizationJob>

#include <Akonadi/SearchCreateJob>

#include <Akonadi/SpecialCollectionsDiscoveryJob>
#include <Akonadi/SpecialCollectionsRequestJob>

#include <Akonadi/TagCreateJob>
#include <Akonadi/TagDeleteJob>
#include <Akonadi/TagFetchJob>
#include <Akonadi/TagModifyJob>

#include <Akonadi/TransactionJobs>
#include <Akonadi/TransactionSequence>

#include <Akonadi/TrashJob>
#include <Akonadi/TrashRestoreJob>

// Trying to bind these two classes causes an error because of their direct use of the private header <private/protocol_p.h>
// that Shiboken can't find when generating the bindings.

// Since these two classes are not used a lot in modern KDE PIM's ecosystem, they will not be binded.

// #include <Akonadi/LinkJob>
// #include <Akonadi/UnLinkJob>

// /core/models types

#include <Akonadi/AgentFilterProxyModel>
#include <Akonadi/AgentInstanceFilterProxyModel>
#include <Akonadi/AgentInstanceModel>
#include <Akonadi/AgentTypeModel>

#include <Akonadi/CollectionFilterProxyModel>

#include <Akonadi/EntityMimeTypeFilterModel>
#include <Akonadi/EntityOrderProxyModel>
#include <Akonadi/EntityRightsFilterModel>
#include <Akonadi/EntityTreeModel>

#include <Akonadi/FavoriteCollectionsModel>

#include <Akonadi/RecursiveCollectionFilterProxyModel>

#include <Akonadi/SelectionProxyModel>

#include <Akonadi/StatisticsProxyModel>

#include <Akonadi/TagModel>

#include <Akonadi/TrashFilterProxyModel>

// /core types

#include <Akonadi/AbstractDifferencesReporter>

#include <Akonadi/AccountActivitiesAbstract>

#include <Akonadi/AgentConfigurationBase>
#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>
#include <Akonadi/AgentType>

#include <Akonadi/AttributeFactory>

#include <Akonadi/CachePolicy>

#include <Akonadi/ChangeNotification>
#include <Akonadi/ChangeRecorder>

#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/CollectionPathResolver>
#include <Akonadi/CollectionStatistics>
#include <Akonadi/CollectionUtils>

#include <Akonadi/Control>

#include <Akonadi/DifferencesAlgorithmInterface>

#include <Akonadi/ExceptionBase>

#include <Akonadi/GidExtractorInterface>

#include <Akonadi/Item>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemMonitor>
#include <Akonadi/ItemSync>

#include <Akonadi/MimeTypeChecker>

#include <Akonadi/Monitor>

#include <Akonadi/NotificationSubscriber>

#include <Akonadi/PartFetcher>

#include <Akonadi/SearchQuery>

#include <Akonadi/ServerManager>

#include <Akonadi/Session>

#include <Akonadi/SpecialCollections>

#include <Akonadi/Tag>
#include <Akonadi/TagCache>
#include <Akonadi/TagFetchScope>

// This causes an error in modules built on top of this one
// due to the unfortunate #include "jobs/job.h" there...
// Since it's mostly unused anyway, let's not expose it
// #include <Akonadi/TagSync>

#include <Akonadi/TrashSettings>
