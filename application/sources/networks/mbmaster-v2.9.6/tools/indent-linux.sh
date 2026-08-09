#!/bin/sh
#
# Source Code indenting settings
#
# ModbusSlave Libary: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
# Copyright (c) 2007-2014 Christian Walter <cwalter@embedded-solutions.at>
# All rights reserved.
#

indent \
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
