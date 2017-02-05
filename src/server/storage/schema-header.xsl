<!--
    Copyright (c) 2013 Volker Krause <vkrause@kde.org>

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
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<!-- schema class header template -->
<xsl:template name="schema-header">
#ifndef <xsl:value-of select="$className"/>_H
#define <xsl:value-of select="$className"/>_H

#include "src/server/storage/schema.h"

namespace Akonadi {
namespace Server {

class <xsl:value-of select="$className"/> : public Schema
{
  public:
    QVector&lt;TableDescription&gt; tables() Q_DECL_OVERRIDE;
    QVector&lt;RelationDescription&gt; relations() Q_DECL_OVERRIDE;
};

} // namespace Server
} // namespace Akonadi

#endif
</xsl:template>

</xsl:stylesheet>
