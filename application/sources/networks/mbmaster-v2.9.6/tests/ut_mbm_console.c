/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_console.c,v 1.2 2007-08-17 23:34:01 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Console.h"

/* ----------------------- Platform includes --------------------------------*/

/* ----------------------- Modbus includes ----------------------------------*/

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Functions prototypes -----------------------------*/
int             iMBM_AddTests( void );

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

int
main( int argc, char *argv[] )
{
    CU_BOOL         Run = CU_FALSE;

    ( void )setvbuf( stdout, NULL, ( int )_IONBF, 0 );

    if( argc > 1 )
    {
        if( 0 == strcmp( "-i", argv[1] ) )
        {
            Run = CU_TRUE;
            CU_set_error_action( CUEA_IGNORE );
        }
        else if( 0 == strcmp( "-f", argv[1] ) )
        {
            Run = CU_TRUE;
            CU_set_error_action( CUEA_FAIL );
        }
        else if( 0 == strcmp( "-A", argv[1] ) )
        {
            Run = CU_TRUE;
            CU_set_error_action( CUEA_ABORT );
        }
        else
        {
            printf( "\nUsage:  ConsoleTest [option]\n\n"
                    "Options:   -i   Run, ignoring framework errors [default].\n"
                    "           -f   Run, failing on framework error.\n"
                    "           -A   run, aborting on framework error.\n" "           -h   Print this message.\n\n" );
        }
    }
    else
    {
        Run = CU_TRUE;
        CU_set_error_action( CUEA_IGNORE );
    }

    if( CU_TRUE == Run )
    {
        if( 0 != CU_initialize_registry(  ) )
        {
            printf( "\nInitialization of Test Registry failed." );
        }
        else
        {
            if( iMBM_AddTests(  ) < 0 )
            {
                CU_cleanup_registry(  );
                return CU_get_error(  );
            }
            else
            {
                CU_console_run_tests(  );
                CU_cleanup_registry(  );
            }
        }
    }

    return 0;
}
