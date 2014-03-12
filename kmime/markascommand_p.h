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

#ifndef MARKASCOMMAND_H
#define MARKASCOMMAND_H

#include "commandbase_p.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/kmime/messagestatus.h>

#include <QList>

class KJob;
class MarkAsCommand : public CommandBase
{
    Q_OBJECT
public:
    MarkAsCommand(const Akonadi::MessageStatus &targetStatus, const Akonadi::Item::List &msgList, bool invert = false, QObject *parent = 0);
    MarkAsCommand(const Akonadi::MessageStatus &targetStatus, const Akonadi::Collection::List &folders, bool invert = false, QObject *parent = 0);
    void execute();

private Q_SLOTS:
    void slotFetchDone(KJob *job);
    void slotModifyItemDone(KJob *job);

private:
    void markMessages();

    Akonadi::Collection::List mFolders;
    QList<Akonadi::Item> mMessages;
    Akonadi::MessageStatus mTargetStatus;
    int mMarkJobCount;
    int mFolderListJobCount;
    int mInvertMark;
};

#endif // MARKASCOMMAND_H
