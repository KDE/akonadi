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

class FakeDataStore : public DataStore
{
    Q_OBJECT

public:
    virtual ~FakeDataStore();
    static DataStore *self();

    bool init() Q_DECL_OVERRIDE;

    QMap<QString, QVariantList> changes() const
    {
        return mChanges;
    }

    virtual bool setItemsFlags(const PimItem::List &items,
                               const QVector<Flag> &flags,
                               bool *flagsChanged = 0,
                               const Collection &col = Collection(),
                               bool silent = false) Q_DECL_OVERRIDE;
    virtual bool appendItemsFlags(const PimItem::List &items,
                                  const QVector<Flag> &flags,
                                  bool *flagsChanged = 0,
                                  bool checkIfExists = true,
                                  const Collection &col = Collection(),
                                  bool silent = false) Q_DECL_OVERRIDE;
    virtual bool removeItemsFlags(const PimItem::List &items,
                                  const QVector<Flag> &flags,
                                  bool *flagsChanged = 0,
                                  const Collection &col = Collection(),
                                  bool silent = false) Q_DECL_OVERRIDE;

    virtual bool setItemsTags(const PimItem::List &items,
                              const Tag::List &tags,
                              bool *tagsChanged = 0,
                              bool silent = false) Q_DECL_OVERRIDE;
    virtual bool appendItemsTags(const PimItem::List &items,
                                 const Tag::List &tags,
                                 bool *tagsChanged = 0,
                                 bool checkIfExists = true,
                                 const Collection &col = Collection(),
                                 bool silent = false) Q_DECL_OVERRIDE;
    virtual bool removeItemsTags(const PimItem::List &items,
                                 const Tag::List &tags,
                                 bool *tagsChanged = 0,
                                 bool silent = false) Q_DECL_OVERRIDE;

    virtual bool removeItemParts(const PimItem &item,
                                 const QSet<QByteArray> &parts)  Q_DECL_OVERRIDE;

    virtual bool invalidateItemCache(const PimItem &item) Q_DECL_OVERRIDE;

    virtual bool appendCollection(Collection &collection) Q_DECL_OVERRIDE;

    virtual bool cleanupCollection(Collection &collection) Q_DECL_OVERRIDE;
    virtual bool cleanupCollection_slow(Collection &collection) Q_DECL_OVERRIDE;

    virtual bool moveCollection(Collection &collection,
                                const Collection &newParent) Q_DECL_OVERRIDE;

    virtual bool appendMimeTypeForCollection(qint64 collectionId,
                                             const QStringList &mimeTypes) Q_DECL_OVERRIDE;

    virtual void activeCachePolicy(Collection &col) Q_DECL_OVERRIDE;

    virtual bool appendMimeType(const QString &mimetype,
                                qint64 *insertId = Q_NULLPTR) Q_DECL_OVERRIDE;
    virtual bool appendPimItem(QVector<Part> &parts,
                               const MimeType &mimetype,
                               const Collection &collection,
                               const QDateTime &dateTime,
                               const QString &remote_id,
                               const QString &remoteRevision,
                               const QString &gid,
                               PimItem &pimItem) Q_DECL_OVERRIDE;

    virtual bool cleanupPimItems(const PimItem::List &items) Q_DECL_OVERRIDE;

    virtual bool unhidePimItem(PimItem &pimItem) Q_DECL_OVERRIDE;
    virtual bool unhideAllPimItems() Q_DECL_OVERRIDE;

    virtual bool addCollectionAttribute(const Collection &col,
                                        const QByteArray &key,
                                        const QByteArray &value) Q_DECL_OVERRIDE;
    virtual bool removeCollectionAttribute(const Collection &col,
                                           const QByteArray &key) Q_DECL_OVERRIDE;

    virtual bool beginTransaction() Q_DECL_OVERRIDE;
    virtual bool rollbackTransaction() Q_DECL_OVERRIDE;
    virtual bool commitTransaction() Q_DECL_OVERRIDE;

    virtual NotificationCollector *notificationCollector() Q_DECL_OVERRIDE;

    void setPopulateDb(bool populate);

protected:
    FakeDataStore();

    QMap<QString, QVariantList> mChanges;

private:
    bool populateDatabase();
    bool mPopulateDb;

};

}
}

#endif // AKONADI_SERVER_FAKEDATASTORE_H
