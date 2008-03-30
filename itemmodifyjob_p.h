/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ITEMMODIFYJOB_P_H
#define AKONADI_ITEMMODIFYJOB_P_H

#include "job_p.h"

#include <QtCore/QStringList>

namespace Akonadi {

class ItemModifyJobPrivate : public JobPrivate
{
  public:
    enum Operation
    {
      RemoteId,
      Dirty
    };

    ItemModifyJobPrivate( ItemModifyJob *parent, const Item &item );

    void setClean();
    QByteArray nextPartHeader();

    Q_DECLARE_PUBLIC( ItemModifyJob )

    QSet<int> mOperations;
    QByteArray mTag;
    Item mItem;
    bool mRevCheck;
    QStringList mParts;
    QByteArray mPendingData;
};

}

#endif
