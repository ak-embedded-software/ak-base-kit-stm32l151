#!/bin/sh
MBMASTER_ROOT="../.."
MBMASTER_FILES="mbmaster/ascii/mbmascii.c
mbmaster/mbm.c
mbmaster/functions/mbmfuncinput.c
mbmaster/functions/mbmfuncdisc.c
mbmaster/functions/mbmfunccoils.c
mbmaster/functions/mbmfuncraw.c
mbmaster/functions/mbmfuncholding.c
mbmaster/functions/mbmfunccustom1.c
mbmaster/common/mbutils.c
mbmaster/rtu/mbmrtu.c
mbmaster/rtu/mbmcrc.c
mbmaster/tcp/mbmtcp.c
mbmaster/ascii/mbmascii.h
mbmaster/include/common/mbframe.h
mbmaster/include/common/mbportlayer.h
mbmaster/include/common/mbtypes.h
mbmaster/include/common/mbutils.h
mbmaster/include/mbm.h
mbmaster/include/internal/mbmiconfig.h
mbmaster/include/internal/mbmiframe.h
mbmaster/include/internal/mbmi.h
mbmaster/rtu/mbmrtu.h
mbmaster/rtu/mbmcrc.h
mbmaster/tcp/mbmtcp.h"

for myfile in $MBMASTER_FILES; do
  src=$MBMASTER_ROOT/$myfile
  dir=`dirname ${myfile}`
  mkdir -p ${dir} >/dev/null 2>&1 
  cp -rp $MBMASTER_ROOT/$myfile $myfile 
done

