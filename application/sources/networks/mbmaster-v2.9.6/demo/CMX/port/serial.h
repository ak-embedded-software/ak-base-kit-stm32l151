/* 
 * MODBUS Library: Low level serial functions
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: serial.h,v 1.1 2008-08-21 13:34:10 cwalter Exp $
 */

#ifndef _SERIAL_H
#define _SERIAL_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif
/* ----------------------- Defines ------------------------------------------*/
/* ----------------------- Type definitions ---------------------------------*/
/* ----------------------- Function prototypes ------------------------------*/
void            vMBPSerialUART0Init( ULONG ulBaudRate, UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits );
USHORT          vMBPSerialUART0Receive( UBYTE * pubBuffer, USHORT usLengthMax );
USHORT          vMBPSerialUART0SendQueueCnt(  );
BOOL            bMBPSerialUART0Send( UBYTE * pubBuffer, USHORT usLength );

#endif
