/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONFETCHHANDLER_H_
#define AKONADI_COLLECTIONFETCHHANDLER_H_

#include "entities.h"
#include "handler.h"

template <typename T> class QStack;

namespace Akonadi
{
namespace Server
{

/**
  @ingroup akonadi_server_handler

  Handler for the LIST command.

  This command is used to get a (limited) listing of the available collections.
  It is different from the LIST command and is more similar to FETCH.

  The @c RID command prefix indicates that @c collection-id is a remote identifier
  instead of a unique identifier. In this case a resource context has to be specified
  previously using the @c RESSELECT command.

  @c depths chooses between recursive (@c INF), flat (1) and local (0, ie. just the
  base collection) listing, 0 indicates the root collection.

  The @c filter-list is used to restrict the listing to collection of a specific
  resource or content type.

  The @c option-list allows to specify the response content to some extend:
  - @c STATISTICS (boolean) allows to include the collection statistics (see Status)
  - @c ANCESTORDEPTH (numeric) allows you to specify the number of ancestor nodes that
    should be included additionally to the @c parent-id included anyway.
    Possible values are @c 0 (the default), @c 1 for the direct parent node and @c INF for all,
    terminating with the root collection.

  The name is encoded as an quoted UTF-8 string. There is no order defined for the
  single responses.

  The ancestors property is encoded as a list of UID/RID pairs.
*/
class CollectionFetchHandler: public Handler
{
public:
    CollectionFetchHandler() = default;
    ~CollectionFetchHandler() override = default;

    bool parseStream() override;

private:
    void listCollection(const Collection &root,
                        const QStack<Collection> &ancestors,
                        const QStringList &mimeTypes,
                        const CollectionAttribute::List &attributes);
    QStack<Collection> ancestorsForCollection(const Collection &col);
    void retrieveCollections(const Collection &topParent, int depth);
    bool checkFilterCondition(const Collection &col) const;
    bool checkChildrenForMimeTypes(const QHash<qint64, Collection> &collectionsMap,
                                   const QHash<qint64, qint64> &parentMap,
                                   const Collection &col);
    CollectionAttribute::List getAttributes(const Collection &colId,
                                            const QSet<QByteArray> &filter = QSet<QByteArray>());
    void retrieveAttributes(const QVariantList &collectionIds);

    Resource mResource;
    QVector<MimeType::Id> mMimeTypes;
    int mAncestorDepth = 0;
    bool mIncludeStatistics = false;
    bool mEnabledCollections = false;
    bool mCollectionsToDisplay = false;
    bool mCollectionsToSynchronize = false;
    bool mCollectionsToIndex = false;
    QSet<QByteArray> mAncestorAttributes;
    QMap<qint64 /*id*/, Collection> mCollections;
    QHash<qint64 /*id*/, Collection> mAncestors;
    QMultiHash<qint64 /*collectionId*/, CollectionAttribute /*mimetypeId*/> mCollectionAttributes;
};

} // namespace Server
} // namespace Akonadi

#endif
