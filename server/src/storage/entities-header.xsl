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

<!-- table class header template -->
<xsl:template name="table-header">
<xsl:variable name="className"><xsl:value-of select="@name"/></xsl:variable>
<xsl:variable name="entityName"><xsl:value-of select="@name"/></xsl:variable>

/**
  Representation of a record in the <xsl:value-of select="$entityName"/> table.
  <xsl:if test="comment != ''">
  &lt;br&gt;
  <xsl:value-of select="comment"/>
  </xsl:if>
  This class is implicitly shared.
*/
class AKONADI_SERVER_EXPORT <xsl:value-of select="$className"/> : public Entity
{
  friend class DataStore;

  public:
    /// List of <xsl:value-of select="$entityName"/> records.
    typedef QList&lt;<xsl:value-of select="$className"/>&gt; List;

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
    <xsl:value-of select="$className"/>( const <xsl:value-of select="$className"/> &amp; other );

    // destructor
    ~<xsl:value-of select="$className"/>();

    /// assignment operator
    <xsl:value-of select="$className"/>&amp; operator=( const <xsl:value-of select="$className"/> &amp; other );

    // accessor methods
    <xsl:for-each select="column[@name != 'id']">
    /**
      Returns the value of the <xsl:value-of select="@name"/> column of this record.
      <xsl:value-of select="comment"/>
    */
    <xsl:value-of select="@type"/><xsl:text> </xsl:text><xsl:value-of select="@name"/>() const;
    /**
      Sets the value of the <xsl:value-of select="@name"/> column of this record.
      <xsl:value-of select="comment"/>
    */
    void <xsl:call-template name="setter-signature"/>;
    </xsl:for-each>

    /** Returns the name of the SQL table. */
    static QString tableName();

    /**
      Returns a list of all SQL column names. The names are in the correct
      order for usage with extractResult().
    */
    static QStringList columnNames();

    /**
      Returns a list of all SQL column names prefixed with their tables names.
      The names are in the correct order for usage with extractResult().
    */
    static QStringList fullColumnNames();

    <xsl:for-each select="column">
    static QString <xsl:value-of select="@name"/>Column();
    static QString <xsl:value-of select="@name"/>FullColumnName();
    </xsl:for-each>

    /**
      Extracts the query result.
      @param query A executed query containing a list of <xsl:value-of select="$entityName"/> records.
      Note that the fields need to be in the correct order (same as in the constructor)!
    */
    static QList&lt; <xsl:value-of select="$className"/> &gt; extractResult( QSqlQuery&amp; query );

    /** Count records with value @p value in column @p column. */
    static int count( const QString &amp;column, const QVariant &amp;value );

    // check existence
    <xsl:if test="column[@name = 'id']">
    /** Checks if a record with id @p id exists. */
    static bool exists( int id );
    </xsl:if>
    <xsl:if test="column[@name = 'name']">
    /** Checks if a record with name @name exists. */
    static bool exists( const QString &amp;name );
    </xsl:if>

    // data retrieval
    <xsl:if test="column[@name = 'id']">
    /** Returns the record with id @p id. */
    <xsl:text>static </xsl:text><xsl:value-of select="$className"/> retrieveById( int id );
    </xsl:if>

    <xsl:if test="column[@name = 'name']">
    /** Returns the record with name @p name. */
    <xsl:text>static </xsl:text><xsl:value-of select="$className"/> retrieveByName( const QString &amp;name );
    </xsl:if>

    /** Retrieve all records from this table. */
    static <xsl:value-of select="$className"/>::List retrieveAll();
    /** Retrieve all records with value @p value in column @p key. */
    static <xsl:value-of select="$className"/>::List retrieveFiltered( const QString &amp;key, const QVariant &amp;value );

    <!-- data retrieval for referenced tables (n:1) -->
    <xsl:for-each select="column[@refTable != '']">
    /**
      Retrieve the <xsl:value-of select="@refTable"/> record referred to by the
      <xsl:value-of select="@name"/> column of this record.
    */
    <xsl:value-of select="@refTable"/><xsl:text> </xsl:text><xsl:call-template name="method-name-n1"/>() const;
    </xsl:for-each>

    <!-- data retrieval for inverse referenced tables (1:n) -->
    <xsl:for-each select="reference">
    /**
      Retrieve a list of all <xsl:value-of select="@table"/> records referring to this record
      in their column <xsl:value-of select="@key"/>.
    */
    QList&lt;<xsl:value-of select="@table"/>&gt; <xsl:value-of select="@name"/>() const;
    </xsl:for-each>

    // data retrieval for n:m relations
    <xsl:for-each select="../relation[@table1 = $entityName]">
    QList&lt;<xsl:value-of select="@table2"/>&gt; <xsl:value-of select="concat(translate(substring(@table2,1,1),'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz'), substring(@table2,2))"/>s() const;
    </xsl:for-each>

    /**
      Inserts this record into the DataStore.
      @param insertId pointer to an int, filled with the identifier of this record on success.
    */
    bool insert( int* insertId = 0 );

    /**
      Stores all changes made to this record into the database.
      Note that this method assumes the existence of an 'id' column to identify
      the record to update. If that column does not exist, all records will be
      changed.
      @returns true on success, false otherwise.
    */
    bool update();

    <xsl:if test="column[@name = 'id']">
    /** Deletes this record. */
    bool remove();

    /** Deletes the record with the given id. */
    static bool remove( int id );
    </xsl:if>

    /**
      Invalidates the cache entry for this record.
      This method has no effect if caching is not enabled for this table.
    */
    void invalidateCache() const;

    /**
      Invalidates all cache entries for this table.
      This method has no effect if caching is not enabled for this table.
    */
    static void invalidateCompleteCache();

    /**
      Enable/disable caching for this table.
      This method is not thread-safe, call before activating multi-threading.
    */
    static void enableCache( bool enable );

  protected:
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
    class Private;
    QSharedDataPointer&lt;Private&gt; d;
};
</xsl:template>

<!-- debug stream operators -->
<xsl:template name="table-debug-header">
<xsl:variable name="className"><xsl:value-of select="@name"/></xsl:variable>

// debug stream operator
AKONADI_SERVER_EXPORT QDebug &amp; operator&lt;&lt;( QDebug&amp; d, const Akonadi::<xsl:value-of select="$className"/>&amp; entity );
</xsl:template>


<!-- relation class header template -->
<xsl:template name="relation-header">
<xsl:variable name="className"><xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation</xsl:variable>

<xsl:if test="comment != ''">
/**
  <xsl:value-of select="comment"/>
*/
</xsl:if>
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

</xsl:stylesheet>
