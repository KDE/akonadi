<!--
    Copyright (c) 2013 Volker Krause <vkrause@kde.org>

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

<xsl:template name="schema-source">
#include "<xsl:value-of select="$fileName"/>.h"

using namespace Akonadi::Server;

QVector&lt;TableDescription&gt; <xsl:value-of select="$className"/>::tables()
{
  QVector&lt;TableDescription&gt; tabs;
  tabs.reserve(<xsl:value-of select="count(database/table)"/>);
  <xsl:for-each select="database/table">
  {
    TableDescription t;
    t.name = QLatin1String("<xsl:value-of select="@name"/>Table");

    t.columns.reserve(<xsl:value-of select="count(column)"/>);
    <xsl:for-each select="column">
    {
      ColumnDescription c;
      c.name = QLatin1String("<xsl:value-of select="@name"/>");
      c.type = QLatin1String("<xsl:value-of select="@type"/>");
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
      <xsl:if test="@refTable">
      c.refTable = QLatin1String("<xsl:value-of select="@refTable"/>");
      </xsl:if>
      <xsl:if test="@refColumn">
      c.refColumn = QLatin1String("<xsl:value-of select="@refColumn"/>");
      </xsl:if>
      <xsl:if test="@default">
      c.defaultValue = QLatin1String("<xsl:value-of select="@default"/>");
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
        const QStringList columns = QString::fromLatin1("<xsl:value-of select="@columns"/>").split( QLatin1Char( ',' ), QString::SkipEmptyParts );
        const QStringList values = QString::fromLatin1("<xsl:value-of select="@values"/>").split( QLatin1Char( ',' ), QString::SkipEmptyParts );

        Q_ASSERT( columns.count() == values.count() );

        DataDescription d;
        for ( int i = 0; i &lt; columns.size(); ++i )
          d.data.insert( columns.at( i ), values.at( i ) );

        t.data.push_back(d);
      }
      </xsl:for-each>
    </xsl:if>

    tabs.push_back(t);
  }
  </xsl:for-each>
  return tabs;
}

QVector&lt;RelationDescription&gt; <xsl:value-of select="$className"/>::relations()
{
  QVector&lt;RelationDescription&gt; rels;
  rels.reserve(<xsl:value-of select="count(database/relation)"/>);
  <xsl:for-each select="database/relation">
  {
    RelationDescription r;
    r.firstTable = QLatin1String("<xsl:value-of select="@table1"/>");
    r.firstColumn = QLatin1String("<xsl:value-of select="@column1"/>");
    r.secondTable = QLatin1String("<xsl:value-of select="@table2"/>");
    r.secondColumn = QLatin1String("<xsl:value-of select="@column2"/>");

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

