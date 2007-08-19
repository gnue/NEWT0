/*------------------------------------------------------------------------*/
/**
 * @file	NewtMem.c
 * @brief   メモリ管理
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <string.h>
#include "NewtMem.h"


/*------------------------------------------------------------------------*/
/** メモリプールの確保
 *
 * @param expandspace   [in] ブロックの拡張サイズ
 *
 * @return				メモリプール
 */

newtPool NewtPoolAlloc(int32_t expandspace)
{
    newtPool	pool;

    pool = (newtPool)calloc(1, sizeof(newtpool_t));

    if (pool != NULL)
    {
        pool->maxspace = pool->expandspace = expandspace;
    }

    return pool;
}


/*------------------------------------------------------------------------*/
/** メモリプールから指定されたサイズのメモリを確保する
 *
 * @param pool		[in] メモリプール
 * @param size		[in] データサイズ
 *
 * @return			確保したメモリへのポインタ
 *
 * @note			現在はまだ独自メモリ管理は行われていない
 */

void * NewtMemAlloc(newtPool pool, size_t size)
{
    if (pool != NULL)
    {
    }

    return malloc(size);
}


/*------------------------------------------------------------------------*/
/** メモリプールから指定されたサイズのメモリを確保する
 *
 * @param pool		[in] メモリプール
 * @param number	[in] データ数
 * @param size		[in] データサイズ
 *
 * @return			確保したメモリへのポインタ
 *
 * @note			現在はまだ独自メモリ管理は行われていない
 */

void * NewtMemCalloc(newtPool pool, size_t number, size_t size)
{
    if (pool != NULL)
    {
    }

    return calloc(number, size);
}


/*------------------------------------------------------------------------*/
/** メモリプールから指定されたサイズのメモリを再確保する
 *
 * @param pool		[in] メモリプール
 * @param ptr		[in] メモリへのポインタ
 * @param size		[in] データサイズ
 *
 * @return			再確保したメモリへのポインタ
 *
 * @note			現在はまだ独自メモリ管理は行われていない
 */

void * NewtMemRealloc(newtPool pool, void * ptr, size_t size)
{
    if (pool != NULL)
    {
    }

    return realloc(ptr, size);
}


/*------------------------------------------------------------------------*/
/** メモリを解放する
 *
 * @param ptr		[in] メモリへのポインタ
 *
 * @return			なし
 *
 * @note			現在はまだ独自メモリ管理は行われていない
 */

void NewtMemFree(void * ptr)
{
    free(ptr);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** スタック情報をセットアップ
 *
 * @param stackinfo	[out]スタック情報
 * @param pool		[in] メモリプール
 * @param datasize	[in] データサイズ
 * @param blocksize	[in] ブロックサイズ
 *
 * @return			なし
 */

void NewtStackSetup(newtStack * stackinfo,
        newtPool pool, uint32_t datasize, uint32_t blocksize)
{
    memset(stackinfo, 0, sizeof(newtStack));

    stackinfo->pool = pool;
    stackinfo->datasize = datasize;
    stackinfo->blocksize = blocksize;
}


/*------------------------------------------------------------------------*/
/** スタックを解放
 *
 * @param stackinfo	[in] スタック情報
 *
 * @return			なし
 */

void NewtStackFree(newtStack * stackinfo)
{
    if (stackinfo->stackp != NULL)
    {
        NewtMemFree(stackinfo->stackp);
        stackinfo->stackp = NULL;
    }

    stackinfo->nums = 0;
    stackinfo->sp = 0;
}


/*------------------------------------------------------------------------*/
/** スタックを拡張
 *
 * @param stackinfo	[in] スタック情報
 * @param n			[in] 必要とされているスタック長
 *
 * @retval			true	必要なだけ確保されている
 * @retval			false   確保できなかった
 */

bool NewtStackExpand(newtStack * stackinfo, uint32_t n)
{
    if (stackinfo->nums < n)
    {
        uint32_t	newsize;
        uint32_t	nums;
        void *		newp;
    
        nums = stackinfo->nums + stackinfo->blocksize;
        newsize = stackinfo->datasize * nums;

        newp = NewtMemRealloc(stackinfo->pool, stackinfo->stackp, newsize);

        if (newp == NULL) return false;
    
        stackinfo->stackp = newp;
        stackinfo->nums = nums;
    }

    return true;
}


/*------------------------------------------------------------------------*/
/** スタックをスリム化
 *
 * @param stackinfo	[in] スタック情報
 * @param n			[in] 必要とされているスタック長
 *
 * @return			なし
 */

void NewtStackSlim(newtStack * stackinfo, uint32_t n)
{
    if (0 < n)
    {
        uint32_t	newsize;
		void *		newp;

        newsize = stackinfo->datasize * n;
		newp = NewtMemRealloc(stackinfo->pool, stackinfo->stackp, newsize);

		if (newp != NULL)
		{
			stackinfo->stackp = newp;
			stackinfo->nums = n;
		}
    }
    else
    {
        NewtStackFree(stackinfo);
    }
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** アラインを計算
 *
 * @param n			[in] アラインする値
 * @param byte		[in] アラインされる単位
 *
 * @return			アラインされた値
 */

uint32_t NewtAlign(uint32_t n, uint16_t byte)
{
    uint32_t	mod;

    mod = n % byte;
    if (0 < mod) n += byte - mod;

    return n;
}
