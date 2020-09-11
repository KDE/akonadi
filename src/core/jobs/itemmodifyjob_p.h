/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ITEMMODIFYJOB_P_H
#define AKONADI_ITEMMODIFYJOB_P_H

#include "akonadicore_export.h"
#include "job_p.h"

namespace Akonadi
{

namespace Protocol
{
class PartMetaData;
class Command;
class ModifyItemsCommand;
using ModifyItemsCommandPtr = QSharedPointer<ModifyItemsCommand>;
}

/**
 * @internal
 */
class AKONADICORE_EXPORT ItemModifyJobPrivate : public JobPrivate
{
public:
    enum Operation {
        RemoteId,
        RemoteRevision,
        Gid,
        Dirty
    };

    explicit ItemModifyJobPrivate(ItemModifyJob *parent);

    void setClean();
    Protocol::PartMetaData preparePart(const QByteArray &partName);

    void conflictResolved();
    void conflictResolveError(const QString &message);

    void doUpdateItemRevision(Item::Id id, int oldRevision, int newRevision) override;

    QString jobDebuggingString() const override;
    Protocol::ModifyItemsCommandPtr fullCommand() const;

    void setSilent(bool silent);

    Q_DECLARE_PUBLIC(ItemModifyJob)

    QSet<int> mOperations;
    QByteArray mTag;
    Item::List mItems;
    bool mRevCheck = true;
    QSet<QByteArray> mParts;
    QSet<QByteArray> mForeignParts;
    QByteArray mPendingData;
    bool mIgnorePayload = false;
    bool mAutomaticConflictHandlingEnabled = true;
    bool mSilent = false;
};

}

#endif
