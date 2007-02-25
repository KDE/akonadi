/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef AKONADISTOREQUERY_H
#define AKONADISTOREQUERY_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

namespace Akonadi {

/**
 * An tool class which does the parsing of a store request for us.
 */
class StoreQuery
{
  public:
    enum Operation
    {
      Replace = 1,
      Add = 2,
      Delete = 4,
      Silent = 8
    };

    enum DataType
    {
      Flags,
      Data,
      Collection,
      RemoteId,
      Dirty
    };

    StoreQuery();

    bool parse( const QByteArray &query );

    int operation() const;
    int dataType() const;
    QList<QByteArray> flags() const;
    QList<QByteArray> sequences() const;
    bool isUidStore() const;
    int continuationSize() const;
    QString collection() const;
    QString remoteId() const { return mCollection; }

    void dump();

  private:
    int mOperation;
    int mDataType;
    QList<QByteArray> mFlags;
    QList<QByteArray> mSequences;
    bool mIsUidStore;
    int mContinuationSize;
    QString mCollection;
};

}

#endif
