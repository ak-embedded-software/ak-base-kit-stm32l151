#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zb_zcl.h"
#include "zb_defs.h"

#if defined(TASK_ZIGBEE_EN)
#include "app.h"
#include "task_list.h"
#endif

/*********************************************************************
 * MACROS
 */
/*** Frame Control ***/
#define zcl_FCType( a )               ( (a) & ZCL_FRAME_CONTROL_TYPE )
#define zcl_FCManuSpecific( a )       ( (a) & ZCL_FRAME_CONTROL_MANU_SPECIFIC )
#define zcl_FCDirection( a )          ( (a) & ZCL_FRAME_CONTROL_DIRECTION )
#define zcl_FCDisableDefaultRsp( a )  ( (a) & ZCL_FRAME_CONTROL_DISABLE_DEFAULT_RSP )

/*** Attribute Access Control ***/
#define zcl_AccessCtrlRead( a )       ( (a) & ACCESS_CONTROL_READ )
#define zcl_AccessCtrlWrite( a )      ( (a) & ACCESS_CONTROL_WRITE )
#define zcl_AccessCtrlCmd( a )        ( (a) & ACCESS_CONTROL_CMD )
#define zcl_AccessCtrlAuthRead( a )   ( (a) & ACCESS_CONTROL_AUTH_READ )
#define zcl_AccessCtrlAuthWrite( a )  ( (a) & ACCESS_CONTROL_AUTH_WRITE )

#define zclParseCmd( a, b )           zclCmdTable[(a)].pfnParseInProfile( (b) )
#define zclProcessCmd( a, b )         zclCmdTable[(a)].pfnProcessInProfile( (b) )

#define zcl_DefaultRspCmd( zclHdr )   ( zcl_ProfileCmd( (zclHdr).fc.type )     && \
	(zclHdr).fc.manuSpecific == 0          && \
	(zclHdr).commandID == ZCL_CMD_DEFAULT_RSP )

// Commands that have corresponding responses
#define CMD_HAS_RSP( cmd )            ( (cmd) == ZCL_CMD_READ                   || \
	(cmd) == ZCL_CMD_READ_RSP               || \
	(cmd) == ZCL_CMD_WRITE                  || \
	(cmd) == ZCL_CMD_WRITE_UNDIVIDED        || \
	(cmd) == ZCL_CMD_CONFIG_REPORT          || \
	(cmd) == ZCL_CMD_READ_REPORT_CFG        || \
	(cmd) == ZCL_CMD_DISCOVER_ATTRS         || \
	(cmd) == ZCL_CMD_DISCOVER_CMDS_RECEIVED || \
	(cmd) == ZCL_CMD_DISCOVER_CMDS_GEN      || \
	(cmd) == ZCL_CMD_DISCOVER_ATTRS_EXT     || \
	(cmd) == ZCL_CMD_REPORT					|| \
	(cmd) == ZCL_CMD_DEFAULT_RSP ) // exception

static afIncomingMSGPacket_t *rawAFMsg = (afIncomingMSGPacket_t *) NULL;

static void *zclParseInReadRspCmd(zclParseCmd_t *pCmd);
static void *zclParseInReportCmd(zclParseCmd_t *pCmd);

/*********************************************************************
 * @fn      zclParseHdr
 *
 * @brief   Parse header of the ZCL format
 *
 * @param   hdr - place to put the frame control information
 * @param   pData - incoming buffer to parse
 *
 * @return  pointer past the header
 */
uint8_t *zclParseHdr( zclFrameHdr_t *hdr, uint8_t *pData )
{
	// Clear the header
	memset( (uint8_t *)hdr, 0, sizeof ( zclFrameHdr_t ) );

	// Parse the Frame Control
	hdr->fc.type = zcl_FCType( *pData );
	hdr->fc.manuSpecific = zcl_FCManuSpecific( *pData ) ? 1 : 0;
	if ( zcl_FCDirection( *pData ) )
	{
		hdr->fc.direction = ZCL_FRAME_SERVER_CLIENT_DIR;
	}
	else
	{
		hdr->fc.direction = ZCL_FRAME_CLIENT_SERVER_DIR;
	}

	hdr->fc.disableDefaultRsp = zcl_FCDisableDefaultRsp( *pData ) ? 1 : 0;
	pData++;  // move past the frame control field

	// parse the manfacturer code
	if ( hdr->fc.manuSpecific )
	{
		hdr->manuCode = BUILD_UINT16( pData[0], pData[1] );
		pData += 2;
	}

	// parse the Transaction Sequence Number
	hdr->transSeqNum = *pData++;

	// parse the Cluster's command ID
	hdr->commandID = *pData++;

	// Should point to the frame payload
	return ( pData );
}

static void *zclParseInReadRspCmd(zclParseCmd_t *pCmd) {
	zclReadRspCmd_t *readRspCmd;
	uint8_t *pBuf = pCmd->pData;
	uint8_t *dataPtr;
	uint8_t numAttr = 0;
	uint8_t hdrLen;
	uint16_t dataLen = 0;
	uint16_t attrDataLen;

	// find out the number of attributes and the length of attribute data
	while (pBuf < (pCmd->pData + pCmd->dataLen)) {
		uint8_t status;

		numAttr++;
		pBuf += 2; // move pass attribute id

		status = *pBuf++;
		if (status == ZCL_STATUS_SUCCESS) {
			uint8_t dataType = *pBuf++;

			attrDataLen = zclGetAttrDataLength(dataType, pBuf, pCmd->dataLen);

			pBuf += attrDataLen; // move pass attribute data

			// add padding if needed
			if (PADDING_NEEDED(attrDataLen)) {
				attrDataLen++;
			}

			dataLen += attrDataLen;
		}
	}

	// calculate the length of the response header
	hdrLen = sizeof(zclReadRspCmd_t) + (numAttr * sizeof(zclReadRspStatus_t));

	readRspCmd = (zclReadRspCmd_t *) ak_malloc(hdrLen + dataLen);
	if (readRspCmd != NULL) {
		uint8_t i;
		pBuf = pCmd->pData;
		dataPtr = (uint8_t *) ((uint8_t *) readRspCmd + hdrLen); // point to data attiture

		readRspCmd->numAttr = numAttr;
		for (i = 0; i < numAttr; i++) {
			zclReadRspStatus_t *statusRec = &(readRspCmd->attrList[i]);

			statusRec->attrID = BUILD_UINT16(pBuf[0], pBuf[1]);
			pBuf += 2;
			//
			statusRec->status = *pBuf++;
			if (statusRec->status == ZCL_STATUS_SUCCESS) {
				statusRec->dataType = *pBuf++;

				attrDataLen = zclGetAttrDataLength(statusRec->dataType, pBuf, pCmd->dataLen);
				statusRec->dataLen = attrDataLen;
				memcpy(dataPtr, pBuf, attrDataLen);
				statusRec->data = dataPtr;

				pBuf += attrDataLen; // move pass attribute data

				// advance attribute data pointer
				if (PADDING_NEEDED(attrDataLen)) {
					attrDataLen++;
				}

				dataPtr += attrDataLen;
			}
		}
	}

	return ((void *) readRspCmd);
}

static void *zclParseInReportCmd(zclParseCmd_t *pCmd) {
	zclReportCmd_t *reportCmd;
	uint8_t *pBuf = pCmd->pData;
	uint16_t attrDataLen;
	uint8_t *dataPtr;
	uint8_t numAttr = 0;
	uint8_t hdrLen;
	uint16_t dataLen = 0;

	// find out the number of attributes and the length of attribute data
	while (pBuf < (pCmd->pData + pCmd->dataLen)) {
		uint8_t dataType;

		numAttr++;
		pBuf += 2; // move pass attribute id

		dataType = *pBuf++;

		attrDataLen = zclGetAttrDataLength(dataType, pBuf, pCmd->dataLen);
		pBuf += attrDataLen; // move pass attribute data

		// add padding if needed
		if (PADDING_NEEDED(attrDataLen)) {
			attrDataLen++;
		}

		dataLen += attrDataLen;
	}

	hdrLen = sizeof(zclReportCmd_t) + (numAttr * sizeof(zclReport_t));

	reportCmd = (zclReportCmd_t *) ak_malloc(hdrLen + dataLen);
	if (reportCmd != NULL) {
		uint8_t i;
		pBuf = pCmd->pData;
		dataPtr = (uint8_t *) ((uint8_t *) reportCmd + hdrLen);

		reportCmd->numAttr = numAttr;
		for (i = 0; i < numAttr; i++) {
			zclReport_t *reportRec = &(reportCmd->attrList[i]);

			reportRec->attrID = BUILD_UINT16(pBuf[0], pBuf[1]);
			pBuf += 2;
			reportRec->dataType = *pBuf++;

			attrDataLen = zclGetAttrDataLength(reportRec->dataType, pBuf, pCmd->dataLen);
			reportRec->dataLen = attrDataLen;
			memcpy(dataPtr, pBuf, attrDataLen);
			reportRec->Data = dataPtr;

			pBuf += attrDataLen; // move pass attribute data

			// advance attribute data pointer
			if (PADDING_NEEDED(attrDataLen)) {
				attrDataLen++;
			}

			dataPtr += attrDataLen;
		}
	}

	return ((void *) reportCmd);
}

uint16_t zclGetAttrDataLength(uint8_t dataType, uint8_t *pData, uint16_t len) {
	uint16_t dataLen = 0;

	if (dataType == ZCL_DATATYPE_LONG_CHAR_STR || dataType == ZCL_DATATYPE_LONG_OCTET_STR) {
		dataLen = BUILD_UINT16( pData[0], pData[1] ) + 2; // long string length + 2 for length field
	} else if (dataType == ZCL_DATATYPE_CHAR_STR || dataType == ZCL_DATATYPE_OCTET_STR) {
		dataLen = *pData + 1; // string length + 1 for length field
	} else if (dataType == ZCL_DATATYPE_STRUCT) {
		dataLen = len - 3; //TODO: need to customize here
	} else {
		dataLen = zclGetDataTypeLength(dataType);
	}

	return (dataLen);
}

uint8_t zclGetDataTypeLength(uint8_t dataType) {
	uint8_t len;

	switch (dataType) {
	case ZCL_DATATYPE_DATA8:
	case ZCL_DATATYPE_BOOLEAN:
	case ZCL_DATATYPE_BITMAP8:
	case ZCL_DATATYPE_INT8:
	case ZCL_DATATYPE_UINT8:
	case ZCL_DATATYPE_ENUM8:
		len = 1;
		break;

	case ZCL_DATATYPE_DATA16:
	case ZCL_DATATYPE_BITMAP16:
	case ZCL_DATATYPE_UINT16:
	case ZCL_DATATYPE_INT16:
	case ZCL_DATATYPE_ENUM16:
	case ZCL_DATATYPE_SEMI_PREC:
	case ZCL_DATATYPE_CLUSTER_ID:
	case ZCL_DATATYPE_ATTR_ID:
		len = 2;
		break;

	case ZCL_DATATYPE_DATA24:
	case ZCL_DATATYPE_BITMAP24:
	case ZCL_DATATYPE_UINT24:
	case ZCL_DATATYPE_INT24:
		len = 3;
		break;

	case ZCL_DATATYPE_DATA32:
	case ZCL_DATATYPE_BITMAP32:
	case ZCL_DATATYPE_UINT32:
	case ZCL_DATATYPE_INT32:
	case ZCL_DATATYPE_SINGLE_PREC:
	case ZCL_DATATYPE_TOD:
	case ZCL_DATATYPE_DATE:
	case ZCL_DATATYPE_UTC:
	case ZCL_DATATYPE_BAC_OID:
		len = 4;
		break;

	case ZCL_DATATYPE_UINT40:
	case ZCL_DATATYPE_INT40:
		len = 5;
		break;

	case ZCL_DATATYPE_UINT48:
	case ZCL_DATATYPE_INT48:
		len = 6;
		break;

	case ZCL_DATATYPE_UINT56:
	case ZCL_DATATYPE_INT56:
		len = 7;
		break;

	case ZCL_DATATYPE_DOUBLE_PREC:
	case ZCL_DATATYPE_IEEE_ADDR:
	case ZCL_DATATYPE_UINT64:
	case ZCL_DATATYPE_INT64:
		len = 8;
		break;

	case ZCL_DATATYPE_128_BIT_SEC_KEY:
		len = SEC_KEY_LEN;
		break;

	case ZCL_DATATYPE_NO_DATA:
	case ZCL_DATATYPE_UNKNOWN:
		// Fall through

	default:
		len = 0;
		break;
	}

	return (len);
}

zclProcMsgStatus_t zcl_ProcessMessageMSG(afIncomingMSGPacket_t *pkt) {
	zclIncoming_t inMsg;

	if (pkt->len == 0) {
		return (ZCL_PROC_INVALID);   // Error, ignore the message
	}

	rawAFMsg = (afIncomingMSGPacket_t *) pkt;
	inMsg.msg = pkt;
	inMsg.attrCmd = NULL;
	inMsg.pData = NULL;
	inMsg.pDataLen = 0;

	inMsg.pData = zclParseHdr(&(inMsg.hdr), (uint8_t*) &pkt->payload[0]);
	inMsg.pDataLen = pkt->len;
	inMsg.pDataLen -= (uint16_t) (inMsg.pData - pkt->payload);

	// Is this a foundation type message
	if (zcl_ProfileCmd(inMsg.hdr.fc.type)) {
		switch (inMsg.hdr.commandID) {
		case ZCL_CMD_READ_RSP: {
			zclParseCmd_t parseCmd;
			parseCmd.endpoint = pkt->dst_endpoint;
			parseCmd.dataLen = inMsg.pDataLen;
			parseCmd.pData = inMsg.pData;

			inMsg.attrCmd = zclParseInReadRspCmd(&parseCmd);

			if (inMsg.attrCmd != NULL) {
				zclReadRspCmd_t *readRspCmd;
				readRspCmd = (zclReadRspCmd_t *) inMsg.attrCmd;
				for (int i = 0; i < readRspCmd->numAttr; i++) {
					zclReadRspStatus_t *statusRec = &(readRspCmd->attrList[i]);
					if (statusRec->status == ZCL_STATUS_SUCCESS) {
#if defined(TASK_ZIGBEE_EN)
						zclOutgoingMsg_t* pOutgoingMsg = (zclOutgoingMsg_t*)ak_malloc(sizeof(zclOutgoingMsg_t));
						pOutgoingMsg->short_addr = pkt->src_addr;
						pOutgoingMsg->cluster_id = pkt->cluster_id;
						pOutgoingMsg->group_id = pkt->group_id;
						pOutgoingMsg->cmd = inMsg.hdr.commandID;
						pOutgoingMsg->attrID = statusRec->attrID;
						pOutgoingMsg->dataType = statusRec->dataType;
						pOutgoingMsg->dataLen = statusRec->dataLen;
						pOutgoingMsg->data = (uint8_t*)ak_malloc(statusRec->dataLen);
						memcpy(pOutgoingMsg->data, statusRec->data, statusRec->dataLen);
						task_post_common_msg(AC_TASK_ZIGBEE_ID, AC_ZIGBEE_ZCL_CMD_HANDLER, (uint8_t*)&pOutgoingMsg, sizeof(zclOutgoingMsg_t*));
#endif
						break;
					} else {
						// statusRec->status?
					}
				}

			}

			// Free the buffer
			if (inMsg.attrCmd) {

				free(inMsg.attrCmd);
			}

			if (CMD_HAS_RSP(inMsg.hdr.commandID))		//Report data commandID = 0x0A
			{
				rawAFMsg = NULL;
				return (ZCL_PROC_SUCCESS); // We're done
			}

			//          status = ZSuccess;
		}
			break;

		case ZCL_CMD_REPORT: {
			zclParseCmd_t parseCmd;
			parseCmd.endpoint = pkt->dst_endpoint;
			parseCmd.dataLen = inMsg.pDataLen;
			parseCmd.pData = inMsg.pData;

			inMsg.attrCmd = zclParseInReportCmd(&parseCmd);

			if (inMsg.attrCmd != NULL) {
				zclReportCmd_t *reportCmd;
				reportCmd = (zclReportCmd_t *) inMsg.attrCmd;
				for (int i = 0; i < reportCmd->numAttr; i++) {
#if defined(TASK_ZIGBEE_EN)
					zclReport_t *reportRec = &(reportCmd->attrList[i]);
					zclOutgoingMsg_t* pOutgoingMsg = (zclOutgoingMsg_t*)ak_malloc(sizeof(zclOutgoingMsg_t));
					pOutgoingMsg->short_addr = pkt->src_addr;
					pOutgoingMsg->cluster_id = pkt->cluster_id;
					pOutgoingMsg->group_id = pkt->group_id;
					pOutgoingMsg->cmd = inMsg.hdr.commandID;
					pOutgoingMsg->attrID = reportRec->attrID;
					pOutgoingMsg->dataType = reportRec->dataType;
					pOutgoingMsg->dataLen = reportRec->dataLen;
					pOutgoingMsg->data = (uint8_t*)ak_malloc(reportRec->dataLen);
					memcpy(pOutgoingMsg->data, reportRec->Data, reportRec->dataLen);
					task_post_common_msg(AC_TASK_ZIGBEE_ID, AC_ZIGBEE_ZCL_CMD_HANDLER, (uint8_t*)&pOutgoingMsg, sizeof(zclOutgoingMsg_t*));
#endif
					break;
				}
			}

			// Free the buffer
			if (inMsg.attrCmd) {
				free(inMsg.attrCmd);
			}

			if (CMD_HAS_RSP(inMsg.hdr.commandID))		//Report data commandID = 0x0A
			{
				rawAFMsg = NULL;
				return (ZCL_PROC_SUCCESS); // We're done
			}
		}
			break;
		default:
			break;
		}
	} else
		// Not a foundation type message, so it must be specific to the cluster ID.

		switch (inMsg.hdr.commandID) {
		case ZCL_CMD_READ: {
			if (pkt->cluster_id == ZCL_CLUSTER_ID_SS_IAS_ZONE) {
				if (inMsg.pDataLen == 6) {
#if defined(TASK_ZIGBEE_EN)
					zclOutgoingMsg_t* pOutgoingMsg = (zclOutgoingMsg_t*)ak_malloc(sizeof(zclOutgoingMsg_t));
					pOutgoingMsg->short_addr = pkt->src_addr;
					pOutgoingMsg->cluster_id = pkt->cluster_id;
					pOutgoingMsg->group_id = pkt->group_id;
					pOutgoingMsg->cmd = inMsg.hdr.commandID;
					pOutgoingMsg->attrID = 0xFFFF;
					pOutgoingMsg->dataType = ZCL_DATATYPE_UNKNOWN;
					pOutgoingMsg->dataLen = inMsg.pDataLen;
					pOutgoingMsg->data = (uint8_t*)ak_malloc(inMsg.pDataLen);
					memcpy(pOutgoingMsg->data, inMsg.pData, inMsg.pDataLen);
					task_post_common_msg(AC_TASK_ZIGBEE_ID, AC_ZIGBEE_ZCL_CMD_HANDLER, (uint8_t*)&pOutgoingMsg, sizeof(zclOutgoingMsg_t*));
#endif
					break;
				}

			}
		}
			break;
		default:
			break;
		}

	{
		return (ZCL_PROC_INVALID); // We're done
	}
}
