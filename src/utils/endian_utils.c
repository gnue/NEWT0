/*------------------------------------------------------------------------*/
/**
 * @file	endian_utils.c
 * @brief   エンディアン・ユーティリティ
 *
 * @author  M.Nukui
 * @date	2005-06-04
 *
 * Copyright (C) 2005 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <string.h>
#include "utils/endian_utils.h"

#if _MSC_VER==1200
# define ULL(a) a
#endif

#ifndef ULL
# define ULL(a) a##ULL
#endif


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 浮動小数点数のバイトオーダ（エンディアン）を変換する
 *
 * @param d			[in] 浮動小数点数
 *
 * @return			バイトオーダを変換した浮動小数点数
 */

double swapd(double d)
{
	uint64_t	tmp = 0;

    memcpy(&tmp, &d, sizeof(d));
	tmp = (tmp >> 32) | (tmp << 32);
	tmp = ((tmp & ULL(0xff00ff00ff00ff00)) >> 8) | ((tmp & ULL(0x00ff00ff00ff00ff)) << 8);
	tmp = ((tmp & ULL(0xffff0000ffff0000)) >> 16) | ((tmp & ULL(0x0000ffff0000ffff)) << 16);
    memcpy(&d, &tmp, sizeof(d));

	return d;
}
