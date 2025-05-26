// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-FileCopyrightText: 2022 Claudio Cambra <claudio.cambra@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "colorproxymodel.h"

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>
#include <Akonadi/AttributeFactory>
#include <Akonadi/CollectionColorAttribute>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/CollectionUtils>
#include <Akonadi/EntityDisplayAttribute>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QFont>
#include <QRandomGenerator>

using namespace Qt::StringLiterals;

namespace
{
static bool hasCompatibleMimeTypes(const Akonadi::Collection &collection)
{
    static QStringList goodMimeTypes = {
        u"text/calendar"_s,
        u"application/x-vnd.akonadi.calendar.event"_s,
        u"application/x-vnd.akonadi.calendar.todo"_s,
        u"text/directory"_s,
        u"application/x-vnd.kde.contactgroup"_s,
        u"application/x-vnd.akonadi.calendar.journal"_s,
    };

    for (int i = 0; i < goodMimeTypes.count(); ++i) {
        if (collection.contentMimeTypes().contains(goodMimeTypes.at(i))) {
            return true;
        }
    }

    return false;
}
}

ColorProxyModel::ColorProxyModel(QObject *parent)
    : Akonadi::CollectionFilterProxyModel(parent)
{
    // Needed to read colorattribute of collections for incidence colors
    Akonadi::AttributeFactory::registerAttribute<Akonadi::CollectionColorAttribute>();
}

QVariant ColorProxyModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (role == Qt::DecorationRole) {
        const Akonadi::Collection collection = Akonadi::CollectionUtils::fromIndex(index);

        if (hasCompatibleMimeTypes(collection)) {
            if (collection.hasAttribute<Akonadi::EntityDisplayAttribute>() && !collection.attribute<Akonadi::EntityDisplayAttribute>()->iconName().isEmpty()) {
                return collection.attribute<Akonadi::EntityDisplayAttribute>()->iconName();
            }
        }
    } else if (role == Qt::FontRole) {
        const Akonadi::Collection collection = Akonadi::CollectionUtils::fromIndex(index);
        if (!collection.contentMimeTypes().isEmpty() && collection.id() == m_standardCollectionId && collection.rights() & Akonadi::Collection::CanCreateItem) {
            auto font = qvariant_cast<QFont>(QSortFilterProxyModel::data(index, Qt::FontRole));
            font.setBold(true);
            return font;
        }
    } else if (role == Qt::DisplayRole) {
        const Akonadi::Collection collection = Akonadi::CollectionUtils::fromIndex(index);
        const Akonadi::Collection::Id colId = collection.id();
        const Akonadi::AgentInstance instance = Akonadi::AgentManager::self()->instance(collection.resource());

        if (!instance.isOnline() && !collection.isVirtual()) {
            return i18nc("@item this is the default calendar", "%1 (Offline)", collection.displayName());
        }
        if (colId == m_standardCollectionId) {
            return i18nc("@item this is the default calendar", "%1 (Default)", collection.displayName());
        }
    } else if (role == Qt::BackgroundRole) {
        const auto color = getCollectionColor(Akonadi::CollectionUtils::fromIndex(index));
        // Otherwise QML will get black
        if (color.isValid()) {
            return color;
        } else {
            return {};
        }
    } else if (role == isResource) {
        return Akonadi::CollectionUtils::isResource(Akonadi::CollectionUtils::fromIndex(index));
    }

    return QSortFilterProxyModel::data(index, role);
}

Qt::ItemFlags ColorProxyModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | QSortFilterProxyModel::flags(index);
}

QHash<int, QByteArray> ColorProxyModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = QSortFilterProxyModel::roleNames();
    roleNames[Qt::CheckStateRole] = QByteArrayLiteral("checkState");
    roleNames[Qt::BackgroundRole] = QByteArrayLiteral("collectionColor");
    roleNames[isResource] = QByteArrayLiteral("isResource");
    return roleNames;
}

QColor ColorProxyModel::getCollectionColor(Akonadi::Collection collection) const
{
    const auto id = collection.id();
    const auto supportsMimeType = collection.contentMimeTypes().contains("application/x-vnd.akonadi.calendar.event"_L1)
        || collection.contentMimeTypes().contains("application/x-vnd.akonadi.calendar.todo"_L1)
        || collection.contentMimeTypes().contains("application/x-vnd.akonadi.calendar.journal"_L1)
        || collection.contentMimeTypes().contains("text/directory"_L1) || collection.contentMimeTypes().contains("application/x-vnd.kde.contactgroup"_L1);

    if (!supportsMimeType) {
        return {};
    }

    if (colorCache.contains(id)) {
        return colorCache[id];
    }

    if (collection.hasAttribute<Akonadi::CollectionColorAttribute>()) {
        const auto colorAttr = collection.attribute<Akonadi::CollectionColorAttribute>();
        if (colorAttr && colorAttr->color().isValid()) {
            colorCache[id] = colorAttr->color();
            return colorAttr->color();
        }
    }

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup resourcesColorsConfig(config, QStringLiteral("Resources Colors"));
    const QStringList colorKeyList = resourcesColorsConfig.keyList();

    QColor color;
    for (const QString &key : colorKeyList) {
        if (key.toLongLong() == id) {
            color = resourcesColorsConfig.readEntry(key, QColor("blue"));
        }
    }

    if (!color.isValid()) {
        color.setRgb(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256));
        colorCache[id] = color;
    }

    auto colorAttr = collection.attribute<Akonadi::CollectionColorAttribute>(Akonadi::Collection::AddIfMissing);
    colorAttr->setColor(color);

    auto modifyJob = new Akonadi::CollectionModifyJob(collection);
    connect(modifyJob, &Akonadi::CollectionModifyJob::result, this, [](KJob *job) {
        if (job->error()) {
            qWarning() << "Error occurred modifying collection color: " << job->errorString();
        }
    });

    return color;
}

QColor ColorProxyModel::color(Akonadi::Collection::Id collectionId) const
{
    return colorCache[collectionId];
}

void ColorProxyModel::setColor(Akonadi::Collection::Id collectionId, const QColor &color)
{
    colorCache[collectionId] = color;
}

void ColorProxyModel::setStandardCollectionId(Akonadi::Collection::Id standardCollectionId)
{
    m_standardCollectionId = standardCollectionId;
}

bool ColorProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
    const auto leftHasChildren = sourceModel()->hasChildren(sourceLeft);
    const auto rightHasChildren = sourceModel()->hasChildren(sourceRight);
    if (leftHasChildren && !rightHasChildren) {
        return false;
    } else if (!leftHasChildren && rightHasChildren) {
        return true;
    }

    return Akonadi::CollectionFilterProxyModel::lessThan(sourceLeft, sourceRight);
}

#include "moc_colorproxymodel.cpp"
