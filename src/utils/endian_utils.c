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


#pragma mark -
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
	tmp = ((tmp & 0xff00ff00ff00ff00ULL) >> 8) | ((tmp & 0x00ff00ff00ff00ffULL) << 8);
	tmp = ((tmp & 0xffff0000ffff0000ULL) >> 16) | ((tmp & 0x0000ffff0000ffffULL) << 16);
    memcpy(&d, &tmp, sizeof(d));

	return d;
}
