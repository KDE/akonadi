<!--
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
class <xsl:value-of select="$className"/> : private Entity
{
    friend class DataStore;

public:
    /// List of <xsl:value-of select="$entityName"/> records.
    typedef QVector&lt;<xsl:value-of select="$className"/>&gt; List;

    // make some stuff accessible from Entity:
    using Entity::Id;
    using Entity::id;
    using Entity::setId;
    using Entity::isValid;
    using Entity::joinByName;
    using Entity::addToRelation;
    using Entity::removeFromRelation;

    <xsl:for-each select="enum">
    enum <xsl:value-of select="@name"/> {
      <xsl:for-each select="value">
      <xsl:value-of select="@name"/><xsl:if test="@value"> = <xsl:value-of select="@value"/></xsl:if>,
      </xsl:for-each>
    };
    </xsl:for-each>

    // constructor
    <xsl:value-of select="$className"/>();
    explicit <xsl:value-of select="$className"/>(
    <xsl:for-each select="column[@name != 'id']">
      <xsl:call-template name="argument"/><xsl:if test="position() != last()">, </xsl:if>
    </xsl:for-each> );
    <xsl:if test="column[@name = 'id']">
    explicit <xsl:value-of select="$className"/>(
    <xsl:for-each select="column">
      <xsl:call-template name="argument"/><xsl:if test="position() != last()">, </xsl:if>
    </xsl:for-each> );
    </xsl:if>
    <xsl:value-of select="$className"/>(const <xsl:value-of select="$className"/> &amp;other);

    // destructor
    ~<xsl:value-of select="$className"/>();

    /// assignment operator
    <xsl:value-of select="$className"/> &amp;operator=(const <xsl:value-of select="$className"/> &amp;other);

    /// comparisson operator, compares ids, not content
    bool operator==(const <xsl:value-of select="$className"/> &amp;other) const;

    // accessor methods
    <xsl:for-each select="column[@name != 'id']">
    /**
      Returns the value of the <xsl:value-of select="@name"/> column of this record.
      <xsl:value-of select="comment"/>
    */
    <xsl:call-template name="data-type"/><xsl:text> </xsl:text><xsl:value-of select="@name"/>() const;
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
    static QVector&lt;<xsl:value-of select="$className"/>&gt; extractResult(QSqlQuery &amp;query);

    /** Count records with value @p value in column @p column. */
    static int count(const QString &amp;column, const QVariant &amp;value);

    // check existence
    <xsl:if test="column[@name = 'id']">
    /** Checks if a record with id @p id exists. */
    static bool exists(qint64 id);
    </xsl:if>
    <xsl:if test="column[@name = 'name']">
    /** Checks if a record with name @name exists. */
    static bool exists(const <xsl:value-of select="column[@name = 'name']/@type"/> &amp;name);
    </xsl:if>

    // data retrieval
    <xsl:if test="column[@name = 'id']">
    /** Returns the record with id @p id. */
    <xsl:text>static </xsl:text><xsl:value-of select="$className"/> retrieveById(qint64 id);
    </xsl:if>

    <xsl:if test="column[@name = 'name'] and $className != 'PartType'">
    /** Returns the record with name @p name. */
    <xsl:text>static </xsl:text><xsl:value-of select="$className"/> retrieveByName(const <xsl:value-of select="column[@name = 'name']/@type"/> &amp;name);

    /** Returns the record with name @p name. If such record does not exist,
        it will be created. This method is thread-safe, so if multiple callers
        call it on non-existent name, only one will create the new record, others
        will wait and read it from the cache. */
    <xsl:text>static </xsl:text><xsl:value-of select="$className"/> retrieveByNameOrCreate(const <xsl:value-of select="column[@name = 'name']/@type"/> &amp;name);
    </xsl:if>

    <xsl:if test="column[@name = 'name'] and $className = 'PartType'">
    <!-- Special case for PartTypes, which are identified by "NS:NAME" -->
    <xsl:text>static PartType retrieveByFQName(const QString &amp;ns, const QString &amp;name);</xsl:text>
    <xsl:text>static PartType retrieveByFQNameOrCreate(const QString &amp;ns, const QString &amp;name);</xsl:text>
    </xsl:if>

    /** Retrieve all records from this table. */
    static <xsl:value-of select="$className"/>::List retrieveAll();
    /** Retrieve all records with value @p value in column @p key. */
    static <xsl:value-of select="$className"/>::List retrieveFiltered(const QString &amp;key, const QVariant &amp;value);

    <!-- data retrieval for referenced tables (n:1) -->
    <xsl:for-each select="column[@refTable != '']">
    <xsl:variable name="method-name"><xsl:call-template name="method-name-n1"/></xsl:variable>
    /**
      Retrieve the <xsl:value-of select="@refTable"/> record referred to by the
      <xsl:value-of select="@name"/> column of this record.
    */
    <xsl:value-of select="@refTable"/><xsl:text> </xsl:text><xsl:value-of select="$method-name"/>() const;

    /**
      Set the  <xsl:value-of select="@refTable"/> record referred to by the
      <xsl:value-of select="@name"/> column of this record.
    */
    void set<xsl:call-template name="uppercase-first"><xsl:with-param name="argument"><xsl:value-of select="$method-name"/></xsl:with-param></xsl:call-template>
      (const <xsl:value-of select="@refTable"/> &amp;value);
    </xsl:for-each>

    <!-- data retrieval for inverse referenced tables (1:n) -->
    <xsl:for-each select="reference">
    /**
      Retrieve a list of all <xsl:value-of select="@table"/> records referring to this record
      in their column <xsl:value-of select="@key"/>.
    */
    QVector&lt;<xsl:value-of select="@table"/>&gt; <xsl:value-of select="@name"/>() const;
    </xsl:for-each>

    // data retrieval for n:m relations
    <xsl:for-each select="../relation[@table1 = $entityName]">
    QVector&lt;<xsl:value-of select="@table2"/>&gt; <xsl:value-of select="concat(translate(substring(@table2,1,1),'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz'), substring(@table2,2))"/>s() const;
    </xsl:for-each>

    /**
      Inserts this record into the DataStore.
      @param insertId pointer to an int, filled with the identifier of this record on success.
    */
    bool insert(qint64 *insertId = nullptr);

    /**
      Returns @c true if this record has any pending changes.
    */
    bool hasPendingChanges() const;

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
    static bool remove(qint64 id);
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
    static void enableCache(bool enable);

    // manipulate n:m relations
    <xsl:for-each select="../relation[@table1 = $entityName]">
    <xsl:variable name="rightSideClass"><xsl:value-of select="@table2"/></xsl:variable>
    /**
      Checks wether this record is in a n:m relation with the <xsl:value-of select="@table2"/> @p value.
    */
    bool relatesTo<xsl:value-of select="@table2"/>(const <xsl:value-of select="$rightSideClass"/> &amp;value) const;
    static bool relatesTo<xsl:value-of select="@table2"/>(qint64 leftId, qint64 rightId);

    /**
      Adds a n:m relation between this record and the <xsl:value-of select="@table2"/> @p value.
    */
    bool add<xsl:value-of select="@table2"/>(const <xsl:value-of select="$rightSideClass"/> &amp;value) const;
    static bool add<xsl:value-of select="@table2"/>(qint64 leftId, qint64 rightId);

    /**
      Removes a n:m relation between this record and the <xsl:value-of select="@table2"/> @p value.
    */
    bool remove<xsl:value-of select="@table2"/>(const <xsl:value-of select="$rightSideClass"/> &amp;value) const;
    static bool remove<xsl:value-of select="@table2"/>(qint64 leftId, qint64 rightId);

    /**
      Removes all relations between this record and any <xsl:value-of select="@table2"/>.
    */
    bool clear<xsl:value-of select="@table2"/>s() const;
    static bool clear<xsl:value-of select="@table2"/>s(qint64 id);
    </xsl:for-each>

    // delete records
    static bool remove(const QString &amp;column, const QVariant &amp;value);

private:
    class Private;
    QSharedDataPointer&lt;Private&gt; d;
};
</xsl:template>

<!-- debug stream operators -->
<xsl:template name="table-debug-header">
<xsl:variable name="className"><xsl:value-of select="@name"/></xsl:variable>

#ifndef QT_NO_DEBUG_STREAM
// debug stream operator
QDebug &amp;operator&lt;&lt;(QDebug &amp;d, const Akonadi::Server::<xsl:value-of select="$className"/> &amp;entity);
#endif
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
