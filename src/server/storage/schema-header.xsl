<!--
    SPDX-FileCopyrightText: 2013 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    QList&lt;TableDescription&gt; tables() override;
    QList&lt;RelationDescription&gt; relations() override;
};

} // namespace Server
} // namespace Akonadi

#endif
</xsl:template>

</xsl:stylesheet>
