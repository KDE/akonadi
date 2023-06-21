/******************************************************************************
 *
 *  SPDX-FileCopyrightText: 2009 Szymon Stefanek <s.stefanek at gmail dot com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 *
 *****************************************************************************/

#include "preprocessorbase.h"

#include "preprocessorbase_p.h"

using namespace Akonadi;

PreprocessorBase::PreprocessorBase(const QString &id)
    : AgentBase(new PreprocessorBasePrivate(this), id)
{
}

PreprocessorBase::~PreprocessorBase()
{
}

void PreprocessorBase::finishProcessing(ProcessingResult result)
{
    Q_D(PreprocessorBase);

    Q_ASSERT_X(result != ProcessingDelayed, "PreprocessorBase::terminateProcessing", "You should never pass ProcessingDelayed to this function");
    Q_ASSERT_X(d->mInDelayedProcessing, "PreprocessorBase::terminateProcessing", "terminateProcessing() called while not in delayed processing mode");
    Q_UNUSED(result)

    d->mInDelayedProcessing = false;
    Q_EMIT d->itemProcessed(d->mDelayedProcessingItemId);
}

void PreprocessorBase::setFetchScope(const ItemFetchScope &fetchScope)
{
    Q_D(PreprocessorBase);

    d->mFetchScope = fetchScope;
}

ItemFetchScope &PreprocessorBase::fetchScope()
{
    Q_D(PreprocessorBase);

    return d->mFetchScope;
}

#include "moc_preprocessorbase.cpp"
