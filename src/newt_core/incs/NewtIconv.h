/*------------------------------------------------------------------------*/
/**
 * @file	NewtIconv.h
 * @brief   文字コード処理（libiconv使用）
 *
 * @author  M.Nukui
 * @date	2005-07-17
 *
 * Copyright (C) 2005 M.Nukui All rights reserved.
 */


#ifndef	NEWTICONV_H
#define	NEWTICONV_H


/* ヘッダファイル */
#include "NewtType.h"


#ifdef HAVE_LIBICONV
#include <iconv.h>


/* マクロ */


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


char *		NewtIconv(iconv_t cd, char* src, size_t srclen, size_t* dstlenp);


#ifdef __cplusplus
}
#endif


#endif /* HAVE_LIBICONV */
#endif /* NEWTICONV_H */
