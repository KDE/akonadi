<!--
    SPDX-FileCopyrightText: 2013 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
<xsl:output method="text" encoding="utf-8"/>

<xsl:include href="schema-header.xsl"/>
<xsl:include href="schema-source.xsl"/>

<!-- select whether to generate header or implementation code. -->
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
      idx.columns = QStringLiteral("<xsl:value-of select="@columns"/>").split(QLatin1Char( ',' ), Qt::SkipEmptyParts);
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

