/***************************************************************************
 *   SPDX-FileCopyrightText: 2013 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef SCHEMA_H
#define SCHEMA_H

#include "schematypes.h"

namespace Akonadi
{
namespace Server
{

/** Methods to access the desired database schema (@see DbInspector for accessing
    the actual database schema).
 */
class Schema
{
public:
    inline virtual ~Schema() = default;

    /** List of tables in the schema. */
    virtual QVector<TableDescription> tables() = 0;

    /** List of relations (N:M helper tables) in the schema. */
    virtual QVector<RelationDescription> relations() = 0;

protected:
    explicit Schema() = default;

private:
    Q_DISABLE_COPY_MOVE(Schema)
};

} // namespace Server
} // namespace Akonadi

#endif
