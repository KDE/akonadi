/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include <QList>
#include <QStringList>

namespace Akonadi
{
namespace Server
{
class SearchHelper
{
public:
    static QList<qint64> matchSubcollectionsByMimeType(const QList<qint64> &ancestors, const QStringList &mimeTypes);
};

} // namespace Server
} // namespace Akonadi
