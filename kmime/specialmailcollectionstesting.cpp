/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "specialmailcollectionstesting_p.h"

#include "../specialcollections_p.h"
#include "specialmailcollections.h"
#include "specialmailcollectionssettings.h"

using namespace Akonadi;

typedef SpecialMailCollectionsSettings Settings;

SpecialMailCollectionsTesting *SpecialMailCollectionsTesting::_t_self()
{
    static SpecialMailCollectionsTesting *instance = 0;
    if (!instance) {
        instance = new SpecialMailCollectionsTesting;
    }
    return instance;
}

void SpecialMailCollectionsTesting::_t_setDefaultResourceId(const QString &resourceId)
{
    Settings::self()->defaultResourceIdItem()->setValue(resourceId);
    Settings::self()->writeConfig();
}

void SpecialMailCollectionsTesting::_t_forgetFoldersForResource(const QString &resourceId)
{
    static_cast<SpecialCollections *>(SpecialMailCollections::self())->d->forgetFoldersForResource(resourceId);
}

void SpecialMailCollectionsTesting::_t_beginBatchRegister()
{
    static_cast<SpecialCollections *>(SpecialMailCollections::self())->d->beginBatchRegister();
}

void SpecialMailCollectionsTesting::_t_endBatchRegister()
{
    static_cast<SpecialCollections *>(SpecialMailCollections::self())->d->endBatchRegister();
}

int SpecialMailCollectionsTesting::_t_knownResourceCount() const
{
    return static_cast<SpecialCollections *>(SpecialMailCollections::self())->d->mFoldersForResource.count();
}

int SpecialMailCollectionsTesting::_t_knownFolderCount() const
{
    const SpecialCollectionsPrivate *d = static_cast<SpecialCollections *>(SpecialMailCollections::self())->d;
    int ret = 0;

    QHashIterator<QString, QHash<QByteArray, Collection> > resourceIt(d->mFoldersForResource);
    while (resourceIt.hasNext()) {
        resourceIt.next();

        QHashIterator<QByteArray, Collection> it(resourceIt.value());
        while (it.hasNext()) {
            it.next();
            if (it.value().isValid()) {
                ret++;
            }
        }
    }
    return ret;
}
