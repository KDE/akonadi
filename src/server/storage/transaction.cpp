/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "transaction.h"
#include "storage/datastore.h"

using namespace Akonadi::Server;

Transaction::Transaction(DataStore *db, const QString &name, bool beginTransaction)
    : mDb(db)
    , mName(name)
    , mCommitted(false)
{
    if (beginTransaction) {
        mDb->beginTransaction(mName);
    }
}

Transaction::~Transaction()
{
    if (!mCommitted) {
        mDb->rollbackTransaction();
    }
}

bool Transaction::commit()
{
    mCommitted = true;
    return mDb->commitTransaction();
}

void Transaction::begin()
{
    mDb->beginTransaction(mName);
}
