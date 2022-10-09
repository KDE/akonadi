/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QString>

namespace Akonadi
{
namespace Server
{
class DataStore;

/**
  Helper class for DataStore transaction handling.
  Works similar to QMutexLocker.
  Supports command-local and session-global transactions.
*/
class Transaction
{
public:
    /**
      Starts a new transaction. The transaction will automatically rolled back
      on destruction if it hasn't been committed explicitly before.
      If there is already a global transaction in progress, this one will be used
      instead of creating a new one.
      @param db The corresponding DataStore. You must not delete @p db during
      the lifetime of a Transaction object.
      @param name A name of the transaction. Used for debugging.
      @param beginTransaction if false, the transaction won't be started, until begin is explicitly called. The default is to begin the transaction right away.
    */
    explicit Transaction(DataStore *db, const QString &name, bool beginTransaction = true);

    /**
      Rolls back the transaction if it hasn't been committed explicitly.
      This also happens if a global transaction is used.
    */
    ~Transaction();

    /**
      Commits the transaction. Returns true on success.
      If a global transaction is used, nothing happens, global transactions have
      to be committed explicitly.
    */
    bool commit();

    void begin();

private:
    Q_DISABLE_COPY(Transaction)
    DataStore *mDb;
    QString mName;
    bool mCommitted;
};

} // namespace Server
} // namespace Akonadi
