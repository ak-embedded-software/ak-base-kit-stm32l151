/*
 * MODBUS Library: Luminary Cortex M3 serial driver
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: drvserial.h,v 1.3 2010-06-13 17:04:48 embedded-so.embedded-solutions.1 Exp $
 */

#ifndef DRVSERIAL_H
#define DRVSERIAL_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/
#define DRV_SERIAL_EVENT_RXRDY      ( 0x01 )
#define DRV_SERIAL_EVENT_TXRDY      ( 0x02 )
#define DRV_SERIAL_EVENT_ABORT		( 0x04 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/
eMBErrorCode eDrvSerialInit( UBYTE ubPort, ULONG ulBaudRate, UBYTE ucDataBits, eMBSerialParity eParity, UBYTE ucStopBits );
eMBErrorCode eDrvSerialClose( UBYTE ubPort );

eMBErrorCode eDrvSerialAbort( UBYTE ubPort );
eMBErrorCode eDrvSerialWaitEvent( UBYTE ubPort, USHORT * pusEvents, USHORT usTimeOut );

eMBErrorCode eDrvSerialReceiveEnable( UBYTE ubPort );
eMBErrorCode eDrvSerialReceiveDisable( UBYTE ubPort );
eMBErrorCode eDrvSerialReceiveReset( UBYTE ubPort );
eMBErrorCode eDrvSerialReceive( UBYTE ubPort, UBYTE * pubBuffer, USHORT usLengthMax, USHORT * pusNBytesReceived );

eMBErrorCode eDrvSerialTransmitEnable( UBYTE ubPort );
eMBErrorCode eDrvSerialTransmitDisable( UBYTE ubPort );
eMBErrorCode eDrvSerialTransmitFree( UBYTE ubPort, USHORT * pusNFreeBytes );
eMBErrorCode eDrvSerialTransmit( UBYTE ubPort, const UBYTE * pubBuffer, USHORT usLength );

#ifdef __cplusplus
    PR_END_EXTERN_C
#endif

#endif
