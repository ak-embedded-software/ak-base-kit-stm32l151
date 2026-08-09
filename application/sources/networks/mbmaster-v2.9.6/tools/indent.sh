#!/bin/sh
#
# Source Code indenting settings
#
# ModbusMaster Libary: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
# Copyright (c) 2007 Christian Walter <wolti@sil.at>
# All rights reserved.
#

#MYDIR=`dirname $0`
MYDIR="/cygdrive/c/GnuWin32/bin/"

$MYDIR/indent.exe --version
$MYDIR/indent.exe \
    --declaration-indentation16 \
    --procnames-start-lines \
    --blank-lines-after-procedures \
    --break-before-boolean-operator \
    --braces-after-if-line \
    --braces-after-struct-decl-line \
    --brace-indent0 \
    --case-indentation0 \
    --no-space-after-function-call-names \
    --no-space-after-for \
    --no-space-after-if \
    --no-space-after-while \
    --no-space-after-casts \
    --space-after-parentheses \
    --dont-format-comments \
    --indent-level4 \
    --honour-newlines \
    --no-tabs \
    --line-length160 \
    $@
   
unix2dos $@
