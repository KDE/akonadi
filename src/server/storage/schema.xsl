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

<xsl:include href="schema-header.xsl"/>
<xsl:include href="schema-source.xsl"/>

<!-- select wether to generate header or implementation code. -->
<xsl:param name="code">header</xsl:param>
<!-- name of the generated schema class -->
<xsl:param name="className">MySchema</xsl:param>
<!-- name of the generated file -->
<xsl:param name="fileName">schema</xsl:param>

<xsl:template name="indexes">
  <xsl:param name="var"/>
  <xsl:value-of select="$var"></xsl:value-of>.indexes.reserve(<xsl:value-of select="count(index)"/>);
  <xsl:for-each select="index">
    {
      IndexDescription idx;
      idx.name = QStringLiteral("<xsl:value-of select="@name"/>");
#if QT_VERSION &lt; QT_VERSION_CHECK(5, 15, 0)
      idx.columns = QStringLiteral("<xsl:value-of select="@columns"/>").split(QLatin1Char( ',' ), QString::SkipEmptyParts);
#else
      idx.columns = QStringLiteral("<xsl:value-of select="@columns"/>").split(QLatin1Char( ',' ), Qt::SkipEmptyParts);
#endif
      <xsl:if test="@unique">
      idx.isUnique = <xsl:value-of select="@unique"/>;
      </xsl:if>
      <xsl:if test="@sort">
      idx.sort = QStringLiteral("<xsl:value-of select="@sort"/>");
      </xsl:if>

      <xsl:value-of select="$var"></xsl:value-of>.indexes.push_back(idx);
    }
  </xsl:for-each>
</xsl:template>



<xsl:template match="/">
/*
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

<!-- header generation -->
<xsl:if test="$code='header'">
<xsl:call-template name="schema-header"/>
</xsl:if>

<!-- cpp generation -->
<xsl:if test="$code='source'">
<xsl:call-template name="schema-source"/>
</xsl:if>

</xsl:template>
</xsl:stylesheet>

