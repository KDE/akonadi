/***************************************************************************
 *   SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef AKONADI_SEARCHHANDLER_H_
#define AKONADI_SEARCHHANDLER_H_

#include "handler.h"

#include <QSet>

namespace Akonadi
{
namespace Server
{
/**
  @ingroup akonadi_server_handler

  Handler for the search commands.
*/
class SearchHandler : public Handler
{
public:
    SearchHandler(AkonadiServer &akonadi);
    ~SearchHandler() override = default;

    bool parseStream() override;

private:
    void processResults(const QSet<qint64> &results);

    Protocol::ItemFetchScope mItemFetchScope;
    Protocol::TagFetchScope mTagFetchScope;
    QSet<qint64> mAllResults;
};

} // namespace Server
} // namespace Akonadi

#endif
