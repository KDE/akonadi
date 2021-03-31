/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include <QStringList>
#include <QVector>

namespace Akonadi
{
namespace Server
{
class SearchHelper
{
public:
    static QVector<qint64> matchSubcollectionsByMimeType(const QVector<qint64> &ancestors, const QStringList &mimeTypes);
};

} // namespace Server
} // namespace Akonadi

