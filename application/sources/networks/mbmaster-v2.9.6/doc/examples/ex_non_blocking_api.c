void
MBMExReadHoldingNonBlockingAPI( void )
{
    eMBMErrorCode   eStatus1, eStatus2;
    eMBMQueryState  eState1;
    eMBMQueryState  eState2;
    xMBMHandle      xMBMMasterHdl1;
    xMBMHandle      xMBMMasterHdl2;
    USHORT          usNRegs1[5], usNRegs2[2];

    /* Initialize MODBUS stacks here. */
    ...
        /* Don't forget to initialize the state variables. */
        eState1 = eState2 = MBM_STATE_NONE;
    do
    {
        if( MBM_STATE_NONE != eState1 )
        {
            vMBMReadHoldingRegistersPolled( xMBMMasterHdl1, 1, 10, 5, usNRegs1, &eState1, &eStatus1 );
        }
        if( MBM_STATE_NONE != eState2 )
        {
            vMBMReadHoldingRegistersPolled( xMBMMasterHdl2, 23, 7, 2, usNRegs2, &eState2, &eStatus2 );
        }
    }
    while( ( eState1 != MBM_STATE_DONE ) || ( eState2 != MBM_STATE_DONE ) );

    if( MBM_ENOERR == eStatus1 )
    {
        /* You can safely access now usNRegs1. */
    }
    if( MBM_ENOERR == eStatus2 )
    {
        /* You can safely access now usNRegs2. */
    }
}
