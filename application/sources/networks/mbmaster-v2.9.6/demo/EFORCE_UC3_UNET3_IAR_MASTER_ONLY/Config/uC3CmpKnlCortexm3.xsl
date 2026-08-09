<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template match="Kernel">
    <p><hr/></p>
    <h3>[Kernel Configuration]</h3>

    <h4>Kernel General</h4>
    <xsl:apply-templates select="General" />

    <h4>Task</h4>
    <table>
    <tr>
    <th>ID Symbol</th>
    <th>Function Name</th>
    <th>Initial Priority</th>
    <th>Extended Information</th>
    <th>Stack Size</th>
    <th>Attributes</th>
    <th>Shared Stack</th>
    </tr>
    <xsl:apply-templates select="Task" />
    </table>

    <h4>Semephore</h4>
    <table>
    <tr>
    <th>ID Symbol</th>
    <th>Initial resource count</th>
    <th>Maximum resource count</th>
    <th>Attributes</th>
    </tr>
    <xsl:apply-templates select="Semaphore" />
    </table>

    <h4>EventFlag</h4>
    <table>
    <tr>
    <th>ID Symbol</th>
    <th>Initial bit pattern(hex)</th>
    <th>Attributes</th>
    </tr>
    <xsl:apply-templates select="EventFlag" />
    </table>

    <h4>DataQueue</h4>
    <table>
    <tr>
    <th>ID Symbol</th>
    <th>Queue Depth</th>
    <th>Attributes</th>
    </tr>
    <xsl:apply-templates select="DataQueue" />
    </table>

    <h4>Mailbox</h4>
    <table>
    <tr>
    <th>ID Symbol</th>
    <th>Attributes</th>
    </tr>
    <xsl:apply-templates select="Mailbox" />
    </table>

    <h4>Fixed-Sized Memory Pool</h4>
    <table>
    <tr>
    <th>ID Symbol</th>
    <th>Memory Blocks</th>
    <th>Memory Block Size</th>
    <th>Attributes</th>
    </tr>
    <xsl:apply-templates select="FixedMemPool" />
    </table>

    <h4>Cyclic Handler</h4>
    <table>
    <tr>
    <th>ID Symbol</th>
    <th>Function Name</th>
    <th>Extended Information</th>
    <th>Activation Cycle</th>
    <th>Activation Phase</th>
    <th>Attributes</th>
    </tr>
    <xsl:apply-templates select="CyclicHandler" />
    </table>

    <h4>Interrupt Service Routine</h4>
    <table>
    <tr>
    <th>Interrupt Number</th>
    <th>Function Name</th>
    <th>Extended Information</th>
    </tr>
    <xsl:apply-templates select="Isr" />
    </table>

    <h4>Non-Kernel ISR</h4>
    <table>
    <tr>
    <th>Interrupt Number</th>
    <th>Function Name</th>
    </tr>
    <xsl:apply-templates select="NonKnlIsr" />
    </table>

    <h4>Shared Stack</h4>
    <table>
    <tr>
    <th>ID Symbol</th>
    <th>Stack Size</th>
    </tr>
    <xsl:apply-templates select="SharedStack" />
    </table>
  </xsl:template>

  <xsl:template match="Kernel/General">
    <table>
      <tr>
        <th>Kernel Mask Level</th>
        <th>Maximum task priority</th>
        <th>Tick Time</th>
        <th>User Initial Function</th>
        <th>User Idle Function</th>
        <th>User Header File</th>
        <th>Time Event Handler (CSTACK)</th>
        <th>System Handler (HSTACK)</th>
      </tr>
      <tr>
        <td><xsl:value-of select="CFG_KNLMSKLVL"/></td>
        <td><xsl:value-of select="CFG_MAXTSKPRI"/></td>
        <td><xsl:value-of select="CFG_TICK"/></td>
        <td><xsl:value-of select="CFG_INITFUNC"/></td>
        <td><xsl:value-of select="CFG_IDLEFUNC"/></td>
        <td><xsl:value-of select="CFG_ADDHEADER"/></td>
        <td><xsl:value-of select="CFG_CSTKSZ"/></td>
        <td><xsl:value-of select="CFG_HSTKSZ"/></td>
      </tr>
    </table>
  </xsl:template>

  <xsl:template match="Task">
    <tr>
      <td><xsl:value-of select="@CFG_TSKID"/></td>
      <td><xsl:value-of select="CFG_TASK"/></td>
      <td><xsl:value-of select="CFG_ITSKPRI"/></td>
      <td><xsl:value-of select="CFG_TSKEXINF"/></td>
      <td><xsl:value-of select="CFG_STKSZ"/></td>
      <td><xsl:value-of select="CFG_TSKATR"/></td>
      <td><xsl:value-of select="CFG_SSTKID"/></td>
    </tr>
  </xsl:template>
  
  <xsl:template match="Semaphore">
    <tr>
      <td><xsl:value-of select="@CFG_SEMID"/></td>
      <td><xsl:value-of select="CFG_ISEMCNT"/></td>
      <td><xsl:value-of select="CFG_MAXSEM"/></td>
      <td><xsl:value-of select="CFG_SEMATR"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="EventFlag">
    <tr>
      <td><xsl:value-of select="@CFG_FLGID"/></td>
      <td><xsl:value-of select="CFG_IFLGPTN"/></td>
      <td><xsl:value-of select="CFG_FLGATR"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="DataQueue">
    <tr>
      <td><xsl:value-of select="@CFG_DTQID"/></td>
      <td><xsl:value-of select="CFG_DTQCNT"/></td>
      <td><xsl:value-of select="CFG_DTQATR"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="Mailbox">
    <tr>
      <td><xsl:value-of select="@CFG_MBXID"/></td>
      <td><xsl:value-of select="CFG_MBXATR"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="FixedMemPool">
    <tr>
      <td><xsl:value-of select="@CFG_MPFID"/></td>
      <td><xsl:value-of select="CFG_BLKCNT"/></td>
      <xsl:choose>
        <xsl:when test="CFG_MPFDIRECT=1">
          <td><xsl:value-of select="CFG_BLKSZ"/></td>
        </xsl:when>
        <xsl:when test="CFG_MPFDIRECT='true'">
          <td><xsl:value-of select="CFG_BLKSZ"/></td>
        </xsl:when>
        <xsl:when test="CFG_MPFDIRECT=0">
          <td><xsl:value-of select="CFG_MPFEXP"/></td>
        </xsl:when>
        <xsl:when test="CFG_MPFDIRECT='false'">
          <td><xsl:value-of select="CFG_MPFEXP"/></td>
        </xsl:when>
      </xsl:choose>
      <td><xsl:value-of select="CFG_MPFATR"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="CyclicHandler">
    <tr>
      <td><xsl:value-of select="@CFG_CYCID"/></td>
      <td><xsl:value-of select="CFG_CYCHDR"/></td>
      <td><xsl:value-of select="CFG_CYCEXINF"/></td>
      <td><xsl:value-of select="CFG_CYCTIM"/></td>
      <td><xsl:value-of select="CFG_CYCPHS"/></td>
      <td><xsl:value-of select="CFG_CYCATR"/></td>
    </tr>
  </xsl:template>
  
  <xsl:template match="Isr">
    <tr>
      <td><xsl:value-of select="CFG_ISRINTNO"/></td>
      <td><xsl:value-of select="CFG_ISR"/></td>
      <td><xsl:value-of select="CFG_ISREXINF"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="NonKnlIsr">
    <tr>
      <td><xsl:value-of select="CFG_NKISRINTNO"/></td>
      <td><xsl:value-of select="CFG_NKISR"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="SharedStack">
    <tr>
      <td><xsl:value-of select="@CFG_SSTKID"/></td>
      <td><xsl:value-of select="CFG_SSTKSZ"/></td>
    </tr>
  </xsl:template>


</xsl:stylesheet>
