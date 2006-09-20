/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

class DataStore;

/**
  Helper class for DataStore transaction handling.
  Works similar to QMutexLocker.
*/
class Transaction
{
  public:
    /**
      Starts a new transaction. The transaction will automatically rolled back
      on destruction if it hasn't been committed explicitly before.
      @param db The corresponding DataStore. You must not delete @p db during
      the lifetime of a Transaction object.
    */
    Transaction( DataStore *db );

    /**
      Rolls back the transaction if it hasn't been committed explicitly.
    */
    ~Transaction();

    /**
      Commits the transaction. Returns true on success.
    */
    bool commit();

  private:
    Q_DISABLE_COPY( Transaction )
    DataStore* mDb;
};

}

#endif
