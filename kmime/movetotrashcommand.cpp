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

#include "movetotrashcommand_p.h"
#include "util_p.h"
#include "movecommand_p.h"
#include "imapsettings.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/kmime/specialmailcollections.h>
#include <akonadi/entitytreemodel.h>

MoveToTrashCommand::MoveToTrashCommand(const QAbstractItemModel *model, const Akonadi::Collection::List &folders, QObject *parent)
    : CommandBase(parent)
{
    the_trashCollectionFolder = -1;
    mFolders = folders;
    mModel = model;
    mFolderListJobCount = mFolders.size();
}

MoveToTrashCommand::MoveToTrashCommand(const QAbstractItemModel *model, const QList< Akonadi::Item > &msgList, QObject *parent)
    : CommandBase(parent)
{
    the_trashCollectionFolder = -1;
    mMessages = msgList;
    mModel = model;
    mFolderListJobCount = 0;
}

void MoveToTrashCommand::slotFetchDone(KJob *job)
{
    mFolderListJobCount--;

    if (job->error()) {
        // handle errors
        Util::showJobError(job);
        emitResult(Failed);
        return;
    }

    Akonadi::ItemFetchJob *fjob = static_cast<Akonadi::ItemFetchJob *>(job);

    mMessages =  fjob->items();
    moveMessages();

    if (mFolderListJobCount > 0) {
        Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(mFolders[mFolderListJobCount - 1], parent());
        job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
        connect(job, SIGNAL(result(KJob*)), this, SLOT(slotFetchDone(KJob*)));
    }
}

void MoveToTrashCommand::execute()
{
    if (!mFolders.isEmpty()) {
        Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(mFolders[mFolderListJobCount - 1], parent());
        job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
        connect(job, SIGNAL(result(KJob*)), this, SLOT(slotFetchDone(KJob*)));
    } else if (!mMessages.isEmpty()) {
        mFolders << mMessages.first().parentCollection();
        moveMessages();
    } else {
        emitResult(OK);
    }
}

void MoveToTrashCommand::moveMessages()
{
    Akonadi::Collection folder = mFolders[mFolderListJobCount];
    if (folder.isValid()) {
        MoveCommand *moveCommand = new MoveCommand(findTrashFolder(folder), mMessages, this);
        connect(moveCommand, SIGNAL(result(Result)), this, SLOT(slotMoveDone(Result)));
        moveCommand->execute();
    } else {
        emitResult(Failed);
    }
}

void MoveToTrashCommand::slotMoveDone(const Result &result)
{
    if (result == Failed) {
        emitResult(Failed);
    }
    if (mFolderListJobCount == 0 && result == OK) {
        emitResult(OK);
    }
}

Akonadi::Collection MoveToTrashCommand::collectionFromId(const Akonadi::Collection::Id &id) const
{
    const QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForCollection(
                                mModel, Akonadi::Collection(id)
                            );
    return idx.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
}

Akonadi::Collection MoveToTrashCommand::trashCollectionFromResource(const Akonadi::Collection &col)
{
    //NOTE(Andras): from kmail/kmkernel.cpp
    Akonadi::Collection trashCol;
    if (col.isValid()) {
        if (col.resource().contains(IMAP_RESOURCE_IDENTIFIER)) {
            //TODO: we really need some standard interface to query for special collections,
            //instead of relying on a resource's settings interface
            OrgKdeAkonadiImapSettingsInterface *iface = Util::createImapSettingsInterface(col.resource());
            if (iface->isValid()) {

                trashCol =  Akonadi::Collection(iface->trashCollection());
                delete iface;
                return trashCol;
            }
            delete iface;
        }
    }
    return trashCol;
}

Akonadi::Collection MoveToTrashCommand::trashCollectionFolder()
{
    if (the_trashCollectionFolder < 0) {
        the_trashCollectionFolder = Akonadi::SpecialMailCollections::self()->defaultCollection(Akonadi::SpecialMailCollections::Trash).id();
    }
    return collectionFromId(the_trashCollectionFolder);
}

Akonadi::Collection MoveToTrashCommand::findTrashFolder(const Akonadi::Collection &folder)
{
    Akonadi::Collection col = trashCollectionFromResource(folder);
    if (!col.isValid()) {
        col = trashCollectionFolder();
    }
    if (folder != col) {
        return col;
    }
    return Akonadi::Collection();
}

#include "moc_movetotrashcommand_p.cpp"
