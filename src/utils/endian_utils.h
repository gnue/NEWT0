/*------------------------------------------------------------------------*/
/**
 * @file	endian_utils.h
 * @brief   エンディアン・ユーティリティ
 *
 * @author  M.Nukui
 * @date	2005-06-04
 *
 * Copyright (C) 2005 M.Nukui All rights reserved.
 */


#ifndef	ENDIAN_UTILS_H
#define	ENDIAN_UTILS_H


/* ヘッダファイル */
#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif


#ifdef HAVE_STDINT_H
	#include <stdint.h>
#else
	#include <inttypes.h>
#endif


#if defined(HAVE_ENDIAN_H)
	#include <endian.h>
#elif defined(HAVE_MACHINE_ENDIAN_H)
	#include <machine/endian.h>
#else
	#ifdef ntohs
	#undef ntohs
	#endif

	#ifdef htons
	#undef htons
	#endif

	#ifdef ntohl
	#undef ntohl
	#endif

	#ifdef htonl
	#undef htonl
	#endif
#endif


/* マクロ定義 */
#ifdef __BIG_ENDIAN__
	#define ntohd(d)	(d)
	#define htond(d)	(d)
#else
	#define ntohd(d)	swapd(d)
	#define htond(d)	swapd(d)
#endif /* __BIG_ENDIAN__ */


#ifndef ntohl
	#define	swap16(n)	(((((uint16_t) n) << 8) & 0xff00) | \
						 ((((uint16_t) n) >> 8) & 0x00ff))

	#define	swap32(n)	(((((uint32_t) n) << 24) & 0xff000000) | \
						 ((((uint32_t) n) <<  8) & 0x00ff0000) | \
						 ((((uint32_t) n) >>  8) & 0x0000ff00) | \
						 ((((uint32_t) n) >> 24) & 0x000000ff))

	#ifdef __BIG_ENDIAN__
		#define ntohs(n)	(n)
		#define htons(n)	(n)
		#define ntohl(n)	(n)
		#define htonl(n)	(n)
	#else
		#define ntohs(n)	swap16(n)
		#define htons(n)	swap16(n)
		#define ntohl(n)	swap32(n)
		#define htonl(n)	swap32(n)
	#endif /* __BIG_ENDIAN__ */
#endif


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


double		swapd(double d);


#ifdef __cplusplus
}
#endif


#endif /* ENDIAN_UTILS_H */
