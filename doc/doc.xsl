<xsl:transform version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <!--xmlns:xhtml="http://www.w3.org/1999/xhtml"-->

  <xsl:param name="filename" select="''"/>

  <xsl:output method="xml" encoding="ISO-8859-1" indent="no"
    doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
    doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd" />

  <xsl:template match="head">
    <head>
      <xsl:apply-templates />
      <link rel="stylesheet" href="article.css" type="text/css" />
    </head>
  </xsl:template>

  <xsl:template match="title">
    <title>
      <xsl:value-of select="$filename" />
    </title>
  </xsl:template>

  <xsl:template match="body">
    <body>
      <h1>
        <xsl:choose>
          <xsl:when test="//h1">
            <xsl:apply-templates select="h1" mode="heading"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$filename"/>
          </xsl:otherwise>
        </xsl:choose>
      </h1>

      <xsl:call-template name="menu" />

      <xsl:call-template name="toc" />

      <div class="main">
        <xsl:apply-templates />
      </div>

      <!--xsl:call-template name="bottom" /-->
    </body>
  </xsl:template>
    
  <xsl:template match="h1" mode="heading">
    <xsl:apply-templates />
  </xsl:template>
    
  <xsl:template match="h1">
  </xsl:template>

  <xsl:template name="menu">
    <xsl:apply-templates select="document('menu.xml')" />
  </xsl:template>

  <xsl:template name="toc">
    <div class="toc">
      <ol>
        <xsl:apply-templates select="//h2" mode="toc" />
      </ol>
    </div>
  </xsl:template>
    
  <xsl:template match="h2" mode="toc">
    <li>
      <a href="#{a/@name}">
        <xsl:value-of select="." />
      </a>
    </li>
  </xsl:template>
    
  <xsl:template 
    match="@*|*|text()">
    <xsl:copy>
      <xsl:apply-templates 
        select="@*|*|text()"/>
    </xsl:copy>
  </xsl:template>

</xsl:transform>
