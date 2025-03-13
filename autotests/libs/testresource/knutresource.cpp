/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "knutresource.h"
#include "knutresource_debug.h"
#include "settingsadaptor.h"
#include "xmlreader.h"
#include "xmlwriter.h"

#include "agentfactory.h"
#include "changerecorder.h"
#include "itemfetchscope.h"
#include "tagcreatejob.h"
#include <QDBusConnection>

#include <KLocalizedString>
#include <QFileDialog>

#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QUuid>

using namespace Akonadi;

KnutResource::KnutResource(const QString &id)
    : ResourceWidgetBase(id)
    , mWatcher(new QFileSystemWatcher(this))
    , mSettings(new KnutSettings())
{
    changeRecorder()->itemFetchScope().fetchFullPayload();
    changeRecorder()->fetchCollection(true);

    new SettingsAdaptor(mSettings);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"), mSettings, QDBusConnection::ExportAdaptors);
    connect(this, &KnutResource::reloadConfiguration, this, &KnutResource::load);
    connect(mWatcher, &QFileSystemWatcher::fileChanged, this, &KnutResource::load);
    load();
}

KnutResource::~KnutResource()
{
    delete mSettings;
}

void KnutResource::load()
{
    if (!mWatcher->files().isEmpty()) {
        mWatcher->removePaths(mWatcher->files());
    }

    // file loading
    QString fileName = mSettings->dataFile();
    if (fileName.isEmpty()) {
        Q_EMIT status(Broken, i18n("No data file selected."));
        return;
    }

    if (!QFile::exists(fileName)) {
        fileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kf6/akonadi_knut_resource/knut-template.xml"));
    }

    if (!mDocument.loadFile(fileName)) {
        Q_EMIT status(Broken, mDocument.lastError());
        return;
    }

    if (mSettings->fileWatchingEnabled()) {
        mWatcher->addPath(fileName);
    }

    Q_EMIT status(Idle, i18n("File '%1' loaded successfully.", fileName));
    synchronize();
}

void KnutResource::save()
{
    if (mSettings->readOnly()) {
        return;
    }
    const QString fileName = mSettings->dataFile();
    if (!mDocument.writeToFile(fileName)) {
        Q_EMIT error(mDocument.lastError());
        return;
    }
}

void KnutResource::configure(WId windowId)
{
    QString oldFile = mSettings->dataFile();
    if (oldFile.isEmpty()) {
        oldFile = QDir::homePath();
    }

    // TODO: Use windowId
    Q_UNUSED(windowId)
    const QString newFile =
        QFileDialog::getSaveFileName(nullptr,
                                     i18n("Select Data File"),
                                     QString(),
                                     QStringLiteral("*.xml |") + i18nc("Filedialog filter for Akonadi data file", "Akonadi Knut Data File"));

    if (newFile.isEmpty() || oldFile == newFile) {
        return;
    }

    mSettings->setDataFile(newFile);
    mSettings->save();
    load();

    Q_EMIT configurationDialogAccepted();
}

void KnutResource::retrieveCollections()
{
    const Collection::List collections = mDocument.collections();
    collectionsRetrieved(collections);
    const Tag::List tags = mDocument.tags();
    for (const Tag &tag : tags) {
        auto createjob = new TagCreateJob(tag);
        createjob->setMergeIfExisting(true);
    }
}

void KnutResource::retrieveItems(const Akonadi::Collection &collection)
{
    Item::List items = mDocument.items(collection, false);
    if (!mDocument.lastError().isEmpty()) {
        cancelTask(mDocument.lastError());
        return;
    }

    itemsRetrieved(items);
}

#ifdef DO_IT_THE_OLD_WAY
bool KnutResource::retrieveItem(const Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts)

    const QDomElement itemElem = mDocument.itemElementByRemoteId(item.remoteId());
    if (itemElem.isNull()) {
        cancelTask(i18n("No item found for remoteid %1", item.remoteId()));
        return false;
    }

    Item i = XmlReader::elementToItem(itemElem, true);
    i.setId(item.id());
    itemRetrieved(i);
    return true;
}
#endif

bool KnutResource::retrieveItems(const Item::List &items, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts)

    Item::List results;
    results.reserve(items.size());
    for (const auto &item : items) {
        const QDomElement itemElem = mDocument.itemElementByRemoteId(item.remoteId());
        if (itemElem.isNull()) {
            cancelTask(i18n("No item found for remoteid %1", item.remoteId()));
            return false;
        }

        Item i = XmlReader::elementToItem(itemElem, true);
        i.setParentCollection(item.parentCollection());
        i.setId(item.id());
        results.push_back(i);
    }

    itemsRetrieved(results);
    return true;
}

void KnutResource::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    QDomElement parentElem = mDocument.collectionElementByRemoteId(parent.remoteId());
    if (parentElem.isNull()) {
        Q_EMIT error(i18n("Parent collection not found in DOM tree."));
        changeProcessed();
        return;
    }

    Collection c(collection);
    c.setRemoteId(QUuid::createUuid().toString());
    if (XmlWriter::writeCollection(c, parentElem).isNull()) {
        Q_EMIT error(i18n("Unable to write collection."));
        changeProcessed();
    } else {
        save();
        changeCommitted(c);
    }
}

void KnutResource::collectionChanged(const Akonadi::Collection &collection)
{
    QDomElement oldElem = mDocument.collectionElementByRemoteId(collection.remoteId());
    if (oldElem.isNull()) {
        Q_EMIT error(i18n("Modified collection not found in DOM tree."));
        changeProcessed();
        return;
    }

    QDomElement newElem;
    newElem = XmlWriter::collectionToElement(collection, mDocument.document());
    // move all items/collections over to the new node
    const QDomNodeList children = oldElem.childNodes();
    const int numberOfChildren = children.count();
    for (int i = 0; i < numberOfChildren; ++i) {
        const QDomElement child = children.at(i).toElement();
        qCDebug(KNUTRESOURCE_LOG) << "reparenting " << child.tagName() << child.attribute(QStringLiteral("rid"));
        if (child.isNull()) {
            continue;
        }
        if (child.tagName() == QLatin1StringView("item") || child.tagName() == QLatin1StringView("collection")) {
            newElem.appendChild(child); // reparents
            --i; // children, despite being const is modified by the reparenting
        }
    }
    oldElem.parentNode().replaceChild(newElem, oldElem);
    save();
    changeCommitted(collection);
}

void KnutResource::collectionRemoved(const Akonadi::Collection &collection)
{
    const QDomElement colElem = mDocument.collectionElementByRemoteId(collection.remoteId());
    if (colElem.isNull()) {
        Q_EMIT error(i18n("Deleted collection not found in DOM tree."));
        changeProcessed();
        return;
    }

    colElem.parentNode().removeChild(colElem);
    save();
    changeProcessed();
}

void KnutResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    QDomElement parentElem = mDocument.collectionElementByRemoteId(collection.remoteId());
    if (parentElem.isNull()) {
        Q_EMIT error(i18n("Parent collection '%1' not found in DOM tree.", collection.remoteId()));
        changeProcessed();
        return;
    }

    Item i(item);
    i.setRemoteId(QUuid::createUuid().toString());
    if (XmlWriter::writeItem(i, parentElem).isNull()) {
        Q_EMIT error(i18n("Unable to write item."));
        changeProcessed();
    } else {
        save();
        changeCommitted(i);
    }
}

void KnutResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts)

    const QDomElement oldElem = mDocument.itemElementByRemoteId(item.remoteId());
    if (oldElem.isNull()) {
        Q_EMIT error(i18n("Modified item not found in DOM tree."));
        changeProcessed();
        return;
    }

    const QDomElement newElem = XmlWriter::itemToElement(item, mDocument.document());
    oldElem.parentNode().replaceChild(newElem, oldElem);
    save();
    changeCommitted(item);
}

void KnutResource::itemRemoved(const Akonadi::Item &item)
{
    const QDomElement itemElem = mDocument.itemElementByRemoteId(item.remoteId());
    if (itemElem.isNull()) {
        Q_EMIT error(i18n("Deleted item not found in DOM tree."));
        changeProcessed();
        return;
    }

    itemElem.parentNode().removeChild(itemElem);
    save();
    changeProcessed();
}

void KnutResource::itemMoved(const Item &item, const Collection &collectionSource, const Collection &collectionDestination)
{
    const QDomElement oldElem = mDocument.itemElementByRemoteId(item.remoteId());
    if (oldElem.isNull()) {
        qCWarning(KNUTRESOURCE_LOG) << "Moved item not found in DOM tree";
        changeProcessed();
        return;
    }

    QDomElement sourceParentElem = mDocument.collectionElementByRemoteId(collectionSource.remoteId());
    if (sourceParentElem.isNull()) {
        Q_EMIT error(i18n("Parent collection '%1' not found in DOM tree.", collectionSource.remoteId()));
        changeProcessed();
        return;
    }

    QDomElement destParentElem = mDocument.collectionElementByRemoteId(collectionDestination.remoteId());
    if (destParentElem.isNull()) {
        Q_EMIT error(i18n("Parent collection '%1' not found in DOM tree.", collectionDestination.remoteId()));
        changeProcessed();
        return;
    }
    QDomElement itemElem = mDocument.itemElementByRemoteId(item.remoteId());
    if (itemElem.isNull()) {
        Q_EMIT error(i18n("No item found for remoteid %1", item.remoteId()));
    }

    sourceParentElem.removeChild(itemElem);
    destParentElem.appendChild(itemElem);

    if (XmlWriter::writeItem(item, destParentElem).isNull()) {
        Q_EMIT error(i18n("Unable to write item."));
    } else {
        save();
    }
    changeProcessed();
}

QSet<qint64> KnutResource::parseQuery(const QString &queryString)
{
    QSet<qint64> resultSet;
    Akonadi::SearchQuery query = Akonadi::SearchQuery::fromJSON(queryString.toLatin1());
    const QList<SearchTerm> subTerms = query.term().subTerms();
    for (const Akonadi::SearchTerm &term : subTerms) {
        if (term.key() == QLatin1StringView("resource")) {
            resultSet << term.value().toInt();
        }
    }
    return resultSet;
}

void KnutResource::search(const QString &query, const Collection &collection)
{
    Q_UNUSED(collection)
    const QList<qint64> result = parseQuery(query).values().toVector();
    qCDebug(KNUTRESOURCE_LOG) << "KNUT QUERY:" << query;
    qCDebug(KNUTRESOURCE_LOG) << "KNUT RESOURCE:" << result;
    searchFinished(result, Akonadi::AgentSearchInterface::Uid);
}

void KnutResource::addSearch(const QString &query, const QString &queryLanguage, const Collection &resultCollection)
{
    Q_UNUSED(query)
    Q_UNUSED(queryLanguage)
    Q_UNUSED(resultCollection)
    qCDebug(KNUTRESOURCE_LOG) << "addSearch: query=" << query << ", queryLanguage=" << queryLanguage << ", resultCollection=" << resultCollection.id();
}

void KnutResource::removeSearch(const Collection &resultCollection)
{
    Q_UNUSED(resultCollection)
    qCDebug(KNUTRESOURCE_LOG) << "removeSearch:" << resultCollection.id();
}

AKONADI_RESOURCE_MAIN(KnutResource)

#include "moc_knutresource.cpp"
