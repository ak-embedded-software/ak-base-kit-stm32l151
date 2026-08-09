/* 
 * MODBUS Slave Library: A portable MODBUS slave for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: mbutils.c,v 1.2 2008-03-06 22:01:48 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Platform includes --------------------------------*/

/* ----------------------- Modbus includes ----------------------------------*/

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
USHORT
usUtlCRC16GetTab( UBYTE ubIdx )
{
    USHORT          usCRC16;
    USHORT          usC;
    UBYTE           i;

    usCRC16 = 0;
    usC = ( USHORT ) ubIdx;

    for( i = 0; i < 8; i++ )
    {

        if( ( usCRC16 ^ usC ) & 0x0001 )
        {
            usCRC16 = ( usCRC16 >> 1 ) ^ ( USHORT ) 0xA001U;
        }
        else
        {
            usCRC16 = usCRC16 >> 1;
        }
        usC = usC >> 1;
    }
    return usCRC16;
}

USHORT
prvCRC16Update( USHORT usCRC, UBYTE ubData )
{
    UBYTE           ubCRCLow = usCRC & 0xFF;
    UBYTE           ubCRCHigh = usCRC >> 8U;

    UBYTE           ubIndex = ubCRCLow ^ ubData;
    USHORT          usCRCTableValue = usUtlCRC16GetTab( ubIndex );

    ubCRCLow = ubCRCHigh ^ ( UBYTE ) ( usCRCTableValue & 0xFF );
    ubCRCHigh = ( UBYTE ) ( usCRCTableValue >> 8U );

    return ( ( USHORT ) ubCRCHigh << 8U ) | ( USHORT ) ubCRCLow;
}

STATIC BOOL
bMBGuessFrameIsComplete( UBYTE arubBuffer[], USHORT usLength )
{
    BOOL            bGuessFrameComplete = FALSE;
    /* A MODBUS exception is a valid frame */
    if( ( arubBuffer[0] & 0x80 ) && ( usLength == 2 ) )
    {
        bGuessFrameComplete = TRUE;
    }
    else
    {
        switch ( arubBuffer[0] )
        {
            /* 0x01 (read coils) and 0x02 (read discrete inputs) have the same format */
        case 0x01:
        case 0x02:
            if( 5 == usLength ) /* Request */
            {
                bGuessFrameComplete = TRUE;
            }
            else if( arubBuffer[1] + 2 == usLength )    /* Response */
            {
                bGuessFrameComplete = TRUE;
            }
            break;
            /* 0x03 (read holding registers) and 0x04 (read input registers) have the same format */
        case 0x03:
        case 0x04:
            if( 5 == usLength ) /* Request */
            {
                bGuessFrameComplete = TRUE;
            }
            else if( ( usLength >= 2 ) && ( ( arubBuffer[1] + 2 ) == usLength ) )       /* Response */
            {
                bGuessFrameComplete = TRUE;
            }
            break;
            /* 0x05 (write single coil) and 0x06 (write single register) */
        case 0x05:
        case 0x06:
            if( 5 == usLength ) /* Request and response */
            {
                bGuessFrameComplete = TRUE;
            }
            break;
            /* 0x07 (read exception status) */
        case 0x07:
            if( ( 1 == usLength ) || ( 2 == usLength ) )
            {
                bGuessFrameComplete = TRUE;
            }
            break;
            /* 0x08 (diagnostics) */
        case 0x08:
            if( 5 == usLength ) /* Request and response */
            {

                bGuessFrameComplete = TRUE;
            }
            break;
            /* 0x0B (get comm event counter) */
        case 0x0B:
            if( 1 == usLength ) /* request */
            {
                bGuessFrameComplete = TRUE;
            }
            else if( 5 == usLength )    /* response */
            {
                bGuessFrameComplete = TRUE;
            }
            break;
            /* 0x0C (get comm event log) */
        case 0x0C:
            if( 1 == usLength ) /* request */
            {
                bGuessFrameComplete = TRUE;
            }
            else if( ( usLength >= 2 ) && ( ( arubBuffer[1] + 2 ) == usLength ) )       /* response */
            {
                bGuessFrameComplete = TRUE;
            }
            break;
            /* 0x0F (write multiple coils) */
        case 0x0F:
            if( ( usLength >= 2 ) && ( ( arubBuffer[5] + 6 ) == usLength ) )    /* request */
            {
                bGuessFrameComplete = TRUE;
            }
            else if( 5 == usLength )    /* response */
            {
                bGuessFrameComplete = TRUE;
            }
            break;
            /* 0x10 (write multiple registers) */
        case 0x010:
            if( ( usLength >= 2 ) && ( ( arubBuffer[5] + 6 ) == usLength ) )    /* request */
            {
                bGuessFrameComplete = TRUE;
            }
            else if( 5 == usLength )    /* response */
            {
                bGuessFrameComplete = TRUE;
            }
            break;
            /* 0x11 (report slave id) */
        case 0x11:
            if( 1 == usLength ) /* request */
            {
                bGuessFrameComplete = TRUE;
            }
            else if( ( usLength >= 2 ) && ( ( arubBuffer[1] + 2 ) == usLength ) )       /* response */
            {
                bGuessFrameComplete = TRUE;

            }
            break;
            /* 0x014 (read file record) */
            /* 0x015 (write file record) */
        case 0x14:
            if( ( usLength >= 2 ) && ( ( arubBuffer[1] + 2 ) == usLength ) )    /* request  and response */
            {
                bGuessFrameComplete = TRUE;
            }
            break;

            /* 0x016 (mask write register) */
        case 0x16:
            if( 7 == usLength ) /* request  and response */
            {
                bGuessFrameComplete = TRUE;
            }
            break;

            /* 0x17 (read/write multiple registers) */
        case 0x17:
            if( ( usLength >= 10 ) && ( arubBuffer[9] + 10 ) == usLength )      /* request */
            {
                bGuessFrameComplete = TRUE;
            }
            else if( ( usLength >= 2 ) && ( arubBuffer[1] + 2 ) == usLength )   /* response */
            {
                bGuessFrameComplete = TRUE;
            }
            break;

            /* 0x18 (read FIFO queue) */
        case 0x18:
            if( 3 == usLength ) /* request */
            {
                bGuessFrameComplete = TRUE;
            }
            else if( ( usLength >= 3 ) && ( arubBuffer[2] + 3 ) == usLength )   /* response */
            {
                bGuessFrameComplete = TRUE;
            }
            break;
        }
    }
    return bGuessFrameComplete;
}

BOOL
bMBGuessRTUFrameIsComplete( UBYTE arubBuffer[], USHORT usLength )
{
    BOOL            bGuessFrameComplete = FALSE;
    if( usLength >= 4 )
    {
        return bMBGuessFrameIsComplete( &arubBuffer[1], usLength - 3 );


    }
    return bGuessFrameComplete;
}

eMBException
eMBErrorcodeToException( eMBErrorCode eCode )
{
    eMBException    eException;

    switch ( eCode )
    {
    case MB_EX_ILLEGAL_FUNCTION:
        eException = MB_PDU_EX_ILLEGAL_FUNCTION;
        break;
    case MB_EX_ILLEGAL_DATA_ADDRESS:
        eException = MB_PDU_EX_ILLEGAL_DATA_ADDRESS;
        break;
    case MB_EX_ILLEGAL_DATA_VALUE:
        eException = MB_PDU_EX_ILLEGAL_DATA_VALUE;
        break;
    case MB_EX_SLAVE_DEVICE_FAILURE:
        eException = MB_PDU_EX_SLAVE_DEVICE_FAILURE;
        break;
    case MB_EX_ACKNOWLEDGE:
        eException = MB_PDU_EX_ACKNOWLEDGE;
        break;
    case MB_EX_SLAVE_BUSY:
        eException = MB_PDU_EX_SLAVE_BUSY;
        break;
    case MB_EX_MEMORY_PARITY_ERROR:
        eException = MB_PDU_EX_MEMORY_PARITY_ERROR;
        break;
    case MB_EX_GATEWAY_PATH_UNAVAILABLE:
        eException = MB_PDU_EX_GATEWAY_PATH_UNAVAILABLE;
        break;
    case MB_EX_GATEWAY_TARGET_FAILED:
        eException = MB_PDU_EX_GATEWAY_TARGET_FAILED;
        break;
    default:
        eException = MB_PDU_EX_SLAVE_DEVICE_FAILURE;
        break;
    }
    return eException;
}

eMBErrorCode
eMBExceptionToErrorcode( UBYTE eMBPDUException )
{
    eMBErrorCode    eStatus = MB_EIO;

    switch ( eMBPDUException )
    {
    case MB_PDU_EX_ILLEGAL_FUNCTION:
        eStatus = MB_EX_ILLEGAL_FUNCTION;
        break;
    case MB_PDU_EX_ILLEGAL_DATA_ADDRESS:
        eStatus = MB_EX_ILLEGAL_DATA_ADDRESS;
        break;
    case MB_PDU_EX_ILLEGAL_DATA_VALUE:
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        break;
    case MB_PDU_EX_SLAVE_DEVICE_FAILURE:
        eStatus = MB_EX_SLAVE_DEVICE_FAILURE;
        break;
    case MB_PDU_EX_ACKNOWLEDGE:
        eStatus = MB_EX_ACKNOWLEDGE;
        break;
    case MB_PDU_EX_SLAVE_BUSY:
        eStatus = MB_EX_SLAVE_BUSY;
        break;
    case MB_PDU_EX_MEMORY_PARITY_ERROR:
        eStatus = MB_EX_MEMORY_PARITY_ERROR;
        break;
    case MB_PDU_EX_GATEWAY_PATH_UNAVAILABLE:
        eStatus = MB_EX_GATEWAY_PATH_UNAVAILABLE;
        break;
    case MB_PDU_EX_GATEWAY_TARGET_FAILED:
        eStatus = MB_EX_GATEWAY_TARGET_FAILED;
        break;
    default:
        break;
    }
    return eStatus;
}
