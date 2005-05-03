/*------------------------------------------------------------------------*/
/**
 * @file	platform.h
 * @brief   プラットフォーム関連のマクロ定義
 *
 * @author  M.Nukui
 * @date	2005-03-17
 *
 * Copyright (C) 2005 M.Nukui All rights reserved.
 */


#ifndef	PLATFORM_H
#define	PLATFORM_H


#ifdef __DARWIN__
	#define __PLATFORM__		"Darwin"
	#define __DYLIBSUFFIX__		".dylib"
	#define	HAVE_STDINT_H		1
#endif

#ifdef __linux__
	#define __PLATFORM__		"Linux"
	#define __DYLIBSUFFIX__		".so"
	#define	HAVE_STDINT_H		1
#endif

#ifdef __FREEBSD__
	#define __PLATFORM__		"FreeBSD"
	#define __DYLIBSUFFIX__		".so"
#endif

#ifdef __MINGW32__
	#define __PLATFORM__		"WIN32"
	#define __DYLIBSUFFIX__		".dll"
	#define	HAVE_STDINT_H		1
#endif

#ifdef __BEOS__
	#define __PLATFORM__		"BeOS"
	#define __DYLIBSUFFIX__		".so"
#endif


#ifndef HAVE_STDINT_H
	#define	HAVE_STDINT_H		0
#endif


// 未定義の場合はデフォルト値を設定する
#ifndef	__PLATFORM__
	#define __PLATFORM__		NULL
#endif

#ifndef	__DYLIBSUFFIX__
	#define __DYLIBSUFFIX__		".so"
#endif


#endif /* PLATFORM_H */
