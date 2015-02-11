/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef PREPROCESSORBASE_P_H
#define PREPROCESSORBASE_P_H

#include "agentbase_p.h"

#include "preprocessorbase.h"
#include "itemfetchscope.h"

class KJob;

namespace Akonadi {

class PreprocessorBasePrivate : public AgentBasePrivate
{
    Q_OBJECT

public:
    explicit PreprocessorBasePrivate(PreprocessorBase *parent);

    void delayedInit() Q_DECL_OVERRIDE;

    void beginProcessItem(qlonglong itemId, qlonglong collectionId, const QString &mimeType);

Q_SIGNALS:
    void itemProcessed(qlonglong id);

private Q_SLOTS:
    void itemFetched(KJob *job);

public:
    bool mInDelayedProcessing;
    qlonglong mDelayedProcessingItemId;
    ItemFetchScope mFetchScope;

    Q_DECLARE_PUBLIC(PreprocessorBase)
};

}

#endif
