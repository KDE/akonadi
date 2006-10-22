<!--
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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
                xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
                version="1.0">
<xsl:output method="text" encoding="utf-8"/>

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
#include &lt;storage/entity.h&gt;

#include &lt;qdebug.h&gt;
#include &lt;qstring.h&gt;
#include &lt;qvariant.h&gt;

namespace Akonadi {

// forward declarations
<xsl:for-each select="database/table">
class <xsl:value-of select="@name"/>;
</xsl:for-each>

<xsl:for-each select="database/table">
<xsl:call-template name="entity-header"/>
</xsl:for-each>

}
#endif

</xsl:if>

<!-- cpp generation -->
<xsl:if test="$code='source'">
#include &lt;storage/entities.h&gt;
#include &lt;storage/datastore.h&gt;

#include &lt;qsqldatabase.h&gt;
#include &lt;QLatin1String&gt;
#include &lt;qsqlquery.h&gt;
#include &lt;qsqlerror.h&gt;
#include &lt;qvariant.h&gt;

using namespace Akonadi;

<xsl:for-each select="database/table">
<xsl:call-template name="entity-source"/>
</xsl:for-each>

</xsl:if>

</xsl:template>



<!-- entity header template -->
<xsl:template name="entity-header">
<xsl:variable name="className"><xsl:value-of select="@name"/></xsl:variable>
class <xsl:value-of select="$className"/> : public Entity
{
  public:
    // constructor
    <xsl:value-of select="$className"/>();
    <xsl:value-of select="$className"/>( <xsl:call-template name="ctor-args"/> );

    // accessor methods
    <xsl:for-each select="column[@name != 'id']">
      <xsl:value-of select="@type"/><xsl:text> </xsl:text><xsl:value-of select="@name"/>() const;
    </xsl:for-each>

    // SQL table information
    static QString tableName();
    <xsl:for-each select="column">
    <xsl:text>static QString </xsl:text><xsl:value-of select="@name"/>Column();
    </xsl:for-each>

    // data retrieval
    <xsl:if test="column[@name = 'id']">
    <xsl:text>static </xsl:text><xsl:value-of select="$className"/> retrieveById( int id );
    </xsl:if>

    <xsl:if test="column[@name = 'name']">
    <xsl:text>static </xsl:text><xsl:value-of select="$className"/> retrieveByName( const QString &amp;name );
    </xsl:if>

    static QList&lt;<xsl:value-of select="$className"/>&gt; retrieveAll();
    static QList&lt;<xsl:value-of select="$className"/>&gt; retrieveFiltered( const QString &amp;key, const QVariant &amp;value );

    // data retrieval for referenced tables
    <xsl:for-each select="column[@refTable != '']">
    <xsl:value-of select="@refTable"/><xsl:text> </xsl:text><xsl:value-of select="concat(translate(substring(@refTable,1,1),'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz'), substring(@refTable,2))"/>() const;
    </xsl:for-each>

    // data retrieval for inverse referenced tables
    <xsl:for-each select="reference">
    QList&lt;<xsl:value-of select="@table"/>&gt; <xsl:value-of select="@name"/>() const;
    </xsl:for-each>

    // data retrieval for n:m relations
    <xsl:variable name="currentName"><xsl:value-of select="@name"/></xsl:variable>
    <xsl:for-each select="../relation[@table1 = $currentName]">
    QList&lt;<xsl:value-of select="@table2"/>&gt; <xsl:value-of select="concat(translate(substring(@table2,1,1),'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz'), substring(@table2,2))"/>s() const;
    </xsl:for-each>

  private:
    // member variables
    <xsl:for-each select="column[@name != 'id']">
    <xsl:value-of select="@type"/><xsl:text> m_</xsl:text><xsl:value-of select="@name"/>;
    </xsl:for-each>
};

// debug stream operator
QDebug &amp; operator&lt;&lt;( QDebug&amp; d, const <xsl:value-of select="$className"/>&amp; entity );
</xsl:template>



<!-- entity source template -->
<xsl:template name="entity-source">
<xsl:variable name="className"><xsl:value-of select="@name"/></xsl:variable>
<xsl:variable name="tableName"><xsl:value-of select="@name"/>Table</xsl:variable>

// constructor
<xsl:value-of select="$className"/>::<xsl:value-of select="$className"/>() : Entity() {}

<xsl:value-of select="$className"/>::<xsl:value-of select="$className"/>( <xsl:call-template name="ctor-args"/> ) :
<xsl:choose>
  <xsl:when test="column[@name='id']">
  Entity( id )
  </xsl:when>
  <xsl:otherwise>
  Entity()
  </xsl:otherwise>
</xsl:choose>
<xsl:for-each select="column">
  <xsl:if test="@name != 'id'">
    <xsl:text>, m_</xsl:text><xsl:value-of select="@name"/>( <xsl:value-of select="@name"/> )
  </xsl:if>
</xsl:for-each>
{
}

// accessor methods
<xsl:for-each select="column[@name != 'id']">
<xsl:value-of select="@type"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::<xsl:value-of select="@name"/>() const
{
  <xsl:text>return m_</xsl:text><xsl:value-of select="@name"/>;
}

</xsl:for-each>

// SQL table information
<xsl:text>QString </xsl:text><xsl:value-of select="$className"/>::tableName()
{
  return QLatin1String( "<xsl:value-of select="$tableName"/>" );
}

<xsl:for-each select="column">
QString <xsl:value-of select="$className"/>::<xsl:value-of select="@name"/>Column()
{
  return QLatin1String( "<xsl:value-of select="@name"/>" );
}
</xsl:for-each>


// data retrieval
<xsl:if test="column[@name='id']">
<xsl:value-of select="$className"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::retrieveById( int id )
{
  <xsl:call-template name="data-retrieval"><xsl:with-param name="key">id</xsl:with-param></xsl:call-template>
}

</xsl:if>
<xsl:if test="column[@name = 'name']">
<xsl:value-of select="$className"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::retrieveByName( const QString &amp;name )
{
  <xsl:call-template name="data-retrieval"><xsl:with-param name="key">name</xsl:with-param></xsl:call-template>
}
</xsl:if>

QList&lt;<xsl:value-of select="$className"/>&gt; <xsl:value-of select="$className"/>::retrieveAll()
{
  QSqlDatabase db = DataStore::self()->database();
  if ( !db.isOpen() )
    return QList&lt;<xsl:value-of select="$className"/>&gt;();

  QSqlQuery query( db );
  QString statement = QLatin1String( "SELECT <xsl:call-template name="column-list"/> FROM " );
  statement.append( tableName() );
  query.prepare( statement );
  if ( !query.exec() ) {
    qDebug() &lt;&lt; "Error during selection of all records from table" &lt;&lt; tableName()
      &lt;&lt; query.lastError().text();
    return QList&lt;<xsl:value-of select="$className"/>&gt;();
  }
  <xsl:call-template name="extract-result-list"/>
}

QList&lt;<xsl:value-of select="$className"/>&gt; <xsl:value-of select="$className"/>::retrieveFiltered( const QString &amp;key, const QVariant &amp;value )
{
  QSqlDatabase db = DataStore::self()->database();
  if ( !db.isOpen() )
    return QList&lt;<xsl:value-of select="$className"/>&gt;();

  QSqlQuery query( db );
  QString statement = QLatin1String( "SELECT <xsl:call-template name="column-list"/> FROM " );
  statement.append( tableName() );
  statement.append( QLatin1String(" WHERE ") );
  statement.append( key );
  statement.append( QLatin1String(" = :key") );
  query.prepare( statement );
  query.bindValue( QLatin1String(":key"), value );
  if ( !query.exec() ) {
    qDebug() &lt;&lt; "Error during selection of records from table" &lt;&lt; tableName()
      &lt;&lt; "filtered by" &lt;&lt; key &lt;&lt; "=" &lt;&lt; value
      &lt;&lt; query.lastError().text();
    return QList&lt;<xsl:value-of select="$className"/>&gt;();
  }
  <xsl:call-template name="extract-result-list"/>
}

// data retrieval for referenced tables
<xsl:for-each select="column[@refTable != '']">
<xsl:value-of select="@refTable"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::<xsl:value-of select="concat(translate(substring(@refTable,1,1),'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz'), substring(@refTable,2))"/>() const
{
  return <xsl:value-of select="@refTable"/>::retrieveById( <xsl:value-of select="@name"/>() );

}
</xsl:for-each>

// data retrieval for inverse referenced tables
<xsl:for-each select="reference">
QList&lt;<xsl:value-of select="@table"/>&gt; <xsl:value-of select="$className"/>::<xsl:value-of select="@name"/>() const
{
  return <xsl:value-of select="@table"/>::retrieveFiltered( <xsl:value-of select="@table"/>::<xsl:value-of select="@key"/>Column(), id() );
}
</xsl:for-each>

// data retrieval for n:m relations
<xsl:variable name="currentName"><xsl:value-of select="@name"/></xsl:variable>
<xsl:for-each select="../relation[@table1 = $currentName]">
QList&lt;<xsl:value-of select="@table2"/>&gt; <xsl:value-of select="$className"/>::<xsl:value-of select="concat(translate(substring(@table2,1,1),'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz'), substring(@table2,2))"/>s() const
{
  QSqlDatabase db = DataStore::self()->database();
  if ( !db.isOpen() )
    return QList&lt;<xsl:value-of select="@table2"/>&gt;();

  QSqlQuery query( db );
  QString statement = QLatin1String( "SELECT <xsl:value-of select="@table2"/>_<xsl:value-of select="@column2"/> FROM " );
  statement.append( QLatin1String("<xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation") );
  statement.append( QLatin1String(" WHERE <xsl:value-of select="@table1"/>_<xsl:value-of select="@column1"/> = :key") );
  query.prepare( statement );
  query.bindValue( QLatin1String(":key"), id() );
  if ( !query.exec() ) {
    qDebug() &lt;&lt; "Error during selection of records from table <xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation"
      &lt;&lt; query.lastError().text();
    return QList&lt;<xsl:value-of select="@table2"/>&gt;();
  }
  QList&lt;<xsl:value-of select="@table2"/>&gt; rv;
  while ( query.next() ) {
    int id = query.value( 0 ).toInt();
    <xsl:value-of select="@table2"/> tmp = <xsl:value-of select="@table2"/>::retrieveById( id );
    if ( tmp.isValid() )
      rv.append( tmp );
  }
  return rv;
}
</xsl:for-each>

// debuig stream operator
QDebug &amp; operator&lt;&lt;( QDebug&amp; d, const <xsl:value-of select="$className"/>&amp; entity )
{
  d &lt;&lt; "[<xsl:value-of select="$className"/>: "
  <xsl:for-each select="column">
    &lt;&lt; "<xsl:value-of select="@name"/> = " &lt;&lt; entity.<xsl:value-of select="@name"/>()
    <xsl:if test="position() != last()">&lt;&lt; ", "</xsl:if>
  </xsl:for-each>
    &lt;&lt; "]";
  return d;
}

</xsl:template>



<!-- Helper templates -->

<!-- constructor argument list -->
<xsl:template name="ctor-args">
  <xsl:for-each select="column">
    <xsl:if test="starts-with(@type,'Q')">
      <xsl:text>const </xsl:text>
    </xsl:if>
    <xsl:value-of select="@type"/><xsl:text> </xsl:text>
    <xsl:if test="starts-with(@type,'Q')">
      <xsl:text>&amp;</xsl:text>
    </xsl:if>
    <xsl:value-of select="@name"/>
    <xsl:if test="position()!=last()">, </xsl:if>
  </xsl:for-each>
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
<xsl:variable name="className"><xsl:value-of select="@name"/></xsl:variable>
  QSqlDatabase db = DataStore::self()->database();
  if ( !db.isOpen() )
    return <xsl:value-of select="$className"/>();

  QSqlQuery query( db );
  QString statement = QLatin1String( "SELECT <xsl:call-template name="column-list"/> FROM " );
  statement.append( tableName() );
  statement.append( QLatin1String(" WHERE <xsl:value-of select="$key"/> = :key") );
  query.prepare( statement );
  query.bindValue( QLatin1String(":key"), <xsl:value-of select="$key"/> );
  if ( !query.exec() ) {
    qDebug() &lt;&lt; "Error during selection of record with <xsl:value-of select="$key"/>"
      &lt;&lt; <xsl:value-of select="$key"/> &lt;&lt; "from table" &lt;&lt; tableName()
      &lt;&lt; query.lastError().text();
    return <xsl:value-of select="$className"/>();
  }
  if ( !query.next() ) {
    return <xsl:value-of select="$className"/>();
  }

  return <xsl:value-of select="$className"/>(
  <xsl:for-each select="column">
    query.value( <xsl:value-of select="position() - 1"/> ).value&lt;<xsl:value-of select="@type"/>&gt;()
    <xsl:if test="position() != last()">,</xsl:if>
  </xsl:for-each>
  );
</xsl:template>

<!-- result list extraction from a query-->
<xsl:template name="extract-result-list">
<xsl:variable name="className"><xsl:value-of select="@name"/></xsl:variable>

  QList&lt;<xsl:value-of select="$className"/>&gt; rv;
  while ( query.next() ) {
    rv.append( <xsl:value-of select="$className"/>(
      <xsl:for-each select="column">
        query.value( <xsl:value-of select="position() - 1"/> ).value&lt;<xsl:value-of select="@type"/>&gt;()
        <xsl:if test="position() != last()">,</xsl:if>
      </xsl:for-each>
    ) );
  }
  return rv;
</xsl:template>

</xsl:stylesheet>

