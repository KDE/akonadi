/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_TRANSACTION_H
#define AKONADI_TRANSACTION_H

#include <qglobal.h>

namespace Akonadi {
namespace Server {

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
      @param beginTransaction if false, the transaction won't be started, until begin is explicitly called. The default is to begin the transaction right away.
    */
    explicit Transaction(DataStore *db, bool beginTransaction = true);

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
    bool mCommitted;
};

} // namespace Server
} // namespace Akonadi

#endif
