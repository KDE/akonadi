/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Copyright (c) 2010 Andras Mantia <andras@kdab.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef REMOVEDUPLICATESCOMMAND_P_H
#define REMOVEDUPLICATESCOMMAND_P_H

#include <commandbase_p.h>

#include "akonadi/collection.h"
#include "akonadi/item.h"

class QAbstractItemModel;
class KJob;

class RemoveDuplicatesCommand : public CommandBase
{
  Q_OBJECT
public:
    RemoveDuplicatesCommand( QAbstractItemModel* model, const Akonadi::Collection::List& folders, QObject* parent = 0);
    virtual void execute();

private Q_SLOTS:
    void slotFetchDone( KJob* job );
private:
    Akonadi::Collection::List mFolders;
    Akonadi::Item::List mDuplicateItems;
    int mJobCount;
    QAbstractItemModel* mModel; //just pass to the internal trash command
};

#endif // REMOVEDUPLICATESCOMMAND_P_H
