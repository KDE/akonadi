/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTIONFETCHHANDLER_H_
#define AKONADI_COLLECTIONFETCHHANDLER_H_

#include "entities.h"
#include "handler.h"

template<typename T> class QStack;

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
class CollectionFetchHandler : public Handler
{
public:
    CollectionFetchHandler(AkonadiServer &akonadi);
    ~CollectionFetchHandler() override = default;

    bool parseStream() override;

private:
    void listCollection(const Collection &root, const QStack<Collection> &ancestors, const QStringList &mimeTypes, const CollectionAttribute::List &attributes);
    QStack<Collection> ancestorsForCollection(const Collection &col);
    void retrieveCollections(const Collection &topParent, int depth);
    bool checkFilterCondition(const Collection &col) const;
    CollectionAttribute::List getAttributes(const Collection &colId, const QSet<QByteArray> &filter = QSet<QByteArray>());
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
