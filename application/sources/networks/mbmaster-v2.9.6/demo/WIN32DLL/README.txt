
                         MODBUS MASTER DLL wIN32
                         =======================

Debugging:
==========

The MODBUS master wIN32 dll can be compiled with runtime debugging support.
For this the compile time flag MBM_ENABLE_DEBUG_FACILITY must be set to 1.
Debugging can then be customized by the following environment variables.

MODBUS_LOGMODULES:

Set the environemt variable MODBUS_LOGMODULES to 0xFFFFto enable all
modules. For a combination of the modules simply or the values of the
modules shown below:

    MB_LOG_CORE         = 0x0001,
    MB_LOG_RTU          = 0x0002,
    MB_LOG_ASCII        = 0x0004,
    MB_LOG_TCP          = 0x0008,
    MB_LOG_PORT_EVENT   = 0x0010,
    MB_LOG_PORT_TIMER   = 0x0020,
    MB_LOG_PORT_SERIAL  = 0x0040,
    MB_LOG_PORT_TCP     = 0x0080,
    MB_LOG_PORT_OTHER   = 0x0100,

MODBUS_LOGLEVELS:

A log message will be logged if the level of the message is smaller than
the value of MODBUS_LOGLEVELS. To disable logging simply set MODBUS_LOGLEVELS
to 0. 

    MB_LOG_ERROR = 0
    MB_LOG_WARN  = 1
    MB_LOG_INFO  = 2
    MB_LOG_DEBUG = 3

MODBUS_LOGFILE:

Filename where the debug log should be written. The file is always appended
and never removed.