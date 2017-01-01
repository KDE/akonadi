/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "knutresource.h"
#include "knutresource_debug.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "xmlwriter.h"
#include "xmlreader.h"
#include <searchquery.h>

#include <QUrl>
#include <agentfactory.h>
#include <changerecorder.h>
#include <collection.h>
#include <KDBusConnectionPool>
#include <item.h>
#include <itemfetchscope.h>
#include <tagcreatejob.h>

#include <QFileDialog>
#include <KLocalizedString>

#include <QFile>
#include <QFileSystemWatcher>
#include <QDir>
#include <QUuid>
#include <QStandardPaths>
#include <QDebug>
#include <QFileDialog>

using namespace Akonadi;

KnutResource::KnutResource(const QString &id)
    : ResourceBase(id)
    , mWatcher(new QFileSystemWatcher(this))
    , mSettings(new KnutSettings())
{
    changeRecorder()->itemFetchScope().fetchFullPayload();
    changeRecorder()->fetchCollection(true);

    new SettingsAdaptor(mSettings);
    KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/Settings"),
            mSettings, QDBusConnection::ExportAdaptors);
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
        emit status(Broken, i18n("No data file selected."));
        return;
    }

    if (!QFile::exists(fileName)) {
        fileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kf5/akonadi_knut_resource/knut-template.xml"));
    }

    if (!mDocument.loadFile(fileName)) {
        emit status(Broken, mDocument.lastError());
        return;
    }

    if (mSettings->fileWatchingEnabled()) {
        mWatcher->addPath(fileName);
    }

    emit status(Idle, i18n("File '%1' loaded successfully.", fileName));
    synchronize();
}

void KnutResource::save()
{
    if (mSettings->readOnly()) {
        return;
    }
    const QString fileName = mSettings->dataFile();
    if (!mDocument.writeToFile(fileName)) {
        emit error(mDocument.lastError());
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
    Q_UNUSED(windowId);
    const QString newFile = QFileDialog::getSaveFileName(
        Q_NULLPTR, i18n("Select Data File"), QString(),
        QStringLiteral("*.xml |") + i18nc("Filedialog filter for Akonadi data file", "Akonadi Knut Data File"));

    if (newFile.isEmpty() || oldFile == newFile) {
        return;
    }

    mSettings->setDataFile(newFile);
    mSettings->save();
    load();

    emit configurationDialogAccepted();
}

void KnutResource::retrieveCollections()
{
    const Collection::List collections = mDocument.collections();
    collectionsRetrieved(collections);
    const Tag::List tags = mDocument.tags();
    Q_FOREACH (const Tag &tag, tags) {
        TagCreateJob *createjob = new TagCreateJob(tag);
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
    Q_UNUSED(parts);

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
    Q_UNUSED(parts);

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
        emit error(i18n("Parent collection not found in DOM tree."));
        changeProcessed();
        return;
    }

    Collection c(collection);
    c.setRemoteId(QUuid::createUuid().toString());
    if (XmlWriter::writeCollection(c, parentElem).isNull()) {
        emit error(i18n("Unable to write collection."));
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
        emit error(i18n("Modified collection not found in DOM tree."));
        changeProcessed();
        return;
    }

    Collection c(collection);
    QDomElement newElem;
    newElem = XmlWriter::collectionToElement(c, mDocument.document());
    // move all items/collections over to the new node
    const QDomNodeList children = oldElem.childNodes();
    const int numberOfChildren = children.count();
    for (int i = 0; i < numberOfChildren; ++i) {
        const QDomElement child = children.at(i).toElement();
        qCDebug(KNUTRESOURCE_LOG) << "reparenting " << child.tagName() << child.attribute(QStringLiteral("rid"));
        if (child.isNull()) {
            continue;
        }
        if (child.tagName() == QStringLiteral("item") || child.tagName() == QStringLiteral("collection")) {
            newElem.appendChild(child);   // reparents
            --i; // children, despite being const is modified by the reparenting
        }
    }
    oldElem.parentNode().replaceChild(newElem, oldElem);
    save();
    changeCommitted(c);
}

void KnutResource::collectionRemoved(const Akonadi::Collection &collection)
{
    const QDomElement colElem = mDocument.collectionElementByRemoteId(collection.remoteId());
    if (colElem.isNull()) {
        emit error(i18n("Deleted collection not found in DOM tree."));
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
        emit error(i18n("Parent collection '%1' not found in DOM tree." , collection.remoteId()));
        changeProcessed();
        return;
    }

    Item i(item);
    i.setRemoteId(QUuid::createUuid().toString());
    if (XmlWriter::writeItem(i, parentElem).isNull()) {
        emit error(i18n("Unable to write item."));
        changeProcessed();
    } else {
        save();
        changeCommitted(i);
    }
}

void KnutResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts);

    const QDomElement oldElem = mDocument.itemElementByRemoteId(item.remoteId());
    if (oldElem.isNull()) {
        emit error(i18n("Modified item not found in DOM tree."));
        changeProcessed();
        return;
    }

    Item i(item);
    const QDomElement newElem = XmlWriter::itemToElement(i, mDocument.document());
    oldElem.parentNode().replaceChild(newElem, oldElem);
    save();
    changeCommitted(i);
}

void KnutResource::itemRemoved(const Akonadi::Item &item)
{
    const QDomElement itemElem = mDocument.itemElementByRemoteId(item.remoteId());
    if (itemElem.isNull()) {
        emit error(i18n("Deleted item not found in DOM tree."));
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
        emit error(i18n("Parent collection '%1' not found in DOM tree.", collectionSource.remoteId()));
        changeProcessed();
        return;
    }

    QDomElement destParentElem = mDocument.collectionElementByRemoteId(collectionDestination.remoteId());
    if (destParentElem.isNull()) {
        emit error(i18n("Parent collection '%1' not found in DOM tree.", collectionDestination.remoteId()));
        changeProcessed();
        return;
    }
    QDomElement itemElem = mDocument.itemElementByRemoteId(item.remoteId());
    if (itemElem.isNull()) {
        emit error(i18n("No item found for remoteid %1", item.remoteId()));
    }

    sourceParentElem.removeChild(itemElem);
    destParentElem.appendChild(itemElem);

    if (XmlWriter::writeItem(item, destParentElem).isNull()) {
        emit error(i18n("Unable to write item."));
    } else {
        save();
    }
    changeProcessed();
}

QSet<qint64> KnutResource::parseQuery(const QString &queryString)
{
    QSet<qint64> resultSet;
    Akonadi::SearchQuery query = Akonadi::SearchQuery::fromJSON(queryString.toLatin1());
    foreach (const Akonadi::SearchTerm &term, query.term().subTerms()) {
        if (term.key() == QStringLiteral("resource")) {
            resultSet << term.value().toInt();
        }
    }
    return resultSet;
}

void KnutResource::search(const QString &query, const Collection &collection)
{
    Q_UNUSED(collection);
    const QVector<qint64> result = parseQuery(query).toList().toVector();
    qCDebug(KNUTRESOURCE_LOG) << "KNUT QUERY:" << query;
    qCDebug(KNUTRESOURCE_LOG) << "KNUT RESOURCE:" << result;
    searchFinished(result, Akonadi::AgentSearchInterface::Uid);
}

void KnutResource::addSearch(const QString &query, const QString &queryLanguage, const Collection &resultCollection)
{
    Q_UNUSED(query);
    Q_UNUSED(queryLanguage);
    Q_UNUSED(resultCollection);
    qCDebug(KNUTRESOURCE_LOG);
}

void KnutResource::removeSearch(const Collection &resultCollection)
{
    Q_UNUSED(resultCollection);
    qCDebug(KNUTRESOURCE_LOG);
}

AKONADI_RESOURCE_MAIN(KnutResource)
