/*------------------------------------------------------------------------*/
/**
 * @file	NewtMem.h
 * @brief   メモリ管理
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTMEM_H
#define	NEWTMEM_H


/* ヘッダファイル */
#include "NewtType.h"


/* 型宣言 */

/// メモリプール
typedef struct {
    void *		pool;			///< 実メモリプールへのポインタ

    int32_t		usesize;		///< 使用サイズ
    int32_t		maxspace;		///< 現在の最大サイズ
    int32_t		expandspace;	///< 一度に拡張できるメモリサイズ

    newtObjRef	obj;			///< 確保したオブジェクトへのチェイン（GC の対象）
    newtObjRef	literal;		///< 確保したリテラルへのチェイン
} newtpool_t;

typedef newtpool_t *	newtPool;   ///< メモリプールへのポインタ


/// スタック
typedef struct {
    newtPool	pool;		///< メモリプール

    void *		stackp;		///< スタック（配列メモリ）へのポインタ
    uint32_t	sp;			///< スタックポインタ

    uint32_t	datasize;	///< データサイズ
    uint32_t	nums;		///< メモリ確保されているデータ数
    uint32_t	blocksize;	///< メモリを一括確保するデータ数
} newtStack;


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


newtPool	NewtPoolAlloc(int32_t expandspace);

void *		NewtMemAlloc(newtPool pool, size_t size);
void *		NewtMemCalloc(newtPool pool, size_t number, size_t size);
void *		NewtMemRealloc(newtPool pool, void * ptr, size_t size);
void		NewtMemFree(void * ptr);

void		NewtStackSetup(newtStack * stackinfo,
                    newtPool pool, uint32_t datasize, uint32_t blocksize);

bool		NewtStackExpand(newtStack * stackinfo, uint32_t n);
void		NewtStackSlim(newtStack * stackinfo, uint32_t n);
void		NewtStackFree(newtStack * stackinfo);

uint32_t	NewtAlign(uint32_t n, uint16_t byte);


#ifdef __cplusplus
}
#endif


#endif /* NEWTMEM_H */

