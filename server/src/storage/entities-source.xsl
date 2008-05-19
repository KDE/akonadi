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

<!-- table class source template -->
<xsl:template name="table-source">
<xsl:variable name="className"><xsl:value-of select="@name"/></xsl:variable>
<xsl:variable name="tableName"><xsl:value-of select="@name"/>Table</xsl:variable>
<xsl:variable name="entityName"><xsl:value-of select="@name"/></xsl:variable>

// private class
class <xsl:value-of select="$className"/>::Private : public QSharedData
{
  public:
    Private() : QSharedData() {}
    Private( const Private &amp; other ) : QSharedData( other )
    {
      <xsl:for-each select="column[@name != 'id']">
      <xsl:value-of select="@name"/> = other.<xsl:value-of select="@name"/>;
      <xsl:value-of select="@name"/>_changed = other.<xsl:value-of select="@name"/>_changed;
      </xsl:for-each>
    }
    ~Private() {};

    <xsl:for-each select="column[@name != 'id']">
    <xsl:value-of select="@type"/><xsl:text> </xsl:text><xsl:value-of select="@name"/>;
    bool <xsl:value-of select="@name"/>_changed;
    </xsl:for-each>

    static void addToCache( const <xsl:value-of select="$className"/> &amp; entry );

    // cache
    static bool cacheEnabled;
    static QMutex cacheMutex;
    <xsl:if test="column[@name = 'id']">
    static QHash&lt;qint64, <xsl:value-of select="$className"/> &gt; idCache;
    </xsl:if>
    <xsl:if test="column[@name = 'name']">
    static QHash&lt;<xsl:value-of select="column[@name = 'name']/@type"/>, <xsl:value-of select="$className"/> &gt; nameCache;
    </xsl:if>
};


// static members
bool <xsl:value-of select="$className"/>::Private::cacheEnabled = false;
QMutex <xsl:value-of select="$className"/>::Private::cacheMutex;
<xsl:if test="column[@name = 'id']">
QHash&lt;qint64, <xsl:value-of select="$className"/> &gt; <xsl:value-of select="$className"/>::Private::idCache;
</xsl:if>
<xsl:if test="column[@name = 'name']">
QHash&lt;<xsl:value-of select="column[@name = 'name']/@type"/>, <xsl:value-of select="$className"/> &gt; <xsl:value-of select="$className"/>::Private::nameCache;
</xsl:if>


void <xsl:value-of select="$className"/>::Private::addToCache( const <xsl:value-of select="$className"/> &amp; entry )
{
  Q_ASSERT( cacheEnabled );
  cacheMutex.lock();
  <xsl:if test="column[@name = 'id']">
  idCache.insert( entry.id(), entry );
  </xsl:if>
  <xsl:if test="column[@name = 'name']">
  nameCache.insert( entry.name(), entry );
  </xsl:if>
  cacheMutex.unlock();
}


// constructor
<xsl:value-of select="$className"/>::<xsl:value-of select="$className"/>() : Entity(),
  d( new Private )
{
<xsl:for-each select="column[@name != 'id']">
  d-&gt;<xsl:value-of select="@name"/>_changed = false;
</xsl:for-each>
}

<xsl:value-of select="$className"/>::<xsl:value-of select="$className"/>(
  <xsl:for-each select="column[@name != 'id']">
    <xsl:call-template name="argument"/><xsl:if test="position() != last()">, </xsl:if>
  </xsl:for-each>
) :
  Entity(),
  d( new Private )
{
<xsl:for-each select="column[@name != 'id']">
  d-&gt;<xsl:value-of select="@name"/> = <xsl:value-of select="@name"/>;
  d-&gt;<xsl:value-of select="@name"/>_changed = true;
</xsl:for-each>
}

<xsl:if test="column[@name = 'id']">
<xsl:value-of select="$className"/>::<xsl:value-of select="$className"/>(
  <xsl:for-each select="column">
    <xsl:call-template name="argument"/><xsl:if test="position() != last()">, </xsl:if>
  </xsl:for-each>
) :
  Entity( id ),
  d( new Private )
{
<xsl:for-each select="column[@name != 'id']">
  d-&gt;<xsl:value-of select="@name"/> = <xsl:value-of select="@name"/>;
  d-&gt;<xsl:value-of select="@name"/>_changed = true;
</xsl:for-each>
}
</xsl:if>

<xsl:value-of select="$className"/>::<xsl:value-of select="$className"/>( const <xsl:value-of select="$className"/> &amp; other )
  : Entity( other ), d( other.d )
{
}

// destructor
<xsl:value-of select="$className"/>::~<xsl:value-of select="$className"/>() {}

// assignment operator
<xsl:value-of select="$className"/>&amp; <xsl:value-of select="$className"/>::operator=( const <xsl:value-of select="$className"/> &amp; other )
{
  if ( this != &amp;other ) {
    d = other.d;
    setId( other.id() );
  }
  return *this;
}

// accessor methods
<xsl:for-each select="column[@name != 'id']">
<xsl:value-of select="@type"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::<xsl:value-of select="@name"/>() const
{
  <xsl:text>return d-&gt;</xsl:text><xsl:value-of select="@name"/>;
}

void <xsl:value-of select="$className"/>::<xsl:call-template name="setter-signature"/>
{
  d-&gt;<xsl:value-of select="@name"/> = <xsl:value-of select="@name"/>;
  d-&gt;<xsl:value-of select="@name"/>_changed = true;
}

</xsl:for-each>

// SQL table information
<xsl:text>QString </xsl:text><xsl:value-of select="$className"/>::tableName()
{
  return QLatin1String( "<xsl:value-of select="$tableName"/>" );
}

QStringList <xsl:value-of select="$className"/>::columnNames()
{
  QStringList rv;
  <xsl:for-each select="column">
  rv.append( QLatin1String( "<xsl:value-of select="@name"/>" ) );
  </xsl:for-each>
  return rv;
}

QStringList <xsl:value-of select="$className"/>::fullColumnNames()
{
  QStringList rv;
  <xsl:for-each select="column">
  rv.append( QLatin1String( "<xsl:value-of select="$tableName"/>.<xsl:value-of select="@name"/>" ) );
  </xsl:for-each>
  return rv;
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
bool <xsl:value-of select="$className"/>::exists( qint64 id )
{
  if ( Private::cacheEnabled ) {
    Private::cacheMutex.lock();
    if ( Private::idCache.contains( id ) ) {
      Private::cacheMutex.unlock();
      return true;
    }
    Private::cacheMutex.unlock();
  }
  return count( idColumn(), id ) > 0;
}
</xsl:if>
<xsl:if test="column[@name = 'name']">
bool <xsl:value-of select="$className"/>::exists( const <xsl:value-of select="column[@name = 'name']/@type"/> &amp;name )
{
  if ( Private::cacheEnabled ) {
    Private::cacheMutex.lock();
    if ( Private::nameCache.contains( name ) ) {
      Private::cacheMutex.unlock();
      return true;
    }
    Private::cacheMutex.unlock();
  }
  return count( nameColumn(), name ) > 0;
}
</xsl:if>


// result extraction
QList&lt; <xsl:value-of select="$className"/> &gt; <xsl:value-of select="$className"/>::extractResult( QSqlQuery &amp; query )
{
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
}

// data retrieval
<xsl:if test="column[@name='id']">
<xsl:value-of select="$className"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::retrieveById( qint64 id )
{
  <xsl:call-template name="data-retrieval">
  <xsl:with-param name="key">id</xsl:with-param>
  <xsl:with-param name="cache">idCache</xsl:with-param>
  </xsl:call-template>
}

</xsl:if>
<xsl:if test="column[@name = 'name']">
<xsl:value-of select="$className"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::retrieveByName( const <xsl:value-of select="column[@name = 'name']/@type"/> &amp;name )
{
  <xsl:call-template name="data-retrieval">
  <xsl:with-param name="key">name</xsl:with-param>
  <xsl:with-param name="cache">nameCache</xsl:with-param>
  </xsl:call-template>
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
  return extractResult( query );
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
  return extractResult( query );
}

// data retrieval for referenced tables
<xsl:for-each select="column[@refTable != '']">
<xsl:value-of select="@refTable"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::<xsl:call-template name="method-name-n1"/>() const
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

  return <xsl:value-of select="$rightSideClass"/>::extractResult( query );
}

// manipulate n:m relations
bool <xsl:value-of select="$className"/>::relatesTo<xsl:value-of select="@table2"/>( const <xsl:value-of select="$rightSideClass"/> &amp; value ) const
{
  return Entity::relatesTo&lt;<xsl:value-of select="$relationName"/>&gt;( id(), value.id() );
}

bool <xsl:value-of select="$className"/>::relatesTo<xsl:value-of select="@table2"/>( qint64 leftId, qint64 rightId )
{
  return Entity::relatesTo&lt;<xsl:value-of select="$relationName"/>&gt;( leftId, rightId );
}

bool <xsl:value-of select="$className"/>::add<xsl:value-of select="@table2"/>( const <xsl:value-of select="$rightSideClass"/> &amp; value ) const
{
  return Entity::addToRelation&lt;<xsl:value-of select="$relationName"/>&gt;( id(), value.id() );
}

bool <xsl:value-of select="$className"/>::add<xsl:value-of select="@table2"/>( qint64 leftId, qint64 rightId )
{
  return Entity::addToRelation&lt;<xsl:value-of select="$relationName"/>&gt;( leftId, rightId );
}

bool <xsl:value-of select="$className"/>::remove<xsl:value-of select="@table2"/>( const <xsl:value-of select="$rightSideClass"/> &amp; value ) const
{
  return Entity::removeFromRelation&lt;<xsl:value-of select="$relationName"/>&gt;( id(), value.id() );
}

bool <xsl:value-of select="$className"/>::remove<xsl:value-of select="@table2"/>( qint64 leftId, qint64 rightId )
{
  return Entity::removeFromRelation&lt;<xsl:value-of select="$relationName"/>&gt;( leftId, rightId );
}

bool <xsl:value-of select="$className"/>::clear<xsl:value-of select="@table2"/>s() const
{
  return Entity::clearRelation&lt;<xsl:value-of select="$relationName"/>&gt;( id() );
}

bool <xsl:value-of select="$className"/>::clear<xsl:value-of select="@table2"/>s( qint64 id )
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
bool <xsl:value-of select="$className"/>::insert( qint64* insertId )
{
  QSqlDatabase db = DataStore::self()->database();
  if ( !db.isOpen() )
    return false;

  QStringList cols, vals;
  <xsl:for-each select="column[@name != 'id']">
  if ( d-&gt;<xsl:value-of select="@name"/>_changed ) {
    cols.append( <xsl:value-of select="@name"/>Column() );
    vals.append( QLatin1String( ":<xsl:value-of select="@name"/>" ) );
  }
  </xsl:for-each>
  QString statement = QString::fromLatin1("INSERT INTO <xsl:value-of select="$tableName"/> (%1) VALUES (%2)")
    .arg( cols.join( QLatin1String(",") ), vals.join( QLatin1String(",") ) );

  QSqlQuery query( db );
  query.prepare( statement );
  <xsl:for-each select="column[@name != 'id']">
  if ( d-&gt;<xsl:value-of select="@name"/>_changed ) {
    query.bindValue( QLatin1String(":<xsl:value-of select="@name"/>"), this-&gt;<xsl:value-of select="@name"/>() );
  }
  </xsl:for-each>

  if ( !query.exec() ) {
    qDebug() &lt;&lt; "Error during insertion into table" &lt;&lt; tableName()
      &lt;&lt; query.lastError().text();
    return false;
  }

  setId( DataStore::self()->lastInsertId( query ) );
  if ( insertId )
    *insertId = id();
  return true;
}

// update existing data
bool <xsl:value-of select="$className"/>::update()
{
  invalidateCache();
  QSqlDatabase db = DataStore::self()->database();
  if ( !db.isOpen() )
    return false;

  QString statement = QLatin1String( "UPDATE " );
  statement += tableName();
  statement += QLatin1String( " SET " );

  QStringList cols;
  <xsl:for-each select="column[@name != 'id']">
  if ( d-&gt;<xsl:value-of select="@name"/>_changed )
    cols.append( <xsl:value-of select="@name"/>Column() + QLatin1String( " = :<xsl:value-of select="@name"/>" ) );;
  </xsl:for-each>
  statement += cols.join( QLatin1String( ", " ) );
  <xsl:if test="column[@name = 'id']">
  statement += QLatin1String( " WHERE id = :id" );
  </xsl:if>

  QSqlQuery query( db );
  query.prepare( statement );
  <xsl:for-each select="column[@name != 'id']">
  if ( d-&gt;<xsl:value-of select="@name"/>_changed ) {
    query.bindValue( QLatin1String(":<xsl:value-of select="@name"/>"), this-&gt;<xsl:value-of select="@name"/>() );
  }
  </xsl:for-each>

  <xsl:if test="column[@name = 'id']">
  query.bindValue( QLatin1String(":id"), id() );
  </xsl:if>
  if ( !query.exec() ) {
    qDebug() &lt;&lt; "Error during updating record with id" &lt;&lt; id()
             &lt;&lt; " in table" &lt;&lt; tableName() &lt;&lt; query.lastError().text();
    return false;
  }
  return true;
}

// delete records
bool <xsl:value-of select="$className"/>::remove( const QString &amp;column, const QVariant &amp;value )
{
  invalidateCompleteCache();
  return Entity::remove&lt;<xsl:value-of select="$className"/>&gt;( column, value );
}

<xsl:if test="column[@name = 'id']">
bool <xsl:value-of select="$className"/>::remove()
{
  invalidateCache();
  return Entity::remove&lt;<xsl:value-of select="$className"/>&gt;( idColumn(), id() );
}

bool <xsl:value-of select="$className"/>::remove( qint64 id )
{
  return remove( idColumn(), id );
}
</xsl:if>

// cache stuff
void <xsl:value-of select="$className"/>::invalidateCache() const
{
  if ( Private::cacheEnabled ) {
    Private::cacheMutex.lock();
    <xsl:if test="column[@name = 'id']">
    Private::idCache.remove( id() );
    </xsl:if>
    <xsl:if test="column[@name = 'name']">
    Private::nameCache.remove( name() );
    </xsl:if>
    Private::cacheMutex.unlock();
  }
}

void <xsl:value-of select="$className"/>::invalidateCompleteCache()
{
  if ( Private::cacheEnabled ) {
    Private::cacheMutex.lock();
    <xsl:if test="column[@name = 'id']">
    Private::idCache.clear();
    </xsl:if>
    <xsl:if test="column[@name = 'name']">
    Private::nameCache.clear();
    </xsl:if>
    Private::cacheMutex.unlock();
  }
}

void <xsl:value-of select="$className"/>::enableCache( bool enable )
{
  Private::cacheEnabled = enable;
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

</xsl:stylesheet>
