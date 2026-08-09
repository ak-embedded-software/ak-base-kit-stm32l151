/************************************************************************************
 * Copyright @ 1995-2005 Freescale Semiconductor, Inc. All rights reserved          *
 *                                                                                  *
 *                                                                                  *
 *                                                                                  *
 * DESCRIPTION                                                                      *
 *   Define constants used by CodeWarrior Preprocessor.                             *
 *                                                                                  *
 *                                                                                  *
 *                                                                                  *
 * NOTE                                                                             *
 *   Use this as a prefix file for the CodeWarrior assembler and compiler for ram   *
 *   target.                                                                        *
 *                                                                                  *
 * HISTORY                                                                          *
 *                                                                                  *	
 ************************************************************************************/
 
/* Define the board we are running on */
#define	M5249C3 /* HSEVB */

/* Defining DEBUG turns on debug print information */
#define DEBUG	1

/* CodeWarrior looks for an underscore prepended to C function names */
#define _UNDERSCORE_

/* Define a constant to inform files we are using CodeWarrior */
#ifndef _MWERKS_
#define _MWERKS_
#endif

/* Modify the interrupt type to work with CodeWarrior */
#define __interrupt__	__declspec(interrupt)
