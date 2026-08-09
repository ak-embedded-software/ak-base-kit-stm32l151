//*****************************************************************************
//
// sine.c - Fixed point sine trigonometric function.
//
// Copyright (c) 2006-2010 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 5727 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include "utils/sine.h"

//*****************************************************************************
//
//! \addtogroup sine_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// A table of the value of the sine function for the first ninety degrees with
// 128 entries (that is, [0] = 0 degrees, [127] = 90 degrees).  Each entry is
// in 0.16 fixed point notation.
//
//*****************************************************************************
static const unsigned short g_pusFixedSineTable[] =
{
    0x0000, 0x032B, 0x0655, 0x097F, 0x0CA9, 0x0FD2, 0x12FB, 0x1623, 0x194A,
    0x1C70, 0x1F95, 0x22B9, 0x25DB, 0x28FC, 0x2C1B, 0x2F39, 0x3255, 0x356E,
    0x3886, 0x3B9B, 0x3EAF, 0x41BF, 0x44CD, 0x47D9, 0x4AE1, 0x4DE7, 0x50E9,
    0x53E9, 0x56E5, 0x59DE, 0x5CD3, 0x5FC4, 0x62B2, 0x659C, 0x6882, 0x6B64,
    0x6E42, 0x711B, 0x73F0, 0x76C0, 0x798C, 0x7C53, 0x7F15, 0x81D2, 0x848A,
    0x873D, 0x89EB, 0x8C93, 0x8F36, 0x91D3, 0x946A, 0x96FC, 0x9987, 0x9C0D,
    0x9E8C, 0xA106, 0xA378, 0xA5E5, 0xA84B, 0xAAAB, 0xAD03, 0xAF55, 0xB1A1,
    0xB3E5, 0xB622, 0xB858, 0xBA87, 0xBCAE, 0xBECE, 0xC0E7, 0xC2F8, 0xC502,
    0xC703, 0xC8FD, 0xCAEF, 0xCCD9, 0xCEBB, 0xD095, 0xD267, 0xD431, 0xD5F2,
    0xD7AB, 0xD95C, 0xDB03, 0xDCA3, 0xDE3A, 0xDFC8, 0xE14D, 0xE2C9, 0xE43D,
    0xE5A7, 0xE709, 0xE862, 0xE9B1, 0xEAF7, 0xEC35, 0xED68, 0xEE93, 0xEFB4,
    0xF0CC, 0xF1DB, 0xF2DF, 0xF3DB, 0xF4CD, 0xF5B5, 0xF694, 0xF769, 0xF834,
    0xF8F6, 0xF9AE, 0xFA5C, 0xFB00, 0xFB9A, 0xFC2B, 0xFCB2, 0xFD2E, 0xFDA1,
    0xFE0A, 0xFE69, 0xFEBE, 0xFF0A, 0xFF4B, 0xFF82, 0xFFAF, 0xFFD2, 0xFFEB,
    0xFFFA, 0xFFFF
};

//*****************************************************************************
//
//! Computes an approximation of the sine of the input angle.
//!
//! \param ulAngle is an angle expressed as a 0.32 fixed-point value that is
//! the percentage of the way around a circle.
//!
//! This function computes the sine for the given input angle.  The angle is
//! specified in 0.32 fixed point format, and is therefore always between 0 and
//! 360 degrees, inclusive of 0 and exclusive of 360.
//!
//! \return Returns the sine of the angle, in 16.16 fixed point format.
//
//*****************************************************************************
long
sine(unsigned long ulAngle)
{
    unsigned long ulIdx;

    //
    // Add 0.5 to the angle.  Since only the upper 9 bits are used to compute
    // the sine value, adding one to the tenth bit is 0.5 from the point of
    // view of the sine table.
    //
    ulAngle += 0x00400000;

    //
    // Get the index into the sine table from bits 29:23.
    //
    ulIdx = (ulAngle >> 23) & 127;

    //
    // If bit 30 is set, the angle is between 90 and 180 or 270 and 360.  In
    // these cases, the sine value is decreasing from one instead of increasing
    // from zero.  The indexing into the table needs to be reversed.
    //
    if(ulAngle & 0x40000000)
    {
        ulIdx = 127 - ulIdx;
    }

    //
    // Get the value of the sine.
    //
    ulIdx = g_pusFixedSineTable[ulIdx];

    //
    // If bit 31 is set, the angle is between 180 and 360.  In this case, the
    // sine value is negative; otherwise it is positive.
    //
    if(ulAngle & 0x80000000)
    {
        return(0 - ulIdx);
    }
    else
    {
        return(ulIdx);
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
