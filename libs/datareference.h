/*
    This file is part of libakonadi.

    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_DATAREFERENCE_H
#define AKONADI_DATAREFERENCE_H

#include "libakonadi_export.h"
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>

namespace Akonadi {

/**
  This class encapsulates a reference to a pim object in
  the pim storage service.
 */
class AKONADI_EXPORT DataReference
{
  public:
    typedef QList<DataReference> List;

    /**
      Creates an empty DataReference.
     */
    DataReference();

    /**
      Creates a new DataReference with the given local @p id and @p remoteId.
     */
    DataReference( int id, const QString &remoteId );

    /**
      Copy constructor.
     */
    DataReference( const DataReference &other );

    /**
      Destroys the DataReference.
     */
    ~DataReference();

    DataReference& operator=( const DataReference &other );

    /**
      Returns the local id of the DataReference or an invalid string if no
      id is set.
     */
    int id() const;

    /**
      Returns the remote of the DataReference or an empty string if no
      remote id is set.
     */
    QString remoteId() const;

    /**
      Returns true if this is a empty reference, ie. one created with
      DataReference().
     */
    bool isNull() const;

    /**
      Returns true if two references are equal.
     */
    bool operator==( const DataReference &other ) const;

    /**
      Returns true if two references are not equal.
     */
    bool operator!=( const DataReference &other ) const;

    /**
      Compares two references.
     */
    bool operator<( const DataReference &other ) const;

  private:
    class Private;
    QSharedDataPointer<Private> d;
};

}

AKONADI_EXPORT uint qHash( const Akonadi::DataReference& );

Q_DECLARE_METATYPE(Akonadi::DataReference)

#endif
