/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef AKTEST_DBINITIALIZER_H
#define AKTEST_DBINITIALIZER_H

#include "entities.h"
#include <private/protocol_p.h>

class DbInitializer
{
public:
    ~DbInitializer();
    Akonadi::Server::Resource createResource(const char *name);
    Akonadi::Server::Collection createCollection(const char *name,
                                                 const Akonadi::Server::Collection &parent = Akonadi::Server::Collection());
    Akonadi::Server::PimItem createItem(const char *name, const Akonadi::Server::Collection &parent);
    Akonadi::Server::Part createPart(qint64 pimitemId, const QByteArray &partname, const QByteArray &data);
    QByteArray toByteArray(bool enabled);
    QByteArray toByteArray(Akonadi::Server::Collection::Tristate tristate);
    Akonadi::Protocol::FetchCollectionsResponsePtr listResponse(const Akonadi::Server::Collection &col,
                                                                bool ancestors = false,
                                                                bool mimetypes = true,
                                                                const QStringList &ancestorFetchScope = QStringList());
    Akonadi::Protocol::FetchItemsResponsePtr fetchResponse(const Akonadi::Server::PimItem &item);
    Akonadi::Server::Collection collection(const char *name);

    void cleanup();

private:
    Akonadi::Server::Resource mResource;
};

#endif
