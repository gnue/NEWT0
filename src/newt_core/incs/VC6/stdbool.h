 /*------------------------------------------------------------------------*/
/**
 * @file	stdbool.h
 * @brief   replacing the missing inttypes.h from the Unix environment
 *
 * @author  M. Melcher
 * @date	2007-03-22
 *
 * Copyright (C) 2007 Matthias Melcher, All Rights Reserved.
 */

#ifndef _NEWT_VC6_STD_BOOL_H_
#define _NEWT_VC6_STD_BOOL_H_

#ifndef __cplusplus

typedef __int8 bool;
static const bool true = 1;
static const bool false = 0;

#endif

#endif

