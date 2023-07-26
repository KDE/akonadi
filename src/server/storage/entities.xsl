<!--
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
<xsl:output method="text" encoding="utf-8"/>

<xsl:include href="entities-header.xsl"/>
<xsl:include href="entities-source.xsl"/>

<!-- select whether to generate header or implementation code. -->
<xsl:param name="code">header</xsl:param>

<xsl:template match="/">
/*
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

<!-- header generation -->
<xsl:if test="$code='header'">
#ifndef AKONADI_ENTITIES_H
#define AKONADI_ENTITIES_H
#include "storage/entity.h"

#include &lt;private/tristate_p.h&gt;

#include &lt;QtCore/QDebug&gt;
#include &lt;QtCore/QSharedDataPointer&gt;
#include &lt;QtCore/QString&gt;
#include &lt;QtCore/QVariant&gt;
#include &lt;QtCore/QStringList&gt;
template &lt;typename T&gt; class QList;

class QSqlQuery;

namespace Akonadi {
namespace Server {

// forward declaration for table classes
<xsl:for-each select="database/table">
class <xsl:value-of select="@name"/>;
</xsl:for-each>

// forward declaration for relation classes
<xsl:for-each select="database/relation">
class <xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation;
</xsl:for-each>

<xsl:for-each select="database/table">
<xsl:call-template name="table-header"/>
</xsl:for-each>

<xsl:for-each select="database/relation">
<xsl:call-template name="relation-header"/>
</xsl:for-each>

/** Returns a list of all table names. */
QList&lt;QString&gt; allDatabaseTables();

} // namespace Server
} // namespace Akonadi

<xsl:for-each select="database/table">
<xsl:call-template name="table-debug-header"/>
</xsl:for-each>

<xsl:for-each select="database/table">
Q_DECLARE_TYPEINFO(Akonadi::Server::<xsl:value-of select="@name"/>, Q_RELOCATABLE_TYPE);
</xsl:for-each>
#endif

</xsl:if>

<!-- cpp generation -->
<xsl:if test="$code='source'">
#include &lt;entities.h&gt;
#include &lt;storage/datastore.h&gt;
#include &lt;storage/selectquerybuilder.h&gt;
#include &lt;utils.h&gt;
#include &lt;akonadiserver_debug.h&gt;

#include &lt;QSqlDatabase&gt;
#include &lt;QSqlQuery&gt;
#include &lt;QSqlError&gt;
#include &lt;QSqlDriver&gt;
#include &lt;QVariant&gt;
#include &lt;QHash&gt;
#include &lt;QMutex&gt;
#include &lt;QThread&gt;

using namespace Akonadi;
using namespace Akonadi::Server;

static QStringList removeEntry(QStringList list, const QString&amp; entry)
{
    list.removeOne(entry);
    return list;
}

<xsl:for-each select="database/table">
<xsl:call-template name="table-source"/>
</xsl:for-each>

<xsl:for-each select="database/relation">
<xsl:call-template name="relation-source"/>
</xsl:for-each>

QList&lt;QString&gt; Akonadi::Server::allDatabaseTables()
{
    static const QList&lt;QString&gt; allTables = {
    <xsl:for-each select="database/table">
        QStringLiteral("<xsl:value-of select="@name"/>Table"),
    </xsl:for-each>
    <xsl:for-each select="database/relation">
        QStringLiteral("<xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation")
        <xsl:if test="position() != last()">,</xsl:if>
    </xsl:for-each>
    };

    return allTables;
}

</xsl:if>

</xsl:template>


<!-- Helper templates -->

<!-- uppercase first letter -->
<xsl:template name="uppercase-first">
<xsl:param name="argument"/>
<xsl:value-of select="concat(translate(substring($argument,1,1),'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'), substring($argument,2))"/>
</xsl:template>

<xsl:template name="data-type">
    <xsl:choose>
    <xsl:when test="@type = 'enum'"><xsl:value-of select="../@name"/>::<xsl:value-of select="@enumType"/></xsl:when>
    <xsl:otherwise><xsl:value-of select="@type"/></xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!-- generates function argument code for the current column -->
<xsl:template name="argument">
    <xsl:if test="starts-with(@type,'Q')">const </xsl:if><xsl:call-template name="data-type"/><xsl:text> </xsl:text>
    <xsl:if test="starts-with(@type,'Q')">&amp;</xsl:if><xsl:value-of select="@name"/>
</xsl:template>

<!-- signature of setter method -->
<xsl:template name="setter-signature">
<xsl:variable name="methodName">
  <xsl:value-of select="concat(translate(substring(@name,1,1),'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'), substring(@name,2))"/>
</xsl:variable>
set<xsl:value-of select="$methodName"/>(<xsl:call-template name="argument"/>)
</xsl:template>

<!-- field name list -->
<xsl:template name="column-list">
  <xsl:for-each select="column">
    <xsl:value-of select="@name"/>
    <xsl:if test="position() != last()"><xsl:text>, </xsl:text></xsl:if>
  </xsl:for-each>
</xsl:template>

<!-- data retrieval for a given key field -->
<xsl:template name="data-retrieval">
<xsl:param name="key"/>
<xsl:param name="key2"/>
<xsl:param name="lookupKey" select="$key"/>
<xsl:param name="cache"/>
<xsl:variable name="className"><xsl:value-of select="@name"/></xsl:variable>
    <xsl:if test="$cache != ''">
    if (Private::cacheEnabled) {
        QMutexLocker lock(&amp;Private::cacheMutex);
        auto it = Private::<xsl:value-of select="$cache"/>.constFind(<xsl:value-of select="$lookupKey"/>);
        if (it != Private::<xsl:value-of select="$cache"/>.constEnd()) {
            return it.value();
        }
    }
    </xsl:if>
    QSqlDatabase db = DataStore::self()->database();
    if (!db.isOpen()) {
        return <xsl:value-of select="$className"/>();
    }

    QueryBuilder qb(tableName(), QueryBuilder::Select);
    static const QStringList columns = removeEntry(columnNames(), <xsl:value-of select="$key"/>Column());
    qb.addColumns(columns);
    qb.addValueCondition(<xsl:value-of select="$key"/>Column(), Query::Equals, <xsl:value-of select="$key"/>);
    <xsl:if test="$key2 != ''">
    qb.addValueCondition(<xsl:value-of select="$key2"/>Column(), Query::Equals, <xsl:value-of select="$key2"/>);
    </xsl:if>
    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) &lt;&lt; "Error during selection of record with <xsl:value-of select="$key"/>"
                                     &lt;&lt; <xsl:value-of select="$key"/> &lt;&lt; "from table" &lt;&lt; tableName()
                                     &lt;&lt; qb.query().lastError().text();
        return <xsl:value-of select="$className"/>();
    }
    if (!qb.query().next()) {
        return <xsl:value-of select="$className"/>();
    }

    <!-- this indirection is required to prevent off-by-one access now that we skip the key column -->
    int valueIndex = 0;
    <xsl:for-each select="column">
    const <xsl:call-template name="data-type"/> &amp;value<xsl:value-of select="position()"/> =
    <xsl:choose>
      <xsl:when test="@name=$key">
        <xsl:value-of select="$key"/>;
      </xsl:when>
      <xsl:otherwise>
        (qb.query().isNull(valueIndex)) ?
        <xsl:call-template name="data-type"/>(<xsl:value-of select="@default"/>) :
        <xsl:choose>
          <xsl:when test="starts-with(@type,'QString')">
            Utils::variantToString(qb.query().value( valueIndex))
          </xsl:when>
          <xsl:when test="starts-with(@type, 'enum')">
            static_cast&lt;<xsl:value-of select="@enumType"/>&gt;(qb.query().value( valueIndex ).value&lt;int&gt;())
          </xsl:when>
          <xsl:when test="starts-with(@type, 'QDateTime')">
            Utils::variantToDateTime(qb.query().value(valueIndex))
          </xsl:when>
          <xsl:otherwise>
            qb.query().value( valueIndex ).value&lt;<xsl:value-of select="@type"/>&gt;()
          </xsl:otherwise>
        </xsl:choose>
           ; ++valueIndex;
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>

    <xsl:value-of select="$className"/> rv(
    <xsl:for-each select="column">
        value<xsl:value-of select="position()"/>
      <xsl:if test="position() != last()">,</xsl:if>
    </xsl:for-each>
    );
    if (Private::cacheEnabled) {
        Private::addToCache(rv);
    }
    return rv;
</xsl:template>

<!-- method name for n:1 referred records -->
<xsl:template name="method-name-n1">
<xsl:choose>
<xsl:when test="@methodName != ''">
  <xsl:value-of select="@methodName"/>
</xsl:when>
<xsl:otherwise>
  <xsl:value-of select="concat(translate(substring(@refTable,1,1),'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz'), substring(@refTable,2))"/>
</xsl:otherwise>
</xsl:choose>
</xsl:template>

</xsl:stylesheet>

