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
    Private() : QSharedData()
    <!-- BEGIN Variable Initializers - order as Variable Declarations below -->
    <xsl:for-each select="column[@type = 'qint64' and @name != 'id']">
      , <xsl:value-of select="@name"/>(<xsl:choose><xsl:when test="@default"><xsl:value-of select="@default"/></xsl:when><xsl:otherwise>0</xsl:otherwise></xsl:choose>)
    </xsl:for-each>
    <xsl:if test="column[@type = 'QDateTime']">
    </xsl:if>
    <xsl:for-each select="column[@type = 'QString']">
      , <xsl:value-of select="@name"/>()
    </xsl:for-each>
    <xsl:for-each select="column[@type = 'QByteArray']">
      , <xsl:value-of select="@name"/>()
    </xsl:for-each>
    <xsl:if test="column[@type = 'QDateTime']">
    // on non-wince, QDateTime is one int
    <xsl:for-each select="column[@type = 'QDateTime']">
      , <xsl:value-of select="@name"/>()
    </xsl:for-each>
    </xsl:if>
    <xsl:for-each select="column[@type = 'int']">
      , <xsl:value-of select="@name"/>(<xsl:choose><xsl:when test="@default"><xsl:value-of select="@default"/></xsl:when><xsl:otherwise>0</xsl:otherwise></xsl:choose>)
    </xsl:for-each>
    <xsl:for-each select="column[@type = 'bool']">
      , <xsl:value-of select="@name"/>(<xsl:choose><xsl:when test="@default"><xsl:value-of select="@default"/></xsl:when><xsl:otherwise>false</xsl:otherwise></xsl:choose>)
    </xsl:for-each>
    <xsl:for-each select="column[@type = 'enum']">
      , <xsl:value-of select="@name"/>(<xsl:choose><xsl:when test="@default"><xsl:value-of select="@default"/></xsl:when><xsl:otherwise>0</xsl:otherwise></xsl:choose>)
    </xsl:for-each>
    <xsl:for-each select="column[@name != 'id']">
      , <xsl:value-of select="@name"/>_changed(false)
    </xsl:for-each>
    <!-- END Variable Initializers - order as Variable Declarations below -->
    {}

    <!-- BEGIN Variable Declarations - order by decreasing sizeof() -->
    <xsl:for-each select="column[@type = 'qint64' and @name != 'id']">
    qint64 <xsl:value-of select="@name"/>;
    </xsl:for-each>
    <xsl:if test="column[@type = 'QDateTime']">
    </xsl:if>
    <xsl:for-each select="column[@type = 'QString']">
    QString <xsl:value-of select="@name"/>;
    </xsl:for-each>
    <xsl:for-each select="column[@type = 'QByteArray']">
    QByteArray <xsl:value-of select="@name"/>;
    </xsl:for-each>
    <xsl:if test="column[@type = 'QDateTime']">
    // on non-wince, QDateTime is one int
    <xsl:for-each select="column[@type = 'QDateTime']">
    QDateTime <xsl:value-of select="@name"/>;
    </xsl:for-each>
    </xsl:if>
    <xsl:for-each select="column[@type = 'int']">
    int <xsl:value-of select="@name"/>;
    </xsl:for-each>
    <xsl:for-each select="column[@type = 'bool']">
    bool <xsl:value-of select="@name"/> : 1;
    </xsl:for-each>
    <xsl:for-each select="column[@type = 'Tristate']">
    Tristate <xsl:value-of select="@name"/>;
    </xsl:for-each>
    <xsl:for-each select="column[@type = 'enum']">
    <xsl:value-of select="@enumType"/><xsl:text> </xsl:text><xsl:value-of select="@name"/>;
    </xsl:for-each>
    <xsl:for-each select="column[@name != 'id']">
    bool <xsl:value-of select="@name"/>_changed : 1;
    </xsl:for-each>
    <!-- END Variable Declarations - order by decreasing sizeof() -->

    static void addToCache(const <xsl:value-of select="$className"/> &amp;entry);

    // cache
    static QAtomicInt cacheEnabled;
    static QMutex cacheMutex;
    <xsl:if test="column[@name = 'id']">
    static QHash&lt;qint64, <xsl:value-of select="$className"/>&gt; idCache;
    </xsl:if>
    <xsl:if test="column[@name = 'name']">
    static QHash&lt;<xsl:value-of select="column[@name = 'name']/@type"/>, <xsl:value-of select="$className"/>&gt; nameCache;
    </xsl:if>
};


// static members
QAtomicInt <xsl:value-of select="$className"/>::Private::cacheEnabled(0);
QMutex <xsl:value-of select="$className"/>::Private::cacheMutex;
<xsl:if test="column[@name = 'id']">
QHash&lt;qint64, <xsl:value-of select="$className"/>&gt; <xsl:value-of select="$className"/>::Private::idCache;
</xsl:if>
<xsl:if test="column[@name = 'name']">
QHash&lt;<xsl:value-of select="column[@name = 'name']/@type"/>, <xsl:value-of select="$className"/>&gt; <xsl:value-of select="$className"/>::Private::nameCache;
</xsl:if>


void <xsl:value-of select="$className"/>::Private::addToCache(const <xsl:value-of select="$className"/> &amp;entry)
{
    Q_ASSERT(cacheEnabled);
    Q_UNUSED(entry); <!-- in case the table has neither an id nor name column -->
    QMutexLocker lock(&amp;cacheMutex);
    <xsl:if test="column[@name = 'id']">
    idCache.insert(entry.id(), entry);
    </xsl:if>
    <xsl:if test="column[@name = 'name']">
      <xsl:choose>
        <xsl:when test="$className = 'PartType'">
        <!-- special case for PartType, which is identified as "NS:NAME" -->
    nameCache.insert(entry.ns() + QLatin1Char(':') + entry.name(), entry);
        </xsl:when>
        <xsl:otherwise>
    nameCache.insert(entry.name(), entry);
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
}


// constructor
<xsl:value-of select="$className"/>::<xsl:value-of select="$className"/>() : Entity(),
    d(new Private)
{
}

<xsl:value-of select="$className"/>::<xsl:value-of select="$className"/>(
  <xsl:for-each select="column[@name != 'id']">
    <xsl:call-template name="argument"/><xsl:if test="position() != last()">, </xsl:if>
  </xsl:for-each>
) :
    Entity(),
    d(new Private)
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
    Entity(id),
    d(new Private)
{
<xsl:for-each select="column[@name != 'id']">
    d-&gt;<xsl:value-of select="@name"/> = <xsl:value-of select="@name"/>;
    d-&gt;<xsl:value-of select="@name"/>_changed = true;
</xsl:for-each>
}
</xsl:if>

<xsl:value-of select="$className"/>::<xsl:value-of select="$className"/>(const <xsl:value-of select="$className"/> &amp;other)
    : Entity(other), d(other.d)
{
}

// destructor
<xsl:value-of select="$className"/>::~<xsl:value-of select="$className"/>() {}

// assignment operator
<xsl:value-of select="$className"/>&amp; <xsl:value-of select="$className"/>::operator=(const <xsl:value-of select="$className"/> &amp;other)
{
    if (this != &amp;other) {
        d = other.d;
        setId(other.id());
    }
    return *this;
}

// comparisson operator
bool <xsl:value-of select="$className"/>::operator==(const <xsl:value-of select="$className"/> &amp;other) const
{
    return id() == other.id();
}

// accessor methods
<xsl:for-each select="column[@name != 'id']">
<xsl:call-template name="data-type"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::<xsl:value-of select="@name"/>() const
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
    static const QString tableName = QStringLiteral("<xsl:value-of select="$tableName"/>");
    return tableName;
}

QStringList <xsl:value-of select="$className"/>::columnNames()
{
    static const QStringList columns = {
        <xsl:for-each select="column">
        <xsl:value-of select="@name"/>Column()
        <xsl:if test="position() != last()">,</xsl:if>
        </xsl:for-each>
    };
    return columns;
}

QStringList <xsl:value-of select="$className"/>::fullColumnNames()
{
    static const QStringList columns = {
        <xsl:for-each select="column">
        <xsl:value-of select="@name"/>FullColumnName()
        <xsl:if test="position() != last()">,</xsl:if>
        </xsl:for-each>
    };
    return columns;
}

<xsl:for-each select="column">
QString <xsl:value-of select="$className"/>::<xsl:value-of select="@name"/>Column()
{
    static const QString column = QStringLiteral("<xsl:value-of select="@name"/>");
    return column;
}

QString <xsl:value-of select="$className"/>::<xsl:value-of select="@name"/>FullColumnName()
{
    static const QString column = QStringLiteral("<xsl:value-of select="$tableName"/>.<xsl:value-of select="@name"/>");
    return column;
}
</xsl:for-each>


// count records
int <xsl:value-of select="$className"/>::count(const QString &amp;column, const QVariant &amp;value)
{
    return Entity::count&lt;<xsl:value-of select="$className"/>&gt;(column, value);
}

// check existence
<xsl:if test="column[@name = 'id']">
bool <xsl:value-of select="$className"/>::exists(qint64 id)
{
    if (Private::cacheEnabled) {
        QMutexLocker lock(&amp;Private::cacheMutex);
        if (Private::idCache.contains(id)) {
            return true;
        }
    }
    return count(idColumn(), id) > 0;
}
</xsl:if>
<xsl:if test="column[@name = 'name']">
bool <xsl:value-of select="$className"/>::exists(const <xsl:value-of select="column[@name = 'name']/@type"/> &amp;name)
{
    if (Private::cacheEnabled) {
        QMutexLocker lock(&amp;Private::cacheMutex);
        if (Private::nameCache.contains(name)) {
            return true;
        }
    }
    return count(nameColumn(), name) > 0;
}
</xsl:if>


// result extraction
QVector&lt;<xsl:value-of select="$className"/>&gt; <xsl:value-of select="$className"/>::extractResult(QSqlQuery &amp;query)
{
    QVector&lt;<xsl:value-of select="$className"/>&gt; rv;
    if (query.driver()->hasFeature(QSqlDriver::QuerySize)) {
        rv.reserve(query.size());
    }
    while (query.next()) {
        rv.append(<xsl:value-of select="$className"/>(
        <xsl:for-each select="column">
              (query.isNull(<xsl:value-of select="position() - 1"/>)
                ? <xsl:call-template name="data-type"/>()
                <xsl:text>: </xsl:text>
                <xsl:choose>
                <xsl:when test="starts-with(@type,'QString')">
                  <xsl:text>Utils::variantToString(query.value(</xsl:text><xsl:value-of select="position() - 1"/><xsl:text>))</xsl:text>
                </xsl:when>
                <xsl:when test="starts-with(@type, 'enum')">
                  <xsl:text>static_cast&lt;</xsl:text><xsl:value-of select="@enumType"/>&gt;(query.value(<xsl:value-of select="position() - 1"/><xsl:text>).value&lt;int&gt;())</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:text>query.value(</xsl:text><xsl:value-of select="position() - 1"/>).value&lt;<xsl:value-of select="@type"/>&gt;<xsl:text>()</xsl:text>
                </xsl:otherwise>
              </xsl:choose>
              <xsl:text>)</xsl:text><xsl:if test="position() != last()"><xsl:text>,</xsl:text></xsl:if>
            </xsl:for-each>
            ));
    }
    return rv;
}

// data retrieval
<xsl:if test="column[@name='id']">
<xsl:value-of select="$className"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::retrieveById(qint64 id)
{
    <xsl:call-template name="data-retrieval">
      <xsl:with-param name="key">id</xsl:with-param>
      <xsl:with-param name="cache">idCache</xsl:with-param>
    </xsl:call-template>
}

</xsl:if>
<xsl:if test="column[@name = 'name'] and $className != 'PartType'">
<xsl:value-of select="$className"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::retrieveByName(const <xsl:value-of select="column[@name = 'name']/@type"/> &amp;name)
{
    <xsl:call-template name="data-retrieval">
      <xsl:with-param name="key">name</xsl:with-param>
      <xsl:with-param name="cache">nameCache</xsl:with-param>
    </xsl:call-template>
}

<xsl:value-of select="$className"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::retrieveByNameOrCreate(const <xsl:value-of select="column[@name = 'name']/@type"/> &amp;name)
{
    static QMutex lock;
    auto rv = retrieveByName(name);
    if (rv.isValid()) {
        return rv;
    }

    if (lock.tryLock()) {
        rv.setName(name);
        if (!rv.insert()) {
            lock.unlock();
            return <xsl:value-of select="$className"/>();
        }

        if (Private::cacheEnabled) {
            Private::addToCache(rv);
        }
        lock.unlock();
        return rv;
    }

    lock.lock();
    lock.unlock();
    return retrieveByName(name);
}
</xsl:if>

<xsl:if test="column[@name = 'name'] and $className = 'PartType'">
<xsl:text>PartType PartType::retrieveByFQName(const QString &amp;ns, const QString &amp;name)</xsl:text>
{
    const QString fqname = ns + QLatin1Char(':') + name;
    <xsl:call-template name="data-retrieval">
      <xsl:with-param name="key">ns</xsl:with-param>
      <xsl:with-param name="key2">name</xsl:with-param>
      <xsl:with-param name="lookupKey">fqname</xsl:with-param>
      <xsl:with-param name="cache">nameCache</xsl:with-param>
    </xsl:call-template>
}

<xsl:text>PartType PartType::retrieveByFQNameOrCreate(const QString &amp;ns, const QString &amp;name)</xsl:text>
{
    static QMutex lock;
    PartType rv = retrieveByFQName(ns, name);
    if (rv.isValid()) {
        return rv;
    }

    if (lock.tryLock()) {
        rv.setNs(ns);
        rv.setName(name);
        if (!rv.insert()) {
            lock.unlock();
            return PartType();
        }

        if (Private::cacheEnabled) {
            Private::addToCache(rv);
        }
        lock.unlock();
        return rv;
    }

    lock.lock();
    lock.unlock();
    return retrieveByFQName(ns, name);
}
</xsl:if>

QVector&lt;<xsl:value-of select="$className"/>&gt; <xsl:value-of select="$className"/>::retrieveAll()
{
    QSqlDatabase db = DataStore::self()->database();
    if (!db.isOpen()) {
        return {};
    }

    QueryBuilder qb(tableName(), QueryBuilder::Select);
    qb.addColumns(columnNames());
    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) &lt;&lt; "Error during selection of all records from table" &lt;&lt; tableName()
                                     &lt;&lt; qb.query().lastError().text() &lt;&lt; qb.query().lastQuery();
        return {};
    }
    return extractResult(qb.query());
}

QVector&lt;<xsl:value-of select="$className"/>&gt; <xsl:value-of select="$className"/>::retrieveFiltered(const QString &amp;key, const QVariant &amp;value)
{
    QSqlDatabase db = DataStore::self()->database();
    if (!db.isOpen()) {
        return {};
    }

    SelectQueryBuilder&lt;<xsl:value-of select="$className"/>&gt; qb;
    if (value.isNull()) {
        qb.addValueCondition(key, Query::Is, QVariant());
    } else {
        qb.addValueCondition(key, Query::Equals, value);
    }
    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) &lt;&lt; "Error during selection of records from table" &lt;&lt; tableName()
                                     &lt;&lt; "filtered by" &lt;&lt; key &lt;&lt; "=" &lt;&lt; value
                                     &lt;&lt; qb.query().lastError().text();
        return {};
    }
    return qb.result();
}

// data retrieval for referenced tables
<xsl:for-each select="column[@refTable != '']">
<xsl:variable name="method-name"><xsl:call-template name="method-name-n1"/></xsl:variable>
<xsl:value-of select="@refTable"/><xsl:text> </xsl:text><xsl:value-of select="$className"/>::<xsl:value-of select="$method-name"/>() const
{
    return <xsl:value-of select="@refTable"/>::retrieveById(<xsl:value-of select="@name"/>());
}

void <xsl:value-of select="$className"/>::
    set<xsl:call-template name="uppercase-first"><xsl:with-param name="argument"><xsl:value-of select="$method-name"/></xsl:with-param></xsl:call-template>
    (const <xsl:value-of select="@refTable"/> &amp;value)
{
    d-&gt;<xsl:value-of select="@name"/> = value.id();
    d-&gt;<xsl:value-of select="@name"/>_changed = true;
}
</xsl:for-each>

// data retrieval for inverse referenced tables
<xsl:for-each select="reference">
QVector&lt;<xsl:value-of select="@table"/>&gt; <xsl:value-of select="$className"/>::<xsl:value-of select="@name"/>() const
{
    return <xsl:value-of select="@table"/>::retrieveFiltered(<xsl:value-of select="@table"/>::<xsl:value-of select="@key"/>Column(), id());
}
</xsl:for-each>

<!-- methods for n:m relations -->
<xsl:for-each select="../relation[@table1 = $entityName]">
<xsl:variable name="relationName"><xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation</xsl:variable>
<xsl:variable name="rightSideClass"><xsl:value-of select="@table2"/></xsl:variable>
<xsl:variable name="rightSideEntity"><xsl:value-of select="@table2"/></xsl:variable>

// data retrieval for n:m relations
QVector&lt;<xsl:value-of select="$rightSideClass"/>&gt; <xsl:value-of select="$className"/>::<xsl:value-of select="concat(translate(substring(@table2,1,1),'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz'), substring(@table2,2))"/>s() const
{
    QSqlDatabase db = DataStore::self()->database();
    if (!db.isOpen()) {
        return {};
    }

    QueryBuilder qb(<xsl:value-of select="$rightSideClass"/>::tableName(), QueryBuilder::Select);
    static const QStringList columns = {
    <xsl:for-each select="/database/table[@name = $rightSideEntity]/column">
        <xsl:value-of select="$rightSideClass"/>::<xsl:value-of select="@name"/>FullColumnName()
        <xsl:if test="position() != last()">,</xsl:if>
    </xsl:for-each>
    };
    qb.addColumns(columns);
    qb.addJoin(QueryBuilder::InnerJoin, <xsl:value-of select="$relationName"/>::tableName(),
            <xsl:value-of select="$relationName"/>::rightFullColumnName(),
            <xsl:value-of select="$rightSideClass"/>::<xsl:value-of select="@column2"/>FullColumnName());
    qb.addValueCondition(<xsl:value-of select="$relationName"/>::leftFullColumnName(), Query::Equals, id());

    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) &lt;&lt; "Error during selection of records from table <xsl:value-of select="@table1"/><xsl:value-of select="@table2"/>Relation"
                                     &lt;&lt; qb.query().lastError().text();
        return {};
    }

    return <xsl:value-of select="$rightSideClass"/>::extractResult(qb.query());
}

// manipulate n:m relations
bool <xsl:value-of select="$className"/>::relatesTo<xsl:value-of select="@table2"/>(const <xsl:value-of select="$rightSideClass"/> &amp;value) const
{
    return Entity::relatesTo&lt;<xsl:value-of select="$relationName"/>&gt;(id(), value.id());
}

bool <xsl:value-of select="$className"/>::relatesTo<xsl:value-of select="@table2"/>(qint64 leftId, qint64 rightId)
{
    return Entity::relatesTo&lt;<xsl:value-of select="$relationName"/>&gt;(leftId, rightId);
}

bool <xsl:value-of select="$className"/>::add<xsl:value-of select="@table2"/>(const <xsl:value-of select="$rightSideClass"/> &amp;value) const
{
    return Entity::addToRelation&lt;<xsl:value-of select="$relationName"/>&gt;(id(), value.id());
}

bool <xsl:value-of select="$className"/>::add<xsl:value-of select="@table2"/>(qint64 leftId, qint64 rightId)
{
    return Entity::addToRelation&lt;<xsl:value-of select="$relationName"/>&gt;(leftId, rightId);
}

bool <xsl:value-of select="$className"/>::remove<xsl:value-of select="@table2"/>(const <xsl:value-of select="$rightSideClass"/> &amp;value) const
{
    return Entity::removeFromRelation&lt;<xsl:value-of select="$relationName"/>&gt;(id(), value.id());
}

bool <xsl:value-of select="$className"/>::remove<xsl:value-of select="@table2"/>(qint64 leftId, qint64 rightId)
{
    return Entity::removeFromRelation&lt;<xsl:value-of select="$relationName"/>&gt;(leftId, rightId);
}

bool <xsl:value-of select="$className"/>::clear<xsl:value-of select="@table2"/>s() const
{
    return Entity::clearRelation&lt;<xsl:value-of select="$relationName"/>&gt;(id());
}

bool <xsl:value-of select="$className"/>::clear<xsl:value-of select="@table2"/>s(qint64 id)
{
    return Entity::clearRelation&lt;<xsl:value-of select="$relationName"/>&gt;(id);
}

</xsl:for-each>

#ifndef QT_NO_DEBUG_STREAM
// debug stream operator
QDebug &amp;operator&lt;&lt;(QDebug &amp;d, const <xsl:value-of select="$className"/> &amp;entity)
{
    d &lt;&lt; "[<xsl:value-of select="$className"/>: "
    <xsl:for-each select="column">
        &lt;&lt; "<xsl:value-of select="@name"/> = " &lt;&lt;
        <xsl:choose>
        <xsl:when test="starts-with(@type, 'enum')">
        static_cast&lt;int&gt;(entity.<xsl:value-of select="@name"/>())
        </xsl:when>
        <xsl:otherwise>
        entity.<xsl:value-of select="@name"/>()
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="position() != last()">&lt;&lt; ", "</xsl:if>
    </xsl:for-each>
        &lt;&lt; "]";
    return d;
}
#endif

// inserting new data
bool <xsl:value-of select="$className"/>::insert(qint64* insertId)
{
    QSqlDatabase db = DataStore::self()->database();
    if (!db.isOpen()) {
        return false;
    }

    QueryBuilder qb(tableName(), QueryBuilder::Insert);
    <xsl:if test="@identificationColumn">
    qb.setIdentificationColumn(QLatin1String("<xsl:value-of select="@identificationColumn"/>"));
    </xsl:if>
    <xsl:for-each select="column[@name != 'id']">
      <xsl:variable name="refColumn"><xsl:value-of select="@refColumn"/></xsl:variable>
      <xsl:if test="$refColumn = 'id'">
    if (d-&gt;<xsl:value-of select="@name"/>_changed  &amp;&amp; d-&gt;<xsl:value-of select="@name"/> &gt; 0) {
        qb.setColumnValue( <xsl:value-of select="@name"/>Column(), this-&gt;<xsl:value-of select="@name"/>() );
    }
      </xsl:if>
      <xsl:if test="$refColumn != 'id'">
    if (d-&gt;<xsl:value-of select="@name"/>_changed) {
        <xsl:choose>
          <xsl:when test="starts-with(@type, 'enum')">
        qb.setColumnValue(<xsl:value-of select="@name"/>Column(), static_cast&lt;int&gt;(this-&gt;<xsl:value-of select="@name"/>()));
          </xsl:when>
          <xsl:otherwise>
        qb.setColumnValue(<xsl:value-of select="@name"/>Column(), this-&gt;<xsl:value-of select="@name"/>());
          </xsl:otherwise>
        </xsl:choose>
    }
      </xsl:if>
    </xsl:for-each>

    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) &lt;&lt; "Error during insertion into table" &lt;&lt; tableName()
                                     &lt;&lt; qb.query().lastError().text();
        return false;
    }

    setId(qb.insertId());
    if (insertId) {
        *insertId = id();
    }
    return true;
}

bool <xsl:value-of select="$className"/>::hasPendingChanges() const
{
      return false
      <xsl:for-each select="column[@name != 'id']">
        || d-&gt;<xsl:value-of select="@name"/>_changed
      </xsl:for-each>;
}

// update existing data
bool <xsl:value-of select="$className"/>::update()
{
    invalidateCache();
    QSqlDatabase db = DataStore::self()->database();
    if (!db.isOpen()) {
        return false;
    }

    QueryBuilder qb(tableName(), QueryBuilder::Update);

    <xsl:for-each select="column[@name != 'id']">
    <xsl:variable name="refColumn"><xsl:value-of select="@refColumn"/></xsl:variable>
    if (d-&gt;<xsl:value-of select="@name"/>_changed) {
        <xsl:if test="$refColumn = 'id'">
        if (d-&gt;<xsl:value-of select="@name"/> &lt;= 0) {
            qb.setColumnValue(<xsl:value-of select="@name"/>Column(), QVariant());
        } else {
        </xsl:if>
        <xsl:choose>
            <xsl:when test="starts-with(@type, 'enum')">
            qb.setColumnValue(<xsl:value-of select="@name"/>Column(), static_cast&lt;int&gt;(this-&gt;<xsl:value-of select="@name"/>()));
            </xsl:when>
            <xsl:otherwise>
            qb.setColumnValue(<xsl:value-of select="@name"/>Column(), this-&gt;<xsl:value-of select="@name"/>());
            </xsl:otherwise>
        </xsl:choose>
        <xsl:if test="$refColumn = 'id'">
        }
        </xsl:if>
    }
    </xsl:for-each>

    <xsl:if test="column[@name = 'id']">
    qb.addValueCondition(idColumn(), Query::Equals, id());
    </xsl:if>

    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) &lt;&lt; "Error during updating record with id" &lt;&lt; id()
                                     &lt;&lt; " in table" &lt;&lt; tableName() &lt;&lt; qb.query().lastError().text();
        return false;
    }
    return true;
}

// delete records
bool <xsl:value-of select="$className"/>::remove(const QString &amp;column, const QVariant &amp;value)
{
    invalidateCompleteCache();
    return Entity::remove&lt;<xsl:value-of select="$className"/>&gt;(column, value);
}

<xsl:if test="column[@name = 'id']">
bool <xsl:value-of select="$className"/>::remove()
{
    invalidateCache();
    return Entity::remove&lt;<xsl:value-of select="$className"/>&gt;(idColumn(), id());
}

bool <xsl:value-of select="$className"/>::remove(qint64 id)
{
    return remove(idColumn(), id);
}
</xsl:if>

// cache stuff
void <xsl:value-of select="$className"/>::invalidateCache() const
{
    if (Private::cacheEnabled) {
        QMutexLocker lock(&amp;Private::cacheMutex);
        <xsl:if test="column[@name = 'id']">
        Private::idCache.remove(id());
        </xsl:if>
        <xsl:if test="column[@name = 'name']">
          <xsl:choose>
            <xsl:when test="$className = 'PartType'">
            <!-- Special handling for PartType, which is identified as "NS:NAME" -->
        Private::nameCache.remove(ns() + QLatin1Char(':') + name());
            </xsl:when>
            <xsl:otherwise>
        Private::nameCache.remove(name());
            </xsl:otherwise>
          </xsl:choose>
        </xsl:if>
    }
}

void <xsl:value-of select="$className"/>::invalidateCompleteCache()
{
    if (Private::cacheEnabled) {
        QMutexLocker lock(&amp;Private::cacheMutex);
        <xsl:if test="column[@name = 'id']">
        Private::idCache.clear();
        </xsl:if>
        <xsl:if test="column[@name = 'name']">
        Private::nameCache.clear();
        </xsl:if>
    }
}

void <xsl:value-of select="$className"/>::enableCache(bool enable)
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
    static const QString table = QStringLiteral("<xsl:value-of select="$tableName"/>" );
    return table;
}

QString <xsl:value-of select="$className"/>::leftColumn()
{
    static const QString column = QStringLiteral("<xsl:value-of select="@table1"/>_<xsl:value-of select="@column1"/>");
    return column;
}

QString <xsl:value-of select="$className"/>::leftFullColumnName()
{
    static const QString column = QStringLiteral("<xsl:value-of select="$tableName"/>.<xsl:value-of select="@table1"/>_<xsl:value-of select="@column1"/>");
    return column;
}

QString <xsl:value-of select="$className"/>::rightColumn()
{
    static const QString column = QStringLiteral("<xsl:value-of select="@table2"/>_<xsl:value-of select="@column2"/>");
    return column;
}

QString <xsl:value-of select="$className"/>::rightFullColumnName()
{
    static const QString column = QStringLiteral("<xsl:value-of select="$tableName"/>.<xsl:value-of select="@table2"/>_<xsl:value-of select="@column2"/>");
    return column;
}
</xsl:template>

</xsl:stylesheet>
