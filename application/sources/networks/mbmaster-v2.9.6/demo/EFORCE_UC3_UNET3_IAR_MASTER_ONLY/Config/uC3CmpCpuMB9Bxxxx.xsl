<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- CPUエントリ --> 
  <xsl:template match="CPU">
    <p><hr/></p>
    <h3>[CPU Configuration]</h3>
    <h4>CPU</h4>
    <table>
    <tr>
    <th>Vendor</th>
    <th>Model</th>
    <th>Target</th>
    </tr>
    <tr>
    <td><xsl:value-of select="chip/@vendor"/></td>
    <td><xsl:value-of select="chip/@name"/></td>
    <td><xsl:value-of select="chip/@target"/></td>
    </tr>
    </table>
    <h4>CPU Core</h4>
    <xsl:apply-templates select="Core" />
    <h4>Clock</h4>
    <xsl:apply-templates select="Clock" />
    <xsl:apply-templates select="UART" />
    <xsl:apply-templates select="Ethernet" />
    <xsl:apply-templates select="DSPLOG" />
  </xsl:template>

  <!-- Core -->
  <xsl:template match="Core">
    <table>
      <tr>
        <th>pre-emption bit</th>
        <th>SysTick Driver</th>
        <th>SysTick Interrupt Level</th>
        <th>Low Power Mode</th>
      </tr>
      <tr>
        <td><xsl:value-of select="CFG_PREEMPTION"/></td>
        <td>
          <xsl:choose>
            <xsl:when test="CFG_SYSTICK_USE=1">Enable</xsl:when>
            <xsl:when test="CFG_SYSTICK_USE='true'">Enable</xsl:when>
            <xsl:when test="CFG_SYSTICK_USE=0">Disable</xsl:when>
            <xsl:when test="CFG_SYSTICK_USE='false'">Disable</xsl:when>
          </xsl:choose>
        </td>
        <td><xsl:value-of select="CFG_SYSTICK_INTLVL"/></td>
        <td>
          <xsl:choose>
            <xsl:when test="CFG_SLEEP_MODE=1">Enable</xsl:when>
            <xsl:when test="CFG_SLEEP_MODE='true'">Enable</xsl:when>
            <xsl:when test="CFG_SLEEP_MODE=0">Disable</xsl:when>
            <xsl:when test="CFG_SLEEP_MODE='false'">Disable</xsl:when>
          </xsl:choose>
        </td>
      </tr>
    </table>
    <h4>Exception Handler</h4>
    <table>
      <tr>
        <th>Exception Number</th>
        <th>Exception Type</th>
        <th>Function Name</th>
      </tr>
    <xsl:apply-templates select="Exception" />
    </table>
  </xsl:template>

  <!-- 例外ハンドラ -->
  <xsl:template match="Exception">
    <tr>
      <td><xsl:value-of select="@no"/></td>
      <td><xsl:value-of select="@type"/></td>
      <td><xsl:value-of select="@hdr"/></td>
    </tr>
  </xsl:template>

  <!-- クロック -->
  <xsl:template match="Clock">
    <table>
    <tr>
    <th>Main oscillator</th>
    <th>PLL K</th>
    <th>PLL M</th>
    <th>PLL N</th>
    <th>Master clock</th>
    <th>Base clock division ratio</th>
    <th>APB0 clock division ratio</th>
    <th>APB1 clock division ratio</th>
    <th>APB2 clock division ratio</th>
    </tr>
    <tr>
    <td><xsl:value-of select="CFG_MB9BF_CLKMO"/>Hz</td>
    <td><xsl:value-of select="CFG_MB9BF_PLLK"/></td>
    <td><xsl:value-of select="CFG_MB9BF_PLLM"/></td>
    <td><xsl:value-of select="CFG_MB9BF_PLLN"/></td>
    <td><xsl:value-of select="CFG_MB9BF_RSC"/></td>
    <td><xsl:value-of select="CFG_MB9BF_BSR"/></td>
    <td><xsl:value-of select="CFG_MB9BF_PSR0"/></td>
    <td><xsl:value-of select="CFG_MB9BF_PSR1"/></td>
    <td><xsl:value-of select="CFG_MB9BF_PSR2"/></td>
    </tr>
    </table>
  </xsl:template>

  <!-- UART -->
  <xsl:template match="UART">
    <h4><xsl:value-of select="@name"/></h4>
    <table>
    <tr>
    <th>Use</th>
    <th>Device ID</th>
    <th>Interrupt level</th>
    <th>Tx buffer size</th>
    <th>Rx buffer size</th>
    <th>XOFF transmit level</th>
    <th>XON transmit level</th>
    <th>Enable Tx FIFO</th>
    <th>Enable Rx FIOF</th>
    <th>Rx Trigger level</th>
    <th>Relocate</th>
    <th>CTS/RTS</th>     
    </tr>
    <tr>
    <td>
    <xsl:choose>
      <xsl:when test="@enable=1">Yes</xsl:when>
      <xsl:when test="@enable='true'">Yes</xsl:when>
      <xsl:when test="@enable=0">No</xsl:when>
      <xsl:when test="@enable='false'">No</xsl:when>
    </xsl:choose>
    </td>
    <td><xsl:value-of select="CFG_MB9BF_UARTID"/></td>
    <td><xsl:value-of select="CFG_MB9BF_UART_INTLVL"/></td>
    <td><xsl:value-of select="CFG_MB9BF_UART_TXBUFSZ"/></td>
    <td><xsl:value-of select="CFG_MB9BF_UART_RXBUFSZ"/></td>
    <td><xsl:value-of select="CFG_MB9BF_UART_XOFF"/></td>
    <td><xsl:value-of select="CFG_MB9BF_UART_XON"/></td>
    <td>
    <xsl:choose>
      <xsl:when test="CFG_MB9BF_UART_FIFO_TX=1">Enable</xsl:when>
      <xsl:when test="CFG_MB9BF_UART_FIFO_TX='true'">Enable</xsl:when>
      <xsl:when test="CFG_MB9BF_UART_FIFO_TX=0">Disable</xsl:when>
      <xsl:when test="CFG_MB9BF_UART_FIFO_TX='false'">Disable</xsl:when>
    </xsl:choose>
    </td>
    <td>
    <xsl:choose>
      <xsl:when test="CFG_MB9BF_UART_FIFO_RX=1">Enable</xsl:when>
      <xsl:when test="CFG_MB9BF_UART_FIFO_RX='true'">Enable</xsl:when>
      <xsl:when test="CFG_MB9BF_UART_FIFO_RX=0">Disable</xsl:when>
      <xsl:when test="CFG_MB9BF_UART_FIFO_RX='false'">Disable</xsl:when>
    </xsl:choose>
    </td>
    <td><xsl:value-of select="CFG_MB9BF_UART_FIFO_RTRG"/></td>
    <td><xsl:value-of select="CFG_MB9BF_UART_RELOCATE"/></td>
    <td>
    <xsl:choose>
      <xsl:when test="CFG_MB9BF_UART_HARDFLOW=1">Enable</xsl:when>
      <xsl:when test="CFG_MB9BF_UART_HARDFLOW='true'">Enable</xsl:when>
      <xsl:when test="CFG_MB9BF_UART_HARDFLOW=0">Disable</xsl:when>
      <xsl:when test="CFG_MB9BF_UART_HARDFLOW='false'">Disable</xsl:when>
    </xsl:choose>
    </td>
    </tr>
    </table>
  </xsl:template>

  <!-- Ethernet -->
  <xsl:template match="Ethernet">
    <h4><xsl:value-of select="@name"/></h4>
    <table>
    <tr>
    <th>Use</th>
    <th>Interrupt Level</th>
    <th>PHY ID</th>
    <th>MII Mode</th>
    <th>PHY Mode</th>
    <th>FILTER Mode</th>
    <th>Checksum offload</th>
    <th>Tx Buffer Count</th>
    <th>Rx Buffer Count</th>
    </tr>
    <tr>
    <td>
    <xsl:choose>
      <xsl:when test="@enable=1">Yes</xsl:when>
      <xsl:when test="@enable='true'">Yes</xsl:when>
      <xsl:when test="@enable=0">No</xsl:when>
      <xsl:when test="@enable='false'">No</xsl:when>
    </xsl:choose>
    </td>
    <td><xsl:value-of select="CFG_MB9BF_ETHER_INTLVL"/></td>
    <td><xsl:value-of select="CFG_MB9BF_ETHER_PHYID"/></td>
    <td>
    <xsl:if test="CFG_MB9BF_ETHER_MII=0">MII</xsl:if>
    <xsl:if test="CFG_MB9BF_ETHER_MII=1">RMII</xsl:if>
    </td>
    <td>
    <xsl:if test="CFG_MB9BF_ETHER_PHYMODE=0">Auto</xsl:if>
    <xsl:if test="CFG_MB9BF_ETHER_PHYMODE=1">10M HD</xsl:if>
    <xsl:if test="CFG_MB9BF_ETHER_PHYMODE=2">10M FD/HD</xsl:if>
    <xsl:if test="CFG_MB9BF_ETHER_PHYMODE=3">100M HD</xsl:if>
    <xsl:if test="CFG_MB9BF_ETHER_PHYMODE=4">100M FD/HD</xsl:if>
    </td>
    <td>
    <xsl:if test="CFG_MB9BF_ETHER_FILTERMODE=0">Perfect filter</xsl:if>
    <xsl:if test="CFG_MB9BF_ETHER_FILTERMODE=1">Promiscuous</xsl:if>
    <xsl:if test="CFG_MB9BF_ETHER_FILTERMODE=2">Multicast</xsl:if>
    </td>
    <td>
    <xsl:if test="CFG_MB9BF_ETHER_CHECKSUM=0">Disable Tx/Rx</xsl:if>
    <xsl:if test="CFG_MB9BF_ETHER_CHECKSUM=1">Enable Tx</xsl:if>
    <xsl:if test="CFG_MB9BF_ETHER_CHECKSUM=2">Enable Rx</xsl:if>
    <xsl:if test="CFG_MB9BF_ETHER_CHECKSUM=3">Enable Tx/Rx</xsl:if>
    </td>
    <td><xsl:value-of select="CFG_MB9BF_ETHER_TXBUFFER"/></td>
    <td><xsl:value-of select="CFG_MB9BF_ETHER_RXBUFFER"/></td>
    </tr>
    </table>
  </xsl:template>
  
  <!-- Dispatch Trace -->
  <xsl:template match="DSPLOG">
    <h4>Dispatch Trace</h4>
    <table>
    <tr>
    <th>Select Timer</th>
    <th>Clock setting</th>
    <th>Clock's Divider</th>
    <th>Trace Mode</th>
    <th>Buffer count</th>
    </tr>
    <tr>
    <td><xsl:choose>
      <xsl:when test="(CFG_MB9_DSPLOG_TMR &gt;= 0) and (CFG_MB9_DSPLOG_TMR &lt;= 15)">
      Base Timer ch.<xsl:value-of select="CFG_MB9_DSPLOG_TMR"/></xsl:when>
      <xsl:otherwise>unknown:<xsl:value-of select="CFG_MB9_DSPLOG_TMR"/></xsl:otherwise>
    </xsl:choose></td>
    <td><xsl:choose>
      <xsl:when test="CFG_MB9_DSPLOG_CLKSET=0">Manual</xsl:when>
      <xsl:when test="CFG_MB9_DSPLOG_CLKSET=1">Auto</xsl:when>
      <xsl:otherwise>unknown:<xsl:value-of select="CFG_MB9_DSPLOG_CLKSET"/></xsl:otherwise>
    </xsl:choose></td>
    <td><xsl:choose>
      <xsl:when test="CFG_MB9_DSPLOG_CLK_PS=0">1</xsl:when>
      <xsl:when test="CFG_MB9_DSPLOG_CLK_PS=1">4</xsl:when>
      <xsl:when test="CFG_MB9_DSPLOG_CLK_PS=2">16</xsl:when>
      <xsl:when test="CFG_MB9_DSPLOG_CLK_PS=3">128</xsl:when>
      <xsl:when test="CFG_MB9_DSPLOG_CLK_PS=4">256</xsl:when>
      <xsl:when test="CFG_MB9_DSPLOG_CLK_PS=8">512</xsl:when>
      <xsl:when test="CFG_MB9_DSPLOG_CLK_PS=9">1024</xsl:when>
      <xsl:when test="CFG_MB9_DSPLOG_CLK_PS=10">128</xsl:when>
      <xsl:otherwise>unknown:<xsl:value-of select="CFG_MB9_DSPLOG_CLK_PS"/></xsl:otherwise>
    </xsl:choose></td>
    <td><xsl:value-of select="CFG_MB9_DSPLOG_MODE"/></td>
    <td><xsl:value-of select="CFG_MB9_DSPLOG_CNT"/></td>
    </tr>
    </table>
  </xsl:template>
  
</xsl:stylesheet>
