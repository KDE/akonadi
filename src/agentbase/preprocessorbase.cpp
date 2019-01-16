/******************************************************************************
 *
 *  Copyright (c) 2009 Szymon Stefanek <s.stefanek at gmail dot com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA, 02110-1301, USA.
 *
 *****************************************************************************/

#include "preprocessorbase.h"

#include "preprocessorbase_p.h"

#include "akonadiagentbase_debug.h"

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
    Q_UNUSED(result);

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
