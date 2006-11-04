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

}
#endif

</xsl:if>

<!-- cpp generation -->
<xsl:if test="$code='source'">
#include &lt;entities.h&gt;
#include &lt;storage/datastore.h&gt;

#include &lt;qsqldatabase.h&gt;
#include &lt;QLatin1String&gt;
#include &lt;qsqlquery.h&gt;
#include &lt;qsqlerror.h&gt;
#include &lt;qvariant.h&gt;

using namespace Akonadi;

<xsl:for-each select="database/table">
<xsl:call-template name="table-source"/>
</xsl:for-each>

<xsl:for-each select="database/relation">
<xsl:call-template name="relation-source"/>
</xsl:for-each>

</xsl:if>

</xsl:template>



<!-- table class header template -->
<xsl:template name="table-header">
<xsl:variable name="className"><xsl:value-of select="@name"/></xsl:variable>
<xsl:variable name="entityName"><xsl:value-of select="@name"/></xsl:variable>

class <xsl:value-of select="$className"/> : public Entity
{
  friend class DataStore;

  public:
    // constructor
    <xsl:value-of select="$className"/>();
    <xsl:value-of select="$className"/>(
    <xsl:for-each select="column[@name != 'id']">
      <xsl:call-template name="argument"/><xsl:if test="position() != last()">, </xsl:if>
    </xsl:for-each> );
    <xsl:if test="column[@name = 'id']">
    <xsl:value-of select="$className"/>(
    <xsl:for-each select="column">
      <xsl:call-template name="argument"/><xsl:if test="position() != last()">, </xsl:if>
    </xsl:for-each> );
    </xsl:if>

    // accessor methods
    <xsl:for-each select="column[@name != 'id']">
    <xsl:value-of select="@type"/><xsl:text> </xsl:text><xsl:value-of select="@name"/>() const;
    void <xsl:call-template name="setter-signature"/>;
    </xsl:for-each>

    // SQL table information
    static QString tableName();
    <xsl:for-each select="column">
    static QString <xsl:value-of select="@name"/>Column();
    static QString <xsl:value-of select="@name"/>FullColumnName();
    </xsl:for-each>

    // count records
    static int count( const QString &amp;column, const QVariant &amp;value );

    // check existence
    <xsl:if test="column[@name = 'id']">
    static bool exists( int id );
    </xsl:if>
    <xsl:if test="column[@name = 'name']">
    static bool exists( const QString &amp;name );
    </xsl:if>

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
    <xsl:for-each select="../relation[@table1 = $entityName]">
    QList&lt;<xsl:value-of select="@table2"/>&gt; <xsl:value-of select="concat(translate(substring(@table2,1,1),'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz'), substring(@table2,2))"/>s() const;
    </xsl:for-each>

  protected:
    // inserting new data
    static bool insert( const <xsl:value-of select="$className"/>&amp; record, int* insertId = 0 );

    // updating existing data
    bool updateColumn( const QString &amp;column, const QVariant &amp;value ) const;

    // delete records
    static bool remove( const QString &amp;column, const QVariant &amp;value );

    // manipulate n:m relations
    <xsl:for-each select="../relation[@table1 = $entityName]">
    <xsl:variable name="rightSideClass"><xsl:value-of select="@table2"/></xsl:variable>
    bool relatesTo<xsl:value-of select="@table2"/>( const <xsl:value-of select="$rightSideClass"/> &amp; value ) const;
    static bool relatesTo<xsl:value-of select="@table2"/>( int leftId, int rightId );
    bool add<xsl:value-of select="@table2"/>( const <xsl:value-of select="$rightSideClass"/> &amp; value ) const;
    static bool add<xsl:value-of select="@table2"/>( int leftId, int rightId );
    bool remove<xsl:value-of select="@table2"/>( const <xsl:value-of select="$rightSideClass"/> &amp; value ) const;
    static bool remove<xsl:value-of select="@table2"/>( int leftId, int rightId );
    bool clear<xsl:value-of select="@table2"/>s() const;
    static bool clear<xsl:value-of select="@table2"/>s( int id );
    </xsl:for-each>

  private:
    // member variables
    <xsl:for-each select="column[@name != 'id']">
    <xsl:value-of select="@type"/><xsl:text> m_</xsl:text><xsl:value-of select="@name"/>;
    bool m_<xsl:value-of select="@name"/>_changed;
    </xsl:for-each>
};

// debug stream operator
QDebug &amp; operator&lt;&lt;( QDebug&amp; d, const <xsl:value-of select="$className"/>&amp; entity );
</xsl:template>


<!-- relation class header template -->
<xsl:template name="relation-header">
<xsl:variable name="className"><xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation</xsl:variable>

class <xsl:value-of select="$className"/>
{
  public:
    // SQL table information
    static QString tableName();
    static QString leftColumn();
    static QString leftFullColumnName();
    static QString rightColumn();
    static QString rightFullColumnName();
};
</xsl:template>



<!-- table class source template -->
<xsl:template name="table-source">
<xsl:variable name="className"><xsl:value-of select="@name"/></xsl:variable>
<xsl:variable name="tableName"><xsl:value-of select="@name"/>Table</xsl:variable>
<xsl:variable name="entityName"><xsl:value-of select="@name"/></xsl:variable>

// constructor
<xsl:value-of select="$className"/>::<xsl:value-of select="$className"/>() : Entity()
<xsl:for-each select="column[@name != 'id']">
, m_<xsl:value-of select="@name"/>_changed( false )
</xsl:for-each>
{
}

<xsl:value-of select="$className"/>::<xsl:value-of select="$className"/>(
  <xsl:for-each select="column[@name != 'id']">
    <xsl:call-template name="argument"/><xsl:if test="position() != last()">, </xsl:if>
  </xsl:for-each>
) :
  Entity()
<xsl:for-each select="column[@name != 'id']">
  , m_<xsl:value-of select="@name"/>( <xsl:value-of select="@name"/> )
  , m_<xsl:value-of select="@name"/>_changed( true )
</xsl:for-each>
{
}

<xsl:if test="column[@name = 'id']">
<xsl:value-of select="$className"/>::<xsl:value-of select="$className"/>(
  <xsl:for-each select="column">
    <xsl:call-template name="argument"/><xsl:if test="position() != last()">, </xsl:if>
  </xsl:for-each>
) :
  Entity( id )
<xsl:for-each select="column[@name != 'id']">
  , m_<xsl:value-of select="@name"/>( <xsl:value-of select="@name"/> )
  , m_<xsl:value-of select="@name"/>_changed( true )
</xsl:for-each>
{
}
</xsl:if>


// accessor methods
<xsl:for-each select="column[@name != 'id']">
<xsl:value-of select="@type"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::<xsl:value-of select="@name"/>() const
{
  <xsl:text>return m_</xsl:text><xsl:value-of select="@name"/>;
}

void <xsl:value-of select="$className"/>::<xsl:call-template name="setter-signature"/>
{
  m_<xsl:value-of select="@name"/> = <xsl:value-of select="@name"/>;
  m_<xsl:value-of select="@name"/>_changed = true;
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

QString <xsl:value-of select="$className"/>::<xsl:value-of select="@name"/>FullColumnName()
{
  return tableName() + QLatin1String( ".<xsl:value-of select="@name"/>" );
}
</xsl:for-each>


// count records
int <xsl:value-of select="$className"/>::count( const QString &amp;column, const QVariant &amp;value )
{
  return Entity::count&lt;<xsl:value-of select="$className"/>&gt;( column, value );
}

// check existence
<xsl:if test="column[@name = 'id']">
bool <xsl:value-of select="$className"/>::exists( int id )
{
  return count( idColumn(), id ) > 0;
}
</xsl:if>
<xsl:if test="column[@name = 'name']">
bool <xsl:value-of select="$className"/>::exists( const QString &amp;name )
{
  return count( nameColumn(), name ) > 0;
}
</xsl:if>


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

<!-- methods for n:m relations -->
<xsl:for-each select="../relation[@table1 = $entityName]">
<xsl:variable name="relationName"><xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation</xsl:variable>
<xsl:variable name="rightSideClass"><xsl:value-of select="@table2"/></xsl:variable>
<xsl:variable name="rightSideEntity"><xsl:value-of select="@table2"/></xsl:variable>
<xsl:variable name="rightSideTable"><xsl:value-of select="@table2"/>Table</xsl:variable>

// data retrieval for n:m relations
QList&lt;<xsl:value-of select="$rightSideClass"/>&gt; <xsl:value-of select="$className"/>::<xsl:value-of select="concat(translate(substring(@table2,1,1),'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz'), substring(@table2,2))"/>s() const
{
  QSqlDatabase db = DataStore::self()->database();
  if ( !db.isOpen() )
    return QList&lt;<xsl:value-of select="$rightSideClass"/>&gt;();

  QSqlQuery query( db );
  QString statement = QLatin1String( "SELECT " );
  <xsl:for-each select="/database/table[@name = $rightSideEntity]/column">
    statement.append( QLatin1String("<xsl:value-of select="$rightSideTable"/>.<xsl:value-of select="@name"/>" ) );
    <xsl:if test="position() != last()">
    statement.append( QLatin1String(", ") );
    </xsl:if>
  </xsl:for-each>
  statement.append( QLatin1String(" FROM <xsl:value-of select="$rightSideTable"/>, <xsl:value-of select="$relationName"/>") );
  statement.append( QLatin1String(" WHERE <xsl:value-of select="$relationName"/>.<xsl:value-of select="@table1"/>_<xsl:value-of select="@column1"/> = :key") );
  statement.append( QLatin1String(" AND <xsl:value-of select="$relationName"/>.<xsl:value-of select="@table2"/>_<xsl:value-of select="@column2"/> = <xsl:value-of select="$rightSideTable"/>.<xsl:value-of select="@column2"/>") );

  query.prepare( statement );
  query.bindValue( QLatin1String(":key"), id() );
  if ( !query.exec() ) {
    qDebug() &lt;&lt; "Error during selection of records from table <xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation"
      &lt;&lt; query.lastError().text();
    return QList&lt;<xsl:value-of select="$rightSideClass"/>&gt;();
  }
  QList&lt;<xsl:value-of select="$rightSideClass"/>&gt; rv;
  while ( query.next() ) {
    rv.append( <xsl:value-of select="$rightSideClass"/>(
    <xsl:for-each select="/database/table[@name= $rightSideEntity]/column">
      query.value( <xsl:value-of select="position() - 1"/> ).value&lt;<xsl:value-of select="@type"/>&gt;()
      <xsl:if test="position() != last()">,</xsl:if>
    </xsl:for-each>
    ) );
  }
  return rv;
}

// manipulate n:m relations
bool <xsl:value-of select="$className"/>::relatesTo<xsl:value-of select="@table2"/>( const <xsl:value-of select="$rightSideClass"/> &amp; value ) const
{
  return Entity::relatesTo&lt;<xsl:value-of select="$relationName"/>&gt;( id(), value.id() );
}

bool <xsl:value-of select="$className"/>::relatesTo<xsl:value-of select="@table2"/>( int leftId, int rightId )
{
  return Entity::relatesTo&lt;<xsl:value-of select="$relationName"/>&gt;( leftId, rightId );
}

bool <xsl:value-of select="$className"/>::add<xsl:value-of select="@table2"/>( const <xsl:value-of select="$rightSideClass"/> &amp; value ) const
{
  return Entity::addToRelation&lt;<xsl:value-of select="$relationName"/>&gt;( id(), value.id() );
}

bool <xsl:value-of select="$className"/>::add<xsl:value-of select="@table2"/>( int leftId, int rightId )
{
  return Entity::addToRelation&lt;<xsl:value-of select="$relationName"/>&gt;( leftId, rightId );
}

bool <xsl:value-of select="$className"/>::remove<xsl:value-of select="@table2"/>( const <xsl:value-of select="$rightSideClass"/> &amp; value ) const
{
  return Entity::removeFromRelation&lt;<xsl:value-of select="$relationName"/>&gt;( id(), value.id() );
}

bool <xsl:value-of select="$className"/>::remove<xsl:value-of select="@table2"/>( int leftId, int rightId )
{
  return Entity::removeFromRelation&lt;<xsl:value-of select="$relationName"/>&gt;( leftId, rightId );
}

bool <xsl:value-of select="$className"/>::clear<xsl:value-of select="@table2"/>s() const
{
  return Entity::clearRelation&lt;<xsl:value-of select="$relationName"/>&gt;( id() );
}

bool <xsl:value-of select="$className"/>::clear<xsl:value-of select="@table2"/>s( int id )
{
  return Entity::clearRelation&lt;<xsl:value-of select="$relationName"/>&gt;( id );
}

</xsl:for-each>

// debug stream operator
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

// inserting new data
bool <xsl:value-of select="$className"/>::insert( const <xsl:value-of select="$className"/>&amp; record, int* insertId )
{
  QSqlDatabase db = DataStore::self()->database();
  if ( !db.isOpen() )
    return false;

  QStringList cols, vals;
  <xsl:for-each select="column[@name != 'id']">
  if ( record.m_<xsl:value-of select="@name"/>_changed ) {
    cols.append( record.<xsl:value-of select="@name"/>Column() );
    vals.append( QLatin1String( ":<xsl:value-of select="@name"/>" ) );
  }
  </xsl:for-each>
  QString statement = QString::fromLatin1("INSERT INTO <xsl:value-of select="$tableName"/> (%1) VALUES (%2)")
    .arg( cols.join( QLatin1String(",") ), vals.join( QLatin1String(",") ) );

  QSqlQuery query( db );
  query.prepare( statement );
  <xsl:for-each select="column[@name != 'id']">
  if ( record.m_<xsl:value-of select="@name"/>_changed ) {
    query.bindValue( QLatin1String(":<xsl:value-of select="@name"/>"), record.<xsl:value-of select="@name"/>() );
  }
  </xsl:for-each>

  if ( !query.exec() ) {
    qDebug() &lt;&lt; "Error during insertion into table" &lt;&lt; tableName()
      &lt;&lt; query.lastError().text();
    return false;
  }

  if ( insertId )
    *insertId = DataStore::self()->lastInsertId( query );
  return true;
}

// update existing data
bool <xsl:value-of select="$className"/>::updateColumn ( const QString &amp;column, const QVariant &amp;value ) const
{
  return Entity::updateColumn&lt;<xsl:value-of select="$className"/>&gt;( column, value );
}

// delete records
bool <xsl:value-of select="$className"/>::remove( const QString &amp;column, const QVariant &amp;value )
{
  return Entity::remove&lt;<xsl:value-of select="$className"/>&gt;( column, value );
}

</xsl:template>


<!-- relation class source template -->
<xsl:template name="relation-source">
<xsl:variable name="className"><xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation</xsl:variable>
<xsl:variable name="tableName"><xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation</xsl:variable>

// SQL table information
QString <xsl:value-of select="$className"/>::tableName()
{
  return QLatin1String( "<xsl:value-of select="$tableName"/>" );
}

QString <xsl:value-of select="$className"/>::leftColumn()
{
  return QLatin1String( "<xsl:value-of select="@table1"/>_<xsl:value-of select="@column1"/>" );
}

QString <xsl:value-of select="$className"/>::leftFullColumnName()
{
  return tableName() + QLatin1String( "." ) + leftColumn();
}

QString <xsl:value-of select="$className"/>::rightColumn()
{
  return QLatin1String( "<xsl:value-of select="@table2"/>_<xsl:value-of select="@column2"/>" );
}

QString <xsl:value-of select="$className"/>::rightFullColumnName()
{
  return tableName() + QLatin1String( "." ) + rightColumn();
}
</xsl:template>



<!-- Helper templates -->

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

