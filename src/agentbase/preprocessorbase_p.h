/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PREPROCESSORBASE_P_H
#define PREPROCESSORBASE_P_H

#include "agentbase_p.h"

#include "preprocessorbase.h"
#include "itemfetchscope.h"

class KJob;

namespace Akonadi
{

class PreprocessorBasePrivate : public AgentBasePrivate
{
    Q_OBJECT

public:
    explicit PreprocessorBasePrivate(PreprocessorBase *parent);

    void delayedInit() override;

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
