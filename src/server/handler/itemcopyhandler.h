/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "entities.h"
#include "handler.h"

namespace Akonadi
{
namespace Server
{
/**
  @ingroup akonadi_server_handler

  Handler for the COPY command.

  This command is used to copy a set of items into the specific collection. It
  is syntactically identical to the IMAP COPY command.

  The copied items differ in the following points from the originals:
  - new unique id
  - empty remote id
  - possible located in a different collection (and thus resource)

  There is only the usual status response indicating success or failure of the
  COPY command
 */
class ItemCopyHandler : public Handler
{
public:
    ItemCopyHandler(AkonadiServer &akonadi);
    ~ItemCopyHandler() override = default;

    bool parseStream() override;

protected:
    /**
      Copy the given item and all its parts into the @p target.
      The changes mentioned above are applied.
    */
    bool copyItem(const PimItem &item, const Collection &target);
    void processItems(const QVector<qint64> &ids);

private:
    Collection mTargetCollection;
};

} // namespace Server
} // namespace Akonadi
