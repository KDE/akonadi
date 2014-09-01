/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef AKONADI_SERVER_MERGE_H
#define AKONADI_SERVER_MERGE_H

#include "handler/akappend.h"

namespace Akonadi {
namespace Server {

class Merge : public AkAppend
{
    Q_OBJECT

public:
    Merge();

    ~Merge();

    bool parseStream();

protected:
    bool mergeItem(PimItem &newItem, PimItem &currentItem,
                   const ChangedAttributes &itemFlags,
                   const ChangedAttributes &itemTagsRID,
                   const ChangedAttributes &itemTagsGID);

    bool notify(const PimItem &item, const Collection &collection);
    bool sendResponse(const QByteArray &response, const PimItem &item);

private:
    QSet<QByteArray> mChangedParts;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_SERVER_MERGE_H
