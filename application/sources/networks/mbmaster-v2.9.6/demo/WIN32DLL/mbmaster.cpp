// mbmasterdll.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "mbport.h"

extern          "C"
{
    extern void     vMBPOtherDLLInit( void );
    extern void     vMBPOtherDLLClose( void );
}

#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL            APIENTRY
DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
{
    switch ( ul_reason_for_call )
    {
    case DLL_PROCESS_ATTACH:
        /* This is a dynamic load with LoadLibrary. */
        vMBPOtherDLLInit(  );
        break;
    case DLL_PROCESS_DETACH:
        vMBPOtherDLLClose(  );
        break;
    }
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif
