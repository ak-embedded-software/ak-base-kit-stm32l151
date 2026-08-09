<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template match="Middle-uNet3">
    <p><hr/></p>
    <h3>[uNet3 Configuration]</h3>
    <h4>uNet3 General</h4>
    <xsl:apply-templates select="General" />

    <h4>Interface</h4>
    <table>
    <tr>
    <th>ID Symbol</th>
    <th>Device Type</th>
    <th>MTU</th>
    <th>MAC</th>
    <th>IPv4-DHCP</th>
    <th>IPv4-Address</th>
    <th>IPv4-Subnet Mask</th>
    <th>IPv4-Default gateway</th>
    <th>IPv4-Check the duplicate IP address.</th>
    <th>IPv6-Use Static Address</th>
    <th>IPv6-Address</th>
    <th>IPv6-Prefix length</th>
    <th>IPv6-Router Address</th>
    </tr>
    <xsl:apply-templates select="NetIf" />
    </table>


    <h4>Interface (PPP)</h4>
    <table>
    <tr>
    <th>ID Symbol</th>
    <th>Device Type</th>
    <th>MTU</th>
    <th>User Name</th>
    <th>Password</th>
    <th>Dial</th>
    <th>COM Device</th>
    <th>Baud Rate</th>
    <th>Flow Control</th>
    </tr>
    <xsl:apply-templates select="NetIf" mode="PPP_L1" />
    </table>
    <table>
    <tr>
    <th>ID Symbol</th>
    <th>Obtain an IP address auto</th>
    <th>Local IP address</th>
    <th>Remote IP address</th>
    <th>Obtain DNS server address auto</th>
    <th>Preffered DNS server</th>
    <th>Alternate DNS server</th>
    <th>Auth Protocol</th>
    <th>VJ Compression</th>
    <th>VJ Slots</th>
    <th>Retry Count</th>
    <th>Retry Interval(ms)</th>
    </tr>
    <xsl:apply-templates select="NetIf" mode="PPP_L2" />
    </table>

    <h4>Socket</h4>
    <table>
    <tr>
    <th>ID Symbol</th>
    <th>Binding to Interface</th>
    <th>IP Version</th>
    <th>Protocol</th>
    <th>Local Port</th>
    <th>SND_SOC timeout (msec)</th>
    <th>RCV_SOC timeout (msec)</th>
    <th>CON_SOC timeout (msec)</th>
    <th>CLS_SOC timeout (msec)</th>
    <th>Send buffer size (byte)</th>
    <th>Receive buffer size (byte)</th>
    </tr>
    <xsl:apply-templates select="Socket" />
    </table>
    
    <h4>Net Application(HTTPd)</h4>
    <table>
    <tr>
    <th>Use</th>
    <th>Session maximum</th>
    </tr>
    <xsl:apply-templates select="NetApplication/HTTPd" />
    </table>
    <!-- contents リスト -->
    <table>
    <tr>
    <th>Content-Type</th>
    <th>URL</th>
    <th>Resource</th>
    </tr>
    <xsl:apply-templates select="NetApplication/HTTPd/Content" />
    </table>
    
    <h4>Net Application(DHCP Client)</h4>
    <table>
    <tr>
    <th>Use DHCP Client Extention</th>
    <th>Retry Count</th>
    </tr>
    <xsl:apply-templates select="NetApplication/DHCPClient" />
    </table>

    <h4>Net Application(PING)</h4>
    <table>
    <tr>
    <th>Use ICMP Echo Request</th>
    </tr>
    <xsl:apply-templates select="NetApplication/Ping" />
    </table>

    <h4>SSL</h4>
    <table>
    <tr>
      <th>Use</th>
      <th>SSL Version</th>
      <th>Max certification size (byte)</th>
      <th>Max certification depth</th>
      <th>Max number of certificates of the client specified by ssl_cli_ini</th>
      <th>Max number of sessions</th>
      <th>Max number of connections</th>
      <th>Memory block size(byte)</th>
      <th>Memory block counts</th>
      <th>RC4 MD5</th>
      <th>RC4 SHA1</th>
      <th>TDES</th>
      <th>AES-128</th>
      <th>AES-256</th>
      <th>Cryptographic processor Timeout (msec)</th>
      <th>HASH processor Timeout (msec)</th>
      <th>Issue the establishment of session(con_ssoc) of multiple tasks with different priorities.</th>
    </tr>
      <xsl:apply-templates select="SSL" />
    </table>

    </xsl:template>

  <!-- 全般 -->
  <xsl:template match="Middle-uNet3/General">
    <table>
      <tr>
        <th>Use uNet3</th>
        <th>Task(ID Symbol) to start uNet3</th>
        <th>Easy mode</th>
        <th>Customize</th>
      </tr>
      <tr>
        <td>
        <xsl:choose>
          <xsl:when test="CFG_UNET3_USE=1">Yes</xsl:when>
          <xsl:when test="CFG_UNET3_USE='true'">Yes</xsl:when>
          <xsl:when test="CFG_UNET3_USE=0">No</xsl:when>
          <xsl:when test="CFG_UNET3_USE='false'">No</xsl:when>
        </xsl:choose>
        </td>
        <td><xsl:value-of select="CFG_UNET3_INITTSK"/></td>
        <td><xsl:value-of select="CFG_UNET3_PARAM_MODE"/></td>
        <td>
        <xsl:choose>
          <xsl:when test="CFG_UNET3_PARAM_CUSTOMIZE=1">Yes</xsl:when>
          <xsl:when test="CFG_UNET3_PARAM_CUSTOMIZE='true'">Yes</xsl:when>
          <xsl:when test="CFG_UNET3_PARAM_CUSTOMIZE=0">No</xsl:when>
          <xsl:when test="CFG_UNET3_PARAM_CUSTOMIZE='false'">No</xsl:when>
        </xsl:choose>
        </td>
      </tr>
    </table>
    <h4>Basic Setting</h4>
    <xsl:apply-templates select="Basic" />
    <h4>IP Setting</h4>
    <xsl:apply-templates select="IP" />
    <h4>ARP Setting</h4>
    <xsl:apply-templates select="ARP" />
    <h4>TCP Setting</h4>
    <xsl:apply-templates select="TCP" />
    <h4>UDP Setting</h4>
    <xsl:apply-templates select="UDP" />
    <h4>IPv6 Setting</h4>
    <xsl:apply-templates select="IPv6" />
  </xsl:template>

  <!-- 基本設定 -->
  <xsl:template match="Middle-uNet3/General/Basic">
    <table>
      <tr>
        <th>Netwrok buffer count</th>
        <th>ARP cache table size</th>
        <th>IP reassemble process table size</th>
        <th>Joining multicast group table size</th>
      </tr>
      <tr>
        <td><xsl:value-of select="CFG_UNET3_NETBUF_CNT"/></td>
        <td><xsl:value-of select="CFG_UNET3_ARPMAX"/></td>
        <td><xsl:value-of select="CFG_UNET3_IPRMAX"/></td>
        <td><xsl:value-of select="CFG_UNET3_MGRMAX"/></td>
      </tr>
    </table>
  </xsl:template>
  
  <!-- IP設定 -->
  <xsl:template match="Middle-uNet3/General/IP">
    <table>
      <tr>
        <th>TTL</th>
        <th>TOS</th>
        <th>Waiting time for fragmented IP (sec)</th>
        <th>Ignore IP reception checksum</th>
      </tr>
      <tr>
        <td><xsl:value-of select="CFG_UNET3_IP4_TTL"/></td>
        <td><xsl:value-of select="CFG_UNET3_IP4_TOS"/></td>
        <td><xsl:value-of select="CFG_UNET3_IP4_IPR_TMO"/></td>
        <td>
        <xsl:choose>
          <xsl:when test="CFG_UNET3_IP4_IGNORE_CHECKSUM=1">Yes</xsl:when>
          <xsl:when test="CFG_UNET3_IP4_IGNORE_CHECKSUM='true'">Yes</xsl:when>
          <xsl:when test="CFG_UNET3_IP4_IGNORE_CHECKSUM=0">No</xsl:when>
          <xsl:when test="CFG_UNET3_IP4_IGNORE_CHECKSUM='false'">No</xsl:when>
        </xsl:choose>
        </td>
      </tr>
    </table>
  </xsl:template>

  <!-- ARP設定 -->
  <xsl:template match="Middle-uNet3/General/ARP">
    <table>
      <tr>
        <th>Retry count</th>
        <th>Retry timeout (sec)</th>
        <th>Cache timeout (minutes)</th>
      </tr>
      <tr>
        <td><xsl:value-of select="CFG_UNET3_ARP_RET_CNT"/></td>
        <td><xsl:value-of select="CFG_UNET3_ARP_RET_TMO"/></td>
        <td><xsl:value-of select="CFG_UNET3_ARP_CLR_TMO"/></td>
      </tr>
    </table>
  </xsl:template>

  <!-- TCP設定 -->
  <xsl:template match="Middle-uNet3/General/TCP">
    <table>
      <tr>
        <th>Open connection timeout (sec)</th>
        <th>Transmission timeout (sec)</th>
        <th>Close connection timeout (sec)</th>
        <th>Ignore TCP reception checksum</th>
        <th>The number of transmission Keep-Avlie</th>
        <th>The time of activation Keep-Avlie(sec)</th>
        <th>The interval of transmission Keep-Avlie (sec)</th>
      </tr>
      <tr>
        <td><xsl:value-of select="CFG_UNET3_TCP_CON_TMO"/></td>
        <td><xsl:value-of select="CFG_UNET3_TCP_SND_TMO"/></td>
        <td><xsl:value-of select="CFG_UNET3_TCP_CLS_TMO"/></td>
        <td>
        <xsl:choose>
          <xsl:when test="CFG_UNET3_TCP_IGNORE_CHECKSUM=1">Yes</xsl:when>
          <xsl:when test="CFG_UNET3_TCP_IGNORE_CHECKSUM='true'">Yes</xsl:when>
          <xsl:when test="CFG_UNET3_TCP_IGNORE_CHECKSUM=0">No</xsl:when>
          <xsl:when test="CFG_UNET3_TCP_IGNORE_CHECKSUM='false'">No</xsl:when>
        </xsl:choose>
        </td>
        <td><xsl:value-of select="CFG_UNET3_TCP_KPA_CNT"/></td>
        <td><xsl:value-of select="CFG_UNET3_TCP_KPA_TMO"/></td>
        <td><xsl:value-of select="CFG_UNET3_TCP_KPA_INT"/></td>
      </tr>
    </table>
  </xsl:template>

  <!-- UDP設定 -->
  <xsl:template match="Middle-uNet3/General/UDP">
    <table>
      <tr>
        <th>Reception queue size</th>
        <th>Ignore UDP reception checksum</th>
        <th>Enable ICMP port unreachable</th>
      </tr>
      <tr>
        <td><xsl:value-of select="CFG_UNET3_PKT_RCV_QUE"/></td>
        <td>
        <xsl:choose>
          <xsl:when test="CFG_UNET3_UDP_IGNORE_CHECKSUM=1">Yes</xsl:when>
          <xsl:when test="CFG_UNET3_UDP_IGNORE_CHECKSUM='true'">Yes</xsl:when>
          <xsl:when test="CFG_UNET3_UDP_IGNORE_CHECKSUM=0">No</xsl:when>
          <xsl:when test="CFG_UNET3_UDP_IGNORE_CHECKSUM='false'">No</xsl:when>
        </xsl:choose>
        </td>
        <td>
        <xsl:choose>
          <xsl:when test="CFG_UNET3_PKT_UNREACH=1">Enable</xsl:when>
          <xsl:when test="CFG_UNET3_PKT_UNREACH='true'">Enable</xsl:when>
          <xsl:when test="CFG_UNET3_PKT_UNREACH=0">Disable</xsl:when>
          <xsl:when test="CFG_UNET3_PKT_UNREACH='false'">Disable</xsl:when>
        </xsl:choose>
        </td>
      </tr>
    </table>
  </xsl:template>

  <!-- IPv6設定 -->
  <xsl:template match="Middle-uNet3/General/IPv6">
    <table>
      <tr>
        <th>Destination cache</th>
        <th>Neighbor cache</th>
        <th>Prefix List</th>
        <th>Default Router List</th>
        <th>Path MTU cache</th>
      </tr>
      <tr>
        <td><xsl:value-of select="CFG_UNET3_DST_CACHE"/></td>
        <td><xsl:value-of select="CFG_UNET3_NEIGH_CACHE"/></td>
        <td><xsl:value-of select="CFG_UNET3_PRFX_LST"/></td>
        <td><xsl:value-of select="CFG_UNET3_RTR_LST"/></td>
        <td><xsl:value-of select="CFG_UNET3_PMTU_CACHE"/></td>
      </tr>
    </table>
  </xsl:template>
  
  <!-- ネットワークインタフェース -->
  <xsl:template match="NetIf">
  <xsl:if test="not(contains(CFG_UNET3_IFDEV, 'PPP'))">
    <tr>
      <td><xsl:value-of select="@CFG_UNET3_IFID"/></td>
      <td><xsl:value-of select="CFG_UNET3_IFDEV"/></td>
      <td><xsl:value-of select="CFG_UNET3_IFMTU"/></td>
      <td><xsl:value-of select="CFG_UNET3_IFMAC"/></td>
      <td>
      <xsl:choose>
        <xsl:when test="IPv4/CFG_UNET3_IF_DHCP_USE=1">Yes</xsl:when>
        <xsl:when test="IPv4/CFG_UNET3_IF_DHCP_USE='true'">Yes</xsl:when>
        <xsl:when test="IPv4/CFG_UNET3_IF_DHCP_USE=0">No</xsl:when>
        <xsl:when test="IPv4/CFG_UNET3_IF_DHCP_USE='false'">No</xsl:when>
      </xsl:choose>
      </td>
      <td><xsl:value-of select="IPv4/CFG_UNET3_IF_IPADDR"/></td>
      <td><xsl:value-of select="IPv4/CFG_UNET3_IF_SUBNET"/></td>
      <td><xsl:value-of select="IPv4/CFG_UNET3_IF_GATEWAY"/></td>
      <td>
      <xsl:choose>
        <xsl:when test="IPv4/CFG_UNET3_IF_CHECKDUP=1">Yes</xsl:when>
        <xsl:when test="IPv4/CFG_UNET3_IF_CHECKDUP='true'">Yes</xsl:when>
        <xsl:when test="IPv4/CFG_UNET3_IF_CHECKDUP=0">No</xsl:when>
        <xsl:when test="IPv4/CFG_UNET3_IF_CHECKDUP='false'">No</xsl:when>
      </xsl:choose>
      </td>
      <td>
      <xsl:choose>
        <xsl:when test="IPv6/CFG_UNET3_IF_IP6_STATIC=1">Yes</xsl:when>
        <xsl:when test="IPv6/CFG_UNET3_IF_IP6_STATIC='true'">Yes</xsl:when>
        <xsl:when test="IPv6/CFG_UNET3_IF_IP6_STATIC=0">No</xsl:when>
        <xsl:when test="IPv6/CFG_UNET3_IF_IP6_STATIC='false'">No</xsl:when>
      </xsl:choose>
      </td>
      <td><xsl:value-of select="IPv6/CFG_UNET3_IF_IP6_ADDR"/></td>
      <td><xsl:value-of select="IPv6/CFG_UNET3_IF_IP6_PREFIXLEN"/></td>
      <td><xsl:value-of select="IPv6/CFG_UNET3_IF_IP6_ROUTER"/></td>
    </tr>
  </xsl:if>
  </xsl:template>

  <!-- PPP設定 -->
  <xsl:template match="NetIf" mode="PPP_L1">
    <xsl:if test="contains(CFG_UNET3_IFDEV, 'PPP')">
      <tr>
        <td><xsl:value-of select="@CFG_UNET3_IFID"/></td>
        <td><xsl:value-of select="CFG_UNET3_IFDEV"/></td>
        <td><xsl:value-of select="CFG_UNET3_IFMTU"/></td>
        <td><xsl:value-of select="CFG_UNET3_PPP_USERNAME"/></td>
        <td><xsl:value-of select="CFG_UNET3_PPP_PASSWORD"/></td>
        <td><xsl:value-of select="CFG_UNET3_PPP_DIAL"/></td>
        <td><xsl:value-of select="CFG_UNET3_PPP_MODEM_DEVID"/></td>
        <td><xsl:value-of select="CFG_UNET3_PPP_MODEM_BAUD"/></td>
        <td><xsl:choose>
          <xsl:when test="CFG_UNET3_PPP_MODEM_FLOW=0">None</xsl:when>
          <xsl:when test="CFG_UNET3_PPP_MODEM_FLOW=1">Xon</xsl:when>
          <xsl:when test="CFG_UNET3_PPP_MODEM_FLOW=2">Hardware</xsl:when>
        </xsl:choose></td>
      </tr>
    </xsl:if>
  </xsl:template>
  <xsl:template match="NetIf" mode="PPP_L2">
    <xsl:if test="contains(CFG_UNET3_IFDEV, 'PPP')">
      <tr>
        <td><xsl:value-of select="@CFG_UNET3_IFID"/></td>
        <td><xsl:choose>
          <xsl:when test="CFG_UNET3_PPP_AUTO_IP=0">Yes</xsl:when>
          <xsl:otherwise>No</xsl:otherwise>
        </xsl:choose></td>
        <td><xsl:value-of select="CFG_UNET3_PPP_IP_LOCAL"/></td>
        <td><xsl:value-of select="CFG_UNET3_PPP_IP_REMOTE"/></td>
        <td><xsl:choose>
          <xsl:when test="CFG_UNET3_PPP_AUTO_DNS=0">Yes</xsl:when>
          <xsl:otherwise>No</xsl:otherwise>
        </xsl:choose></td>
        <td><xsl:value-of select="CFG_UNET3_PPP_DNS_PRIMARY"/></td>
        <td><xsl:value-of select="CFG_UNET3_PPP_DNS_SECONDARY"/></td>
        <td><xsl:choose>
          <xsl:when test="CFG_UNET3_PPP_AUTH_PROTO=0">CHAP</xsl:when>
          <xsl:otherwise>PAP</xsl:otherwise>
        </xsl:choose></td>
        <td><xsl:choose>
          <xsl:when test="CFG_UNET3_PPP_VJ_COMPRESS=0">Yes</xsl:when>
          <xsl:otherwise>No</xsl:otherwise>
        </xsl:choose></td>
        <td><xsl:value-of select="CFG_UNET3_PPP_VJ_SLOTS"/></td>
        <td><xsl:value-of select="CFG_UNET3_PPP_RETRY_CNT"/></td>
        <td><xsl:value-of select="CFG_UNET3_PPP_RETRY_INTERVAL"/></td>
      </tr>
    </xsl:if>
  </xsl:template>

  <!-- ソケット -->
  <xsl:template match="Socket">
    <tr>
      <td><xsl:value-of select="@CFG_UNET3_SOCID"/></td>
      <td><xsl:value-of select="CFG_UNET3_SOC_BINDIF"/></td>
      <td>
      <xsl:choose>
        <xsl:when test="CFG_UNET3_SOC_IPVER=0">IPv4</xsl:when>
        <xsl:when test="CFG_UNET3_SOC_IPVER=1">IPv6</xsl:when>
      </xsl:choose>
      </td>
      <td>
      <xsl:choose>
        <xsl:when test="CFG_UNET3_SOCPROTO=6">TCP</xsl:when>
        <xsl:when test="CFG_UNET3_SOCPROTO=17">UDP</xsl:when>
        <xsl:when test="CFG_UNET3_SOCPROTO=1">ICMP</xsl:when>
        <xsl:when test="CFG_UNET3_SOCPROTO=58">ICMP6</xsl:when>
      </xsl:choose>
      </td>
      <td><xsl:value-of select="CFG_UNET3_SOCPORT"/></td>
      <td><xsl:value-of select="CFG_UNET3_SOC_SNDTMO"/></td>
      <td><xsl:value-of select="CFG_UNET3_SOC_RCVTMO"/></td>
      <td><xsl:value-of select="CFG_UNET3_SOC_CONTMO"/></td>
      <td><xsl:value-of select="CFG_UNET3_SOC_CLSTMO"/></td>
      <td><xsl:value-of select="CFG_UNET3_SOC_SNDBUF"/></td>
      <td><xsl:value-of select="CFG_UNET3_SOC_RCVBUF"/></td>
    </tr>
  </xsl:template>

  <!-- HTTPd -->
  <xsl:template match="NetApplication/HTTPd">
    <tr>
      <td>
      <xsl:choose>
        <xsl:when test="@enable='true'">Yes</xsl:when>
        <xsl:when test="@enable='false'">No</xsl:when>
      </xsl:choose>
      </td>
      <td><xsl:value-of select="CFG_UNET3_HTTPD_SESSION"/></td>
    </tr>
  </xsl:template>
  <!-- コンテンツ一覧 -->
  <xsl:template match="NetApplication/HTTPd/Content">
    <tr>
    <td><xsl:value-of select="CFG_UNET3_HTTPD_CONTENT_TYPE"/></td>
    <td><xsl:value-of select="CFG_UNET3_HTTPD_CONTENT_URL"/></td>
    <td><xsl:value-of select="CFG_UNET3_HTTPD_CONTENT_RES"/></td>
    </tr>
  </xsl:template>

  <!-- DHCPClient -->
  <xsl:template match="NetApplication/DHCPClient">
    <tr>
      <td>
      <xsl:choose>
        <xsl:when test="CFG_UNET3_DHCPCLI_EXT_USE='true'">Yes</xsl:when>
        <xsl:when test="CFG_UNET3_DHCPCLI_EXT_USE='false'">No</xsl:when>
      </xsl:choose>
      </td>
      <td><xsl:value-of select="CFG_UNET3_DHCPCLI_RETRY_CNT"/></td>
    </tr>
  </xsl:template>
  
  <!-- DHCPClient -->
  <xsl:template match="NetApplication/Ping">
    <tr>
      <td>
      <xsl:choose>
        <xsl:when test="CFG_UNET3_PING_ICMP_ECHO_USE='true'">Yes</xsl:when>
        <xsl:when test="CFG_UNET3_PING_ICMP_ECHO_USE='false'">No</xsl:when>
      </xsl:choose>
      </td>
    </tr>
  </xsl:template>

  <!-- SSL -->
  <xsl:template match="SSL">
    <tr>
      <td><xsl:choose>
        <xsl:when test="@enable='true'">Yes</xsl:when>
        <xsl:when test="@enable='false'">No</xsl:when>
      </xsl:choose></td>
      <td><xsl:choose>
        <xsl:when test="CFG_UNET3_SSL_VERSION=0">SSL v3.0</xsl:when>
        <xsl:when test="CFG_UNET3_SSL_VERSION=1">TLS v1.0</xsl:when>
        <xsl:when test="CFG_UNET3_SSL_VERSION=2">TLS v1.1</xsl:when>
      </xsl:choose></td>
      <td><xsl:value-of select="CFG_UNET3_SSL_CERT_SIZE"/></td>
      <td><xsl:value-of select="CFG_UNET3_SSL_CERT_DEPTH"/></td>
      <td><xsl:value-of select="CFG_UNET3_SSL_CA_CERT_MAX"/></td>
      <td><xsl:value-of select="CFG_UNET3_SSL_MAX_SESSION"/></td>
      <td><xsl:value-of select="CFG_UNET3_SSL_MAX_CONNECTION"/></td>
      <td><xsl:value-of select="CFG_UNET3_SSL_NET_BUF_MPF_SZ"/></td>
      <td><xsl:value-of select="CFG_UNET3_SSL_NET_BUF_MPF_BLK"/></td>
      <td><xsl:choose>
        <xsl:when test="CFG_UNET3_SSL_CIPHER_SPEC0='true'">Enable</xsl:when>
        <xsl:when test="CFG_UNET3_SSL_CIPHER_SPEC0='false'">Disable</xsl:when>
      </xsl:choose></td>
      <td><xsl:choose>
        <xsl:when test="CFG_UNET3_SSL_CIPHER_SPEC1='true'">Enable</xsl:when>
        <xsl:when test="CFG_UNET3_SSL_CIPHER_SPEC1='false'">Disable</xsl:when>
      </xsl:choose></td>
      <td><xsl:choose>
        <xsl:when test="CFG_UNET3_SSL_CIPHER_SPEC2='true'">Enable</xsl:when>
        <xsl:when test="CFG_UNET3_SSL_CIPHER_SPEC2='false'">Disable</xsl:when>
      </xsl:choose></td>
      <td><xsl:choose>
        <xsl:when test="CFG_UNET3_SSL_CIPHER_SPEC3='true'">Enable</xsl:when>
        <xsl:when test="CFG_UNET3_SSL_CIPHER_SPEC3='false'">Disable</xsl:when>
      </xsl:choose></td>
      <td><xsl:choose>
        <xsl:when test="CFG_UNET3_SSL_CIPHER_SPEC4='true'">Enable</xsl:when>
        <xsl:when test="CFG_UNET3_SSL_CIPHER_SPEC4='false'">Disable</xsl:when>
      </xsl:choose></td>
      <td><xsl:value-of select="CFG_UNET3_SSL_CRYP_TMO"/></td>
      <td><xsl:value-of select="CFG_UNET3_SSL_HASH_TMO"/></td>
      <td><xsl:choose>
        <xsl:when test="CFG_UNET3_SSL_ALLOW_MULTSK='true'">Enable</xsl:when>
        <xsl:when test="CFG_UNET3_SSL_ALLOW_MULTSK='false'">Disable</xsl:when>
      </xsl:choose></td>
    </tr>
  </xsl:template>



  </xsl:stylesheet>
