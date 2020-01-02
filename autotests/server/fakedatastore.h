/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKONADI_SERVER_FAKEDATASTORE_H
#define AKONADI_SERVER_FAKEDATASTORE_H

#include "storage/datastore.h"

namespace Akonadi {
namespace Server {

class FakeAkonadiServer;

class FakeDataStoreFactory : public DataStoreFactory
{
public:
    FakeDataStoreFactory(FakeAkonadiServer &akonadi);
    DataStore *createStore() override;
private:
    FakeAkonadiServer &m_akonadi;
};

class FakeDataStore : public DataStore
{
    Q_OBJECT
    friend class FakeDataStoreFactory;
public:
    ~FakeDataStore() override;

    bool init() override;

    QMap<QString, QVariantList> changes() const
    {
        return mChanges;
    }

    bool setItemsFlags(const PimItem::List &items,
                               const QVector<Flag> &flags,
                               bool *flagsChanged = nullptr,
                               const Collection &col = Collection(),
                               bool silent = false) override;
    bool appendItemsFlags(const PimItem::List &items,
                                  const QVector<Flag> &flags,
                                  bool *flagsChanged = nullptr,
                                  bool checkIfExists = true,
                                  const Collection &col = Collection(),
                                  bool silent = false) override;
    bool removeItemsFlags(const PimItem::List &items,
                                  const QVector<Flag> &flags,
                                  bool *flagsChanged = nullptr,
                                  const Collection &col = Collection(),
                                  bool silent = false) override;

    bool setItemsTags(const PimItem::List &items,
                              const Tag::List &tags,
                              bool *tagsChanged = nullptr,
                              bool silent = false) override;
    bool appendItemsTags(const PimItem::List &items,
                                 const Tag::List &tags,
                                 bool *tagsChanged = nullptr,
                                 bool checkIfExists = true,
                                 const Collection &col = Collection(),
                                 bool silent = false) override;
    bool removeItemsTags(const PimItem::List &items,
                                 const Tag::List &tags,
                                 bool *tagsChanged = nullptr,
                                 bool silent = false) override;

    bool removeItemParts(const PimItem &item,
                                 const QSet<QByteArray> &parts)  override;

    bool invalidateItemCache(const PimItem &item) override;

    bool appendCollection(Collection &collection,
                                  const QStringList &mimeTypes,
                                  const QMap<QByteArray, QByteArray> &attributes) override;

    bool cleanupCollection(Collection &collection) override;
    bool cleanupCollection_slow(Collection &collection) override;

    bool moveCollection(Collection &collection,
                                const Collection &newParent) override;

    virtual bool appendMimeTypeForCollection(qint64 collectionId,
                                             const QStringList &mimeTypes) override;

    void activeCachePolicy(Collection &col) override;

    bool appendPimItem(QVector<Part> &parts,
                               const QVector<Flag> &flags,
                               const MimeType &mimetype,
                               const Collection &collection,
                               const QDateTime &dateTime,
                               const QString &remote_id,
                               const QString &remoteRevision,
                               const QString &gid,
                               PimItem &pimItem) override;

    bool cleanupPimItems(const PimItem::List &items, bool silent = false) override;

    bool unhidePimItem(PimItem &pimItem) override;
    bool unhideAllPimItems() override;

    bool addCollectionAttribute(const Collection &col,
                                        const QByteArray &key,
                                        const QByteArray &value,
                                        bool silent = false) override;
    bool removeCollectionAttribute(const Collection &col,
                                           const QByteArray &key) override;

    bool beginTransaction(const QString &name = QString()) override;
    bool rollbackTransaction() override;
    bool commitTransaction() override;

    void setPopulateDb(bool populate);

protected:
    FakeDataStore(FakeAkonadiServer &akonadi);

    QMap<QString, QVariantList> mChanges;

private:
    bool populateDatabase();
    bool mPopulateDb;

};

}
}

#endif // AKONADI_SERVER_FAKEDATASTORE_H
