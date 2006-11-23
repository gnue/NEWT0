//--------------------------------------------------------------------------
/**
 * @file  NewtLib.h
 * @brief 拡張ライブラリ
 *
 * @author M.Nukui
 * @date 2004-06-10
 *
 * Copyright (C) 2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTLIB_H
#define	NEWTLIB_H


/* ヘッダファイル */
#include "NewtType.h"


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


void	newt_install(void);		///< 拡張ライブラリのエントリポイント（インストール）


#ifdef __cplusplus
}
#endif


#endif /* NEWTLIB_H */
