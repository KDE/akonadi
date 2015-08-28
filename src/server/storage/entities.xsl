<!--
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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
<xsl:output method="text" encoding="utf-8"/>

<xsl:include href="entities-header.xsl"/>
<xsl:include href="entities-source.xsl"/>

<!-- select wether to generate header or implementation code. -->
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

#include &lt;shared/akdebug.h&gt;
#include &lt;QtCore/QDebug&gt;
#include &lt;QtCore/QSharedDataPointer&gt;
#include &lt;QtCore/QString&gt;
#include &lt;QtCore/QVariant&gt;

template &lt;typename T&gt; class QVector;
class QSqlQuery;
class QStringList;

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
QVector&lt;QString&gt; allDatabaseTables();

} // namespace Server
} // namespace Akonadi

<xsl:for-each select="database/table">
<xsl:call-template name="table-debug-header"/>
</xsl:for-each>

<xsl:for-each select="database/table">
Q_DECLARE_TYPEINFO( Akonadi::Server::<xsl:value-of select="@name"/>, Q_MOVABLE_TYPE );
</xsl:for-each>
#endif

</xsl:if>

<!-- cpp generation -->
<xsl:if test="$code='source'">
#include &lt;entities.h&gt;
#include &lt;storage/datastore.h&gt;
#include &lt;storage/selectquerybuilder.h&gt;
#include &lt;utils.h&gt;

#include &lt;qsqldatabase.h&gt;
#include &lt;qsqlquery.h&gt;
#include &lt;qsqlerror.h&gt;
#include &lt;qsqldriver.h&gt;
#include &lt;qvariant.h&gt;
#include &lt;QtCore/QHash&gt;
#include &lt;QtCore/QMutex&gt;

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

QVector&lt;QString&gt; Akonadi::Server::allDatabaseTables()
{
  static const QVector&lt;QString&gt; allTables = QVector&lt;QString&gt;()
  <xsl:for-each select="database/table">
    &lt;&lt; QStringLiteral( "<xsl:value-of select="@name"/>Table" )
  </xsl:for-each>
  <xsl:for-each select="database/relation">
    &lt;&lt; QStringLiteral( "<xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation" )
  </xsl:for-each>
  ;
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

<!-- generates function argument code for the current column -->
<xsl:template name="argument">
  <xsl:if test="starts-with(@type,'Q')">const </xsl:if><xsl:value-of select="@type"/><xsl:text> </xsl:text>
  <xsl:if test="starts-with(@type,'Q')">&amp;</xsl:if><xsl:value-of select="@name"/>
</xsl:template>

<!-- signature of setter method -->
<xsl:template name="setter-signature">
<xsl:variable name="methodName">
  <xsl:value-of select="concat(translate(substring(@name,1,1),'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'), substring(@name,2))"/>
</xsl:variable>
set<xsl:value-of select="$methodName"/>( <xsl:call-template name="argument"/> )
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
  if ( Private::cacheEnabled ) {
    QMutexLocker lock(&amp;Private::cacheMutex);
    QHash&lt;<xsl:value-of select="column[@name = $key]/@type"/>, <xsl:value-of select="$className"/>&gt;::const_iterator it = Private::<xsl:value-of select="$cache"/>.constFind(<xsl:value-of select="$lookupKey"/>);
    if ( it != Private::<xsl:value-of select="$cache"/>.constEnd() ) {
      return it.value();
    }
  }
  </xsl:if>
  QSqlDatabase db = DataStore::self()->database();
  if ( !db.isOpen() )
    return <xsl:value-of select="$className"/>();

  QueryBuilder qb( tableName(), QueryBuilder::Select );
  static const QStringList columns = removeEntry(columnNames(), <xsl:value-of select="$key"/>Column());
  qb.addColumns( columns );
  qb.addValueCondition( <xsl:value-of select="$key"/>Column(), Query::Equals, <xsl:value-of select="$key"/> );
  <xsl:if test="$key2 != ''">
  qb.addValueCondition( <xsl:value-of select="$key2"/>Column(), Query::Equals, <xsl:value-of select="$key2"/> );
  </xsl:if>
  if ( !qb.exec() ) {
    akDebug() &lt;&lt; "Error during selection of record with <xsl:value-of select="$key"/>"
      &lt;&lt; <xsl:value-of select="$key"/> &lt;&lt; "from table" &lt;&lt; tableName()
      &lt;&lt; qb.query().lastError().text();
    return <xsl:value-of select="$className"/>();
  }
  if ( !qb.query().next() ) {
    return <xsl:value-of select="$className"/>();
  }

  <!-- this indirection is required to prevent off-by-one access now that we skip the key column -->
  int valueIndex = 0;
  <xsl:for-each select="column">
    const <xsl:value-of select="@type"/> value<xsl:value-of select="position()"/> =
    <xsl:choose>
      <xsl:when test="@name=$key">
        <xsl:value-of select="$key"/>;
      </xsl:when>
      <xsl:otherwise>
        (qb.query().isNull(valueIndex)) ?
        <xsl:value-of select="@type"/>() :
        <xsl:choose>
          <xsl:when test="starts-with(@type,'QString')">
          Utils::variantToString( qb.query().value( valueIndex ) )
          </xsl:when>
          <xsl:when test="starts-with(@type, 'Tristate')">
          static_cast&lt;Tristate&gt;(qb.query().value( valueIndex ).value&lt;int&gt;())
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
  if ( Private::cacheEnabled ) {
    Private::addToCache( rv );
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

