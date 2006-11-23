/*------------------------------------------------------------------------*/
/**
 * @file	NewtGC.h
 * @brief   ガベージコレクション
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTGC_H
#define	NEWTGC_H

/* ヘッダファイル */
#include "NewtMem.h"


/* マクロ */
#define NewtGCHint(r, hint)									///< GC を効率良く行うためのヒントを与える


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


void		NewtCheckGC(newtPool pool, size_t size);
newtObjRef	NewtObjChainAlloc(newtPool pool, size_t size, size_t dataSize);
void		NewtPoolRelease(newtPool pool);

void		NewtGC(void);

newtRef		NsGC(newtRefArg rcvr);


#ifdef __cplusplus
}
#endif


#endif /* NEWTGC_H */
