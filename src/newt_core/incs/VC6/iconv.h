 /*------------------------------------------------------------------------*/
/**
 * @file	unistd.h
 * @brief   replacing the missing inttypes.h from the Unix environment
 *
 * @author  M. Melcher
 * @date	2007-03-22
 *
 * Copyright (C) 2007 Matthias Melcher, All Rights Reserved.
 */

#ifndef _NEWT_VC6_ICONV_H_
#define _NEWT_VC6_ICONV_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef __int32 iconv_t;

iconv_t iconv_open(const char *, const char *);
size_t  iconv(iconv_t, const char **, size_t *, char **, size_t *);
int     iconv_close(iconv_t);

#ifdef __cplusplus
}
#endif

#endif

