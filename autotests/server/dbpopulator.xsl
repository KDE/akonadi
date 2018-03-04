<!--
    Copyright (c) 2014 - Daniel Vrátil <dvratil@redhat.com>

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
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="exsl"
                version="1.0">

<xsl:output method="text" encoding="utf-8"/>

<xsl:variable name="schema" select="document('../../src/server/storage/akonadidb.xml')/database"/>
<xsl:variable name="data" select="." />

<xsl:template name="first-upper-case">
  <xsl:param name="name"/>
  <xsl:value-of select="concat(translate(substring($name, 1, 1), 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'), substring($name, 2))"/>
</xsl:template>

<xsl:template name="first-lower-case">
  <xsl:param name="name"/>
  <xsl:value-of select="concat(translate(substring($name, 1, 1), 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz'), substring($name, 2))"/>
</xsl:template>

<xsl:template name="entity-variable-name">
  <xsl:call-template name="first-lower-case">
    <xsl:with-param name="name" select="local-name()" />
  </xsl:call-template>
  <xsl:value-of select="generate-id()" />
</xsl:template>

<xsl:template name="name-to-setter">
  <xsl:text>set</xsl:text><xsl:call-template name="first-upper-case"><xsl:with-param name="name" select="@name"/></xsl:call-template>
</xsl:template>

<xsl:template name="translate-enum-value">
  <xsl:param name="value" />
  <xsl:param name="enumType" />
  <xsl:param name="table" />

  <xsl:value-of select="$table"/>::<xsl:value-of select="substring($value, string-length($enumType) + 3)" />
</xsl:template>

<xsl:template name="parse-entities-recursively">
  <xsl:param name="attributes" />
  <xsl:for-each select="*">
    <xsl:choose>
      <xsl:when test="local-name() = 'MimeType'">
        <xsl:if test="parent::Collection">
          <xsl:variable name="mt" select="@name" />
          <xsl:variable name="mimeTypeId" select="generate-id(exsl:node-set($attributes)/MimeTypes/*[@id = $mt])"/>
          <xsl:variable name="collectionId" select="generate-id(parent::node())" />
          <xsl:text>collection</xsl:text><xsl:value-of select="$collectionId"/>.addMimeType(mimeType<xsl:value-of select="$mimeTypeId" /><xsl:text>);

  </xsl:text>
        </xsl:if>
      </xsl:when>
      <xsl:when test="local-name() = 'Flag'">
        <xsl:if test="parent::PimItem">
          <xsl:variable name="flag" select="@name" />
          <xsl:variable name="flagId" select="generate-id(exsl:node-set($attributes)/Flags/*[@id = $flag])"/>
          <xsl:variable name="pimItemId" select="generate-id(parent::node())" />
          <xsl:text>pimItem</xsl:text><xsl:value-of select="$pimItemId"/>.addFlag(flag<xsl:value-of select="$flagId" /><xsl:text>);

  </xsl:text>
        </xsl:if>
      </xsl:when>
      <xsl:when test="local-name() = 'Tag'">
        <xsl:if test="parent::PimItem">
          <xsl:variable name="tag" select="@gid" />
          <xsl:variable name="tagId" select="generate-id(exsl:node-set($attributes)/Tags/*[@id = $tag])"/>
          <xsl:variable name="pimItemId" select="generate-id(parent::node())" />
          <xsl:text>pimItem</xsl:text><xsl:value-of select="$pimItemId"/>.addTag(tag<xsl:value-of select="$tagId" /><xsl:text>);

  </xsl:text>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="parse-entity">
          <xsl:with-param name="table" select="current()"/>
          <xsl:with-param name="attributes" select="$attributes"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<xsl:template name="parse-entity">
  <xsl:param name="table"/>
  <xsl:param name="attributes"/>
  <xsl:variable name="tableName" select="local-name()" />

  <!-- Declare the entity variable //-->
  <xsl:variable name="variable">
    <xsl:call-template name="entity-variable-name"/>
  </xsl:variable>
  <xsl:value-of select="concat(local-name(), ' ', $variable)"/><xsl:text>;
  </xsl:text>

  <!-- Call setters for all specified values //-->
  <xsl:for-each select="$schema/table[@name = $tableName]/column[@name != 'id']">
    <xsl:variable name="columnName" select="@name" />
    <xsl:variable name="columnValue">
      <xsl:choose>
        <!-- If value of the current column is explicitly set, then use it //-->
        <xsl:when test="$table/@*[local-name() = $columnName]">
          <xsl:value-of select="$table/@*[local-name() = $columnName]" />
        </xsl:when>
        <!-- If value is not specified, but the column has a foreign key, then resolve value from
             the referred entity //-->
        <xsl:when test="@refTable">
          <xsl:variable name="refTable" select="@refTable"/>
          <xsl:variable name="refElement" select="generate-id(($table/ancestor::*[local-name() = $refTable][1])[last()])"/>
          <xsl:choose>
            <!-- Special handling for PimItem.partTypeId //-->
            <xsl:when test="$refTable = 'PartType' and @refColumn = 'id'">
              <xsl:variable name="partType" select="$table/@partType" />
              <xsl:variable name="partTypeId" select="generate-id(exsl:node-set($attributes)/PartTypes/*[@id = $partType])" />
              <xsl:text>partType</xsl:text><xsl:value-of select="$partTypeId" /><xsl:text>.id()</xsl:text>
            </xsl:when>
            <xsl:when test="$refTable = 'MimeType' and @refColumn = 'id'">
              <xsl:variable name="mimeType" select="$table/@mimeType" />
              <xsl:variable name="mimeTypeId" select="generate-id(exsl:node-set($attributes)/MimeTypes/*[@id = $mimeType])" />
              <xsl:text>mimeType</xsl:text><xsl:value-of select="$mimeTypeId"/><xsl:text>.id()</xsl:text>
            </xsl:when>
            <!-- Get id of entity refered to via refTable and refColumn //-->
            <xsl:when test="$refElement">
              <xsl:call-template name="first-lower-case">
                <xsl:with-param name="name" select="@refTable"/>
              </xsl:call-template>
              <xsl:value-of select="$refElement"/><xsl:text>.id()</xsl:text>
            </xsl:when>
            <!-- Default value (fallback) //-->
            <xsl:otherwise>
              <xsl:text>NULL</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>
    <xsl:if test="$columnValue != ''">
      <xsl:value-of select="$variable"/>.<xsl:call-template name="name-to-setter" />
      <xsl:text>(</xsl:text>
      <!-- Handle various default types //-->
      <xsl:choose>
        <xsl:when test="@type = 'qint64' or @type = 'int' or @type = 'bool'">
          <xsl:choose>
            <xsl:when test="$columnValue = 'NULL'">
              <xsl:text>0</xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="$columnValue" />
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="@type = 'enum'">
          <xsl:choose>
            <xsl:when test="$columnValue = 'NULL'">
              <xsl:call-template name="translate-enum-value">
                <xsl:with-param name="value"><xsl:value-of select="$schema/table[@name = $tableName]/column[@name = $columnName]/@default"/></xsl:with-param>
                <xsl:with-param name="enumType"><xsl:value-of select="$schema/table[@name = $tableName]/column[@name = $columnName]/@enumType"/></xsl:with-param>
                <xsl:with-param name="table"><xsl:value-of select="$tableName"/></xsl:with-param>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="translate-enum-value">
                <xsl:with-param name="value"><xsl:value-of select="$columnValue"/></xsl:with-param>
                <xsl:with-param name="enumType"><xsl:value-of select="$schema/table[@name = $tableName]/column[@name = $columnName]/@enumType"/></xsl:with-param>
                <xsl:with-param name="table"><xsl:value-of select="$tableName"/></xsl:with-param>
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="@type = 'QString'">
          <xsl:choose>
            <xsl:when test="$columnValue = 'NULL'">
              <xsl:text>QString()</xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:text>QLatin1String("</xsl:text><xsl:value-of select="$columnValue" /><xsl:text>")</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="@type = 'QByteArray'">
          <xsl:text>"</xsl:text><xsl:value-of select="$columnValue" /><xsl:text>"</xsl:text>
        </xsl:when>
        <xsl:when test="@type = 'QDateTime'">
          <xsl:text>QDateTime::fromString(QLatin1String("</xsl:text><xsl:value-of select="$columnValue" /><xsl:text>"), Qt::ISODate)</xsl:text>
        </xsl:when>
      </xsl:choose>
      <xsl:text>);
  </xsl:text>
    </xsl:if>
  </xsl:for-each>

  <!-- Call .insert() //-->
  if (!<xsl:value-of select="$variable" />.insert()) {
    qWarning() &lt;&lt; "Failed to insert <xsl:value-of select="$variable" /> into database";
    qWarning() &lt;&lt; "DB Error:" &lt;&lt; FakeDataStore::self()->database().lastError().text();
    return false;
  }
  qDebug() &lt;&lt; "<xsl:value-of select="local-name()" /> '<xsl:choose>
    <xsl:when test="local-name() = 'PimItem'">
      <xsl:value-of select="$table/@remoteId" />
    </xsl:when>
    <xsl:when test="local-name() = 'Tag'">
      <xsl:value-of select="$table/@gid" />
    </xsl:when>
    <xsl:when test="local-name() = 'PartType'">
      <xsl:value-of select="$table/@ns" />:<xsl:value-of select="$table/@name" />
    </xsl:when>
    <xsl:when test="local-name() = 'Part'">
      <xsl:value-of select="$table/@partType" />
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$table/@name"/>
    </xsl:otherwise>
  </xsl:choose>' inserted with id" &lt;&lt; <xsl:value-of select="$variable" />.id();

  <!-- Recursively process child entities //-->
  <xsl:call-template name="parse-entities-recursively">
    <xsl:with-param name="attributes" select="$attributes"/>
  </xsl:call-template>
</xsl:template>



<!-- Finds all entities with name "attrType" (like <MimeType> or <Flag>) as well
     as all attributes with "attrName" ("mimeType = ...") and transforms them into
     a simple list of elements //-->
<xsl:template name="transform-all-attributes">
  <xsl:param name="attrType" />
  <xsl:param name="attrName" />
  <xsl:copy>
    <xsl:for-each select="/descendant::*[local-name() = $attrType]">
    <xsl:element name="{$attrType}">
        <xsl:choose>
          <!-- Special case of Tags, which don't have name, but GID //-->
          <xsl:when test="$attrType = 'Tag'">
            <xsl:attribute name="id"><xsl:value-of select="@gid" /></xsl:attribute>
            <xsl:attribute name="gid"><xsl:value-of select="@gid" /></xsl:attribute>
          </xsl:when>
          <xsl:otherwise>
            <xsl:attribute name="id"><xsl:value-of select="@name" /></xsl:attribute>
            <xsl:attribute name="name"><xsl:value-of select="@name" /></xsl:attribute>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:element>
    </xsl:for-each>
    <xsl:for-each select="/descendant::*">
      <xsl:variable name="attrValue" select="@*[local-name() = $attrName]" />
      <xsl:if test="$attrValue != ''">
        <xsl:element name="{$attrType}">
          <xsl:attribute name="id"><xsl:value-of select="$attrValue" /></xsl:attribute>
          <xsl:choose>
            <xsl:when test="$attrType = 'PartType' or $attrName = 'partType'">
              <xsl:attribute name="ns"><xsl:value-of select="substring-before(@partType, ':')" /></xsl:attribute>
              <xsl:attribute name="name"><xsl:value-of select="substring-after(@partType, ':')" /></xsl:attribute>
            </xsl:when>
            <xsl:otherwise>
              <xsl:attribute name="name"><xsl:value-of select="$attrValue" /></xsl:attribute>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:element>
      </xsl:if>
    </xsl:for-each>
  </xsl:copy>
</xsl:template>

<!-- Finds all entities of type 'attrType' or attributes with name 'attrName',
     sorts them, removes duplicates and returns a new simple list of elements //-->
<xsl:template name="parse-attributes">
  <xsl:param name="attrType" />
  <xsl:param name="attrName" />
  <xsl:variable name="tmp">
    <xsl:call-template name="transform-all-attributes">
      <xsl:with-param name="attrType" select="$attrType"/>
      <xsl:with-param name="attrName" select="$attrName"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="sorted">
    <xsl:copy>
      <xsl:for-each select="exsl:node-set($tmp)/*">
        <xsl:sort select="@id" data-type="text"/>
        <xsl:copy-of select="." />
      </xsl:for-each>
    </xsl:copy>
  </xsl:variable>

  <xsl:copy>
    <xsl:for-each select="exsl:node-set($sorted)/*">
      <xsl:sort select="@id" data-type="text"/>
      <xsl:if test="position() = 1 or @id != preceding-sibling::*[1]/@id">
        <xsl:copy-of select="."/>
      </xsl:if>
    </xsl:for-each>
  </xsl:copy>
</xsl:template>


<xsl:template match="/">
<!-- Header generation //-->
<xsl:if test="$code='header'">
/*
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef AKONADI_SERVER_DBPOPULATOR_H
#define AKONADI_SERVER_DBPOPULATOR_H

namespace Akonadi {
namespace Server {

class DbPopulator
{
public:
    DbPopulator();
    ~DbPopulator();

    bool run();

};

}
}
#endif
</xsl:if>

<!-- Source generation //-->
<xsl:if test="$code='source'">
/*
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#include "dbpopulator.h"
#include "entities.h"
#include "fakedatastore.h"

#include &lt;QtSql/QSqlDatabase&gt;
#include &lt;QtSql/QSqlQuery&gt;
#include &lt;QtSql/QSqlError&gt;

#include &lt;QtCore/QString&gt;
#include &lt;QtCore/QVariant&gt;

using namespace Akonadi::Server;

DbPopulator::DbPopulator()
{
}

DbPopulator::~DbPopulator()
{
}



bool DbPopulator::run()
{

  <!-- Extract, declare and insert into database all mimetypes, flags, parttypes and tags //-->
  <xsl:variable name="attributes">
    <xsl:element name="MimeTypes">
      <xsl:call-template name="parse-attributes">
        <xsl:with-param name="attrType">MimeType</xsl:with-param>
        <xsl:with-param name="attrName">mimeType</xsl:with-param>
      </xsl:call-template>
    </xsl:element>
    <xsl:element name="Flags">
      <xsl:call-template name="parse-attributes">
        <xsl:with-param name="attrType">Flag</xsl:with-param>
        <xsl:with-param name="attrName">flag</xsl:with-param>
      </xsl:call-template>
    </xsl:element>
    <xsl:element name="PartTypes">
      <xsl:call-template name="parse-attributes">
        <xsl:with-param name="attrType">PartType</xsl:with-param>
        <xsl:with-param name="attrName">partType</xsl:with-param>
      </xsl:call-template>
    </xsl:element>
    <xsl:element name="Tags">
      <xsl:call-template name="parse-attributes">
        <xsl:with-param name="attrType">Tag</xsl:with-param>
        <xsl:with-param name="attrName">tag</xsl:with-param>
      </xsl:call-template>
    </xsl:element>
  </xsl:variable>


  <xsl:for-each select="exsl:node-set($attributes)/*/*">
    <xsl:call-template name="parse-entity">
      <xsl:with-param name="table" select="current()"/>
    </xsl:call-template>
  </xsl:for-each>


  <!-- Extract, declare and insert into database all remaining entities //-->
  <xsl:for-each select="/data">
    <xsl:call-template name="parse-entities-recursively">
      <xsl:with-param name="attributes" select="$attributes" />
    </xsl:call-template>
  </xsl:for-each>

  qDebug() &lt;&lt; "Database successfully populated";
  return true;
}
</xsl:if>


</xsl:template>
</xsl:stylesheet>

