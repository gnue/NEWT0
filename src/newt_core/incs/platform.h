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


/* ヘッダファイル */
#include "config.h"


// 未定義の場合はデフォルト値を設定する
#ifndef	__PLATFORM__
	#define __PLATFORM__			NULL
#endif

#ifndef	__DYLIBSUFFIX__
	#define __DYLIBSUFFIX__		".so"
#endif

#ifndef NEWT_EXPORT
	#define NEWT_EXPORT
#endif


#endif /* PLATFORM_H */
