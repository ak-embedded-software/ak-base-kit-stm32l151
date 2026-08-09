/******************************************************************************
  Target Script for Cortex-M3

  Copyright (c) 2008 Rowley Associates Limited.

  This file may be distributed under the terms of the License Agreement
  provided with this software.

  THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND, INCLUDING THE
  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ******************************************************************************/

function Reset()
{
  TargetInterface.stopAndReset(400);
}

function SRAMReset()
{
  Reset();
}

function FLASHReset()
{
  Reset();
}


