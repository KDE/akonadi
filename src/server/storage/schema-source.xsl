<!--
    SPDX-FileCopyrightText: 2013 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
<xsl:output method="text" encoding="utf-8"/>

<xsl:template name="data-type">
  <xsl:choose>
  <xsl:when test="@type = 'enum'"><xsl:value-of select="@enumType"/></xsl:when>
  <xsl:otherwise><xsl:value-of select="@type"/></xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="schema-source">
#include "<xsl:value-of select="$fileName"/>.h"

using namespace Akonadi::Server;

QList&lt;TableDescription&gt; <xsl:value-of select="$className"/>::tables()
{
  QList&lt;TableDescription&gt; tabs;
  tabs.reserve(<xsl:value-of select="count(database/table)"/>);
  <xsl:for-each select="database/table">
  {
    TableDescription t;
    t.name = QStringLiteral("<xsl:value-of select="@name"/>Table");

    t.columns.reserve(<xsl:value-of select="count(column)"/>);
    <xsl:for-each select="column">
    {
      ColumnDescription c;
      c.name = QStringLiteral("<xsl:value-of select="@name"/>");
      c.type = QStringLiteral("<xsl:call-template name="data-type"/>");
      <xsl:if test="@size">
      c.size = <xsl:value-of select="@size"/>;
      </xsl:if>
      <xsl:if test="@allowNull">
      c.allowNull = <xsl:value-of select="@allowNull"/>;
      </xsl:if>
      <xsl:if test="@isAutoIncrement">
      c.isAutoIncrement = <xsl:value-of select="@isAutoIncrement"/>;
      </xsl:if>
      <xsl:if test="@isPrimaryKey">
      c.isPrimaryKey = <xsl:value-of select="@isPrimaryKey"/>;
      </xsl:if>
      <xsl:if test="@isUnique">
      c.isUnique = <xsl:value-of select="@isUnique"/>;
      </xsl:if>
      <xsl:if test="@type = 'enum'">
      c.isEnum = true;
      </xsl:if>
      <xsl:if test="@refTable">
      c.refTable = QStringLiteral("<xsl:value-of select="@refTable"/>");
      </xsl:if>
      <xsl:if test="@refColumn">
      c.refColumn = QStringLiteral("<xsl:value-of select="@refColumn"/>");
      </xsl:if>
      <xsl:if test="@default">
      c.defaultValue = QStringLiteral("<xsl:value-of select="@default"/>");
      </xsl:if>
      <xsl:if test="@onUpdate">
      c.onUpdate = ColumnDescription::<xsl:value-of select="@onUpdate"/>;
      </xsl:if>
      <xsl:if test="@onDelete">
      c.onDelete = ColumnDescription::<xsl:value-of select="@onDelete"/>;
      </xsl:if>
      <xsl:if test="@noUpdate">
      c.noUpdate = <xsl:value-of select="@noUpdate"/>;
      </xsl:if>

      <xsl:if test="@type = 'enum'">
      c.enumValueMap = {
      <xsl:for-each select="../enum">
        <xsl:for-each select="value">
        { QStringLiteral("<xsl:value-of select="../@name"/>::<xsl:value-of select="@name"/>"),
            <xsl:choose>
             <xsl:when test="@value"><xsl:value-of select="@value"/></xsl:when>
             <xsl:otherwise><xsl:value-of select="position() - 1"/></xsl:otherwise>
            </xsl:choose> }<xsl:if test="position() != last()">,</xsl:if>
        </xsl:for-each>
      </xsl:for-each>
      };
      </xsl:if>

      t.columns.push_back(c);
    }
    </xsl:for-each>

    <xsl:if test="count(index) > 0">
      <xsl:if test="count(index) > 0">
        <xsl:call-template name="indexes">
          <xsl:with-param name="var">t</xsl:with-param>
        </xsl:call-template>
      </xsl:if>
    </xsl:if>

    <xsl:if test="count(data) > 0">
      t.data.reserve(<xsl:value-of select="count(data)"/>);
      <xsl:for-each select="data">
      {
        const QStringList columns = QStringLiteral("<xsl:value-of select="@columns"/>").split( QLatin1Char( ',' ), Qt::SkipEmptyParts );
        const QStringList values = QStringLiteral("<xsl:value-of select="@values"/>").split( QLatin1Char( ',' ), Qt::SkipEmptyParts );
        Q_ASSERT( columns.count() == values.count() );

        DataDescription d;
        for ( int i = 0; i &lt; columns.size(); ++i ) {
          d.data.insert( columns.at( i ), values.at( i ) );
        }
        t.data.push_back(d);
      }
      </xsl:for-each>
    </xsl:if>

    tabs.push_back(t);
  }
  </xsl:for-each>
  return tabs;
}

QList&lt;RelationDescription&gt; <xsl:value-of select="$className"/>::relations()
{
  QList&lt;RelationDescription&gt; rels;
  rels.reserve(<xsl:value-of select="count(database/relation)"/>);
  <xsl:for-each select="database/relation">
  {
    RelationDescription r;
    r.firstTable = QStringLiteral("<xsl:value-of select="@table1"/>");
    r.firstColumn = QStringLiteral("<xsl:value-of select="@column1"/>");
    r.secondTable = QStringLiteral("<xsl:value-of select="@table2"/>");
    r.secondColumn = QStringLiteral("<xsl:value-of select="@column2"/>");
    <xsl:if test="count(index) > 0">
      <xsl:call-template name="indexes">
        <xsl:with-param name="var">r</xsl:with-param>
      </xsl:call-template>
    </xsl:if>
    rels.push_back(r);

  }
  </xsl:for-each>
  return rels;
}

</xsl:template>
</xsl:stylesheet>

