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

#ifndef MOVECOMMAND_H
#define MOVECOMMAND_H

#include "commandbase_p.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QList>

class KJob;
class MoveCommand : public CommandBase
{
    Q_OBJECT
public:
    MoveCommand(const Akonadi::Collection &destFolder, const Akonadi::Item::List &msgList, QObject *parent = 0);
    void execute();

private Q_SLOTS:
    void slotMoveResult(KJob *job);

private:
    Akonadi::Collection mDestFolder;
    QList<Akonadi::Item> mMessages;
};

#endif // MOVECOMMAND_H
