/*------------------------------------------------------------------------*/
/**
 * @file	Objects.h
 * @brief   基本オブジェクトインタフェース
 *			（Newton C++ Tools / Newton.framework 互換用）
 *
 * @author  M.Nukui
 * @date	2005-03-10
 *
 * Copyright (C) 2005 M.Nukui All rights reserved.
 */

#ifndef __OBJECTS_H
#define __OBJECTS_H


/* ヘッダファイル */
#include "Newton/Newton.h"


/* 型定義 */
typedef newtRef					Ref;
typedef newtRefVar				RefVar;
typedef newtRefArg				RefArg;


/* マクロ */
#define	MAKEINT(i)				NewtMakeInteger(i)
#define	MAKECHAR(c)				NewtMakeCharacter(c)
#define	MAKEBOOLEAN(b)			NewtMakeBoolean(b)
#define	MAKEPTR(p)				NewtMakePointer(p)
#define	MAKEMAGICPTR(index)		NewtMakeMagicPointer(index)

#define	NILREF					kNewtRefNIL
#define	TRUEREF					kNewtRefTRUE
#define	FALSEREF				NILREF
#define	INVALIDPTRREF			NewtMakeInt30(0)

// 残りは未実装、定義すること


#endif	/* __OBJECTS_H */
