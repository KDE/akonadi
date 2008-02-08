<!--
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

<!-- TODO
 - complete type mapping
 - support type annotations
-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
                xmlns:kcfg="http://www.kde.org/standards/kcfg/1.0"
                xmlns="http://www.kde.org/standards/kcfg/1.0">

<xsl:param name="code">interfaceName</xsl:param>

<xsl:template match="/">
<node>
<interface>
  <xsl:attribute name="name"><xsl:value-of select="$interfaceName"/></xsl:attribute>
  <xsl:for-each select="kcfg:kcfg/kcfg:group/kcfg:entry">
    <method>
      <xsl:attribute name="name">
        <xsl:value-of select="concat(translate(substring(@name,1,1),'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz'), substring(@name,2))"/>
      </xsl:attribute>
      <arg direction="out">
      <xsl:attribute name="type"><xsl:call-template name="convertType"/></xsl:attribute>
      </arg>
    </method>
    <method>
      <xsl:attribute name="name">
        <xsl:value-of select="concat( 'set', concat(translate(substring(@name,1,1),'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'), substring(@name,2)) )"/>
      </xsl:attribute>
      <arg direction="in" identifier="value">
      <xsl:attribute name="type"><xsl:call-template name="convertType"/></xsl:attribute>
      </arg>
    </method>
  </xsl:for-each>
</interface>
</node>
</xsl:template>

<xsl:template name="convertType">
<xsl:choose>
  <xsl:when test="@type = 'String'">s</xsl:when>
  <xsl:when test="@type = 'StringList'">as</xsl:when>
  <xsl:when test="@type = 'Font'">?</xsl:when>
  <xsl:when test="@type = 'Rect'">?</xsl:when>
  <xsl:when test="@type = 'Size'">?</xsl:when>
  <xsl:when test="@type = 'Color'">?</xsl:when>
  <xsl:when test="@type = 'Point'">?</xsl:when>
  <xsl:when test="@type = 'Int'">i</xsl:when>
  <xsl:when test="@type = 'UInt'">u</xsl:when>
  <xsl:when test="@type = 'Bool'">b</xsl:when>
  <xsl:when test="@type = 'Double'">d</xsl:when>
  <xsl:when test="@type = 'DateTime'">?</xsl:when>
  <xsl:when test="@type = 'LongLong'">x</xsl:when>
  <xsl:when test="@type = 'ULongLong'">t</xsl:when>
  <xsl:when test="@type = 'IntList'">ai</xsl:when>
  <xsl:when test="@type = 'Enum'">i</xsl:when>
  <xsl:when test="@type = 'Path'">s</xsl:when>
  <xsl:when test="@type = 'PathList'">as</xsl:when>
  <xsl:when test="@type = 'Password'">s</xsl:when>
  <xsl:when test="@type = 'Url'">s</xsl:when>
  <xsl:when test="@type = 'UrlList'">as</xsl:when>
  <xsl:otherwise>v</xsl:otherwise>
</xsl:choose>
</xsl:template>

</xsl:stylesheet>