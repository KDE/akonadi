/*
   SPDX-FileCopyrightText: 2009-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "collectionmaintenancepage.h"
#include "agentmanager.h"
#include "akonadiwidgets_debug.h"
#include "cachepolicy.h"
#include "core/collectionstatistics.h"
#include "indexpolicyattribute.h"
#include "monitor.h"
#include "servermanager.h"
#include "ui_collectionmaintenancepage.h"

#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

#include <KFormat>
#include <KLocalizedString>
#include <QCheckBox>
#include <QPushButton>

using namespace Akonadi;

class CollectionMaintenancePage::Private
{
public:
    Private()
    {
    }

    void slotReindexCollection()
    {
        if (currentCollection.isValid()) {
            // Don't allow to reindex twice.
            ui.reindexButton->setEnabled(false);

            const auto service = ServerManager::agentServiceName(ServerManager::Agent, QStringLiteral("akonadi_indexing_agent"));
            QDBusInterface indexingAgentIface(service, QStringLiteral("/"), QStringLiteral("org.freedesktop.Akonadi.Indexer"));
            if (indexingAgentIface.isValid()) {
                indexingAgentIface.call(QStringLiteral("reindexCollection"), static_cast<qlonglong>(currentCollection.id()));
                ui.indexedCountLbl->setText(i18n("Remember that indexing can take some minutes."));
            } else {
                qCWarning(AKONADIWIDGETS_LOG) << "indexer interface not valid";
            }
        }
    }

    void updateLabel(qint64 nbMail, qint64 nbUnreadMail, qint64 size)
    {
        ui.itemsCountLbl->setText(QString::number(qMax(0LL, nbMail)));
        ui.unreadItemsCountLbl->setText(QString::number(qMax(0LL, nbUnreadMail)));
        ui.folderSizeLbl->setText(KFormat().formatByteSize(qMax(0LL, size)));
    }

    Akonadi::Collection currentCollection;
    Akonadi::Monitor *monitor = nullptr;

    Ui::CollectionMaintenancePage ui;
};

CollectionMaintenancePage::CollectionMaintenancePage(QWidget *parent)
    : CollectionPropertiesPage(parent)
    , d(new Private)
{
    setObjectName(QStringLiteral("Akonadi::CollectionMaintenancePage"));
    setPageTitle(i18n("Maintenance"));
}

CollectionMaintenancePage::~CollectionMaintenancePage()
{
    delete d;
}

void CollectionMaintenancePage::init(const Collection &col)
{
    d->ui.setupUi(this);

    d->currentCollection = col;
    d->monitor = new Monitor(this);
    d->monitor->setObjectName(QStringLiteral("CollectionMaintenancePageMonitor"));
    d->monitor->setCollectionMonitored(col, true);
    d->monitor->fetchCollectionStatistics(true);
    connect(d->monitor, &Monitor::collectionStatisticsChanged, this, [this](Collection::Id /*unused*/, const CollectionStatistics &stats) {
        d->updateLabel(stats.count(), stats.unreadCount(), stats.size());
    });

    if (!col.isVirtual()) {
        const AgentInstance instance = Akonadi::AgentManager::self()->instance(col.resource());
        d->ui.folderTypeLbl->setText(instance.type().name());
    } else {
        d->ui.folderTypeLbl->hide();
        d->ui.filesLayout->labelForField(d->ui.folderTypeLbl)->hide();
    }

    connect(d->ui.reindexButton, &QPushButton::clicked, this, [this]() {
        d->slotReindexCollection();
    });

    // Check if the resource caches full payloads or at least has local storage
    // (so that the indexer can retrieve the payloads on demand)
    const auto resource = Akonadi::AgentManager::self()->instance(col.resource()).type();
    if (!col.cachePolicy().localParts().contains(QLatin1String("RFC822"))
        && resource.customProperties().value(QStringLiteral("HasLocalStorage"), QString()) != QLatin1String("true")) {
        d->ui.indexingGroup->hide();
    }
}

void CollectionMaintenancePage::load(const Collection &col)
{
    init(col);
    if (col.isValid()) {
        d->updateLabel(col.statistics().count(), col.statistics().unreadCount(), col.statistics().size());
        const auto attr = col.attribute<Akonadi::IndexPolicyAttribute>();
        const bool indexingWasEnabled(!attr || attr->indexingEnabled());
        d->ui.enableIndexingChkBox->setChecked(indexingWasEnabled);
        if (indexingWasEnabled) {
            const auto service = ServerManager::agentServiceName(ServerManager::Agent, QStringLiteral("akonadi_indexing_agent"));
            QDBusInterface indexingAgentIface(service, QStringLiteral("/"), QStringLiteral("org.freedesktop.Akonadi.Indexer"));
            if (indexingAgentIface.isValid()) {
                auto reply = indexingAgentIface.asyncCall(QStringLiteral("indexedItems"), static_cast<qint64>(col.id()));
                auto w = new QDBusPendingCallWatcher(reply, this);
                connect(w, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
                    QDBusPendingReply<qint64> reply = *w;
                    if (reply.isError()) {
                        d->ui.indexedCountLbl->setText(i18n("Error while retrieving indexed items count"));
                        qCWarning(AKONADIWIDGETS_LOG) << "Failed to retrieve indexed items count:" << reply.error().message();
                    } else {
                        d->ui.indexedCountLbl->setText(i18np("Indexed %1 item in this folder", "Indexed %1 items in this folder", reply.argumentAt<0>()));
                    }
                    w->deleteLater();
                });
                d->ui.indexedCountLbl->setText(i18n("Calculating indexed items..."));
            } else {
                qCDebug(AKONADIWIDGETS_LOG) << "Failed to obtain Indexer interface";
                d->ui.indexedCountLbl->hide();
            }
        } else {
            d->ui.indexedCountLbl->hide();
        }
    }
}

void CollectionMaintenancePage::save(Collection &collection)
{
    if (!collection.hasAttribute<Akonadi::IndexPolicyAttribute>() && d->ui.enableIndexingChkBox->isChecked()) {
        return;
    }

    auto attr = collection.attribute<Akonadi::IndexPolicyAttribute>(Akonadi::Collection::AddIfMissing);
    attr->setIndexingEnabled(d->ui.enableIndexingChkBox->isChecked());
}
