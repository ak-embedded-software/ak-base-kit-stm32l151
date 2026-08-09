void
MBMExReadHoldingBlockingAPI( void )
{
    eMBMErrorCode   eStatus;
    xMBMHandle      xMBMMasterHdl1;
    USHORT          usNRegs[5];

    /* Initialize MODBUS stack here. */
    ...eStatus = eMBMReadHoldingRegisters( xMBMMasterHdl1, 1, 10, 5, usNRegs );
    if( MBM_ENOERR == eStatus )
    {
        /* You can safely access now usNRegs. */
    }
}
