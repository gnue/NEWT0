/*------------------------------------------------------------------------*/
/**
 * @file	NewtGC.c
 * @brief   ガベージコレクション
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#ifdef HAVE_MEMORY_H
	#include <memory.h>
#else
	#include <string.h>
#endif


#include "NewtGC.h"
#include "NewtObj.h"
#include "NewtMem.h"
#include "NewtEnv.h"
#include "NewtVM.h"
#include "NewtIO.h"
#include "NewtPrint.h"


/* 関数プロトタイプ */
static void		NewtPoolSnap(const char * title, newtPool pool, int32_t usesize);

static void		NewtObjChain(newtObjRef * objp, newtObjRef obj);
static void		NewtPoolChain(newtPool pool, newtObjRef obj, bool literal);
static void		NewtObjFree(newtPool pool, newtObjRef obj);
static void		NewtObjChainFree(newtPool pool, newtObjRef * objp);

#if 0
static void		NewtPoolMarkClean(newtPool pool);
#endif

static void		NewtPoolSweep(newtPool pool, bool mark);

static void		NewtGCRefMark(newtRefArg r, bool mark);
static void		NewtGCRegMark(vm_reg_t * reg, bool mark);
static void		NewtGCStackMark(vm_env_t * env, bool mark);
static void		NewtGCMark(vm_env_t * env, bool mark);


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** メモリプールのスナップショットを出力する
 *
 * @param title		[in] タイトル
 * @param pool		[in] メモリプール
 * @param usesize	[in] GC前の使用サイズ
 *
 * @return			なし
 */

void NewtPoolSnap(const char * title, newtPool pool, int32_t usesize)
{
    NewtDebugMsg(title, "mem = %d(%d)\n", pool->usesize, pool->usesize - usesize);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** オブジェクトデータをチェインする
 *
 * @param objp		[in] チェインされるオブジェクトデータへのポインタ
 * @param obj		[in] チェインするオブジェクトデータ
 *
 * @return			なし
 *
 * @note			objp の参照先が NULL の場合は obj をセットして返す
 */

void NewtObjChain(newtObjRef * objp, newtObjRef obj)
{
    if (*objp == NULL)
    {
        *objp = obj;
    }
    else
    {
        obj->header.nextp = *objp;
        *objp = obj;
    }
}


/*------------------------------------------------------------------------*/
/** メモリプール内でオブジェクトデータをチェインする
 *
 * @param pool		[in] メモリプール
 * @param obj		[in] チェインするオブジェクトデータ
 * @param literal	[in] リテラルかどうか
 *
 * @return			なし
 */

void NewtPoolChain(newtPool pool, newtObjRef obj, bool literal)
{
    if (obj != NULL && pool != NULL)
    {
        if (literal)
            NewtObjChain(&pool->literal, obj);
        else
            NewtObjChain(&pool->obj, obj);
    }
}


/*------------------------------------------------------------------------*/
/** GCが必要かチェックする
 *
 * @param pool		[in] メモリプール
 * @param size		[in] 追加サイズ
 *
 * @return			なし
 */

void NewtCheckGC(newtPool pool, size_t size)
{
    if (pool->maxspace < pool->usesize + size)
    {
        NEWT_NEEDGC = true;
    }
}


/*------------------------------------------------------------------------*/
/** メモリプール内でオブジェクトメモリを確保してチェインする
 *
 * @param pool		[in] メモリプール
 * @param size		[in] オブジェクトサイズ
 * @param dataSize	[in] データサイズ
 *
 * @return			オブジェクトデータ
 */

newtObjRef NewtObjChainAlloc(newtPool pool, size_t size, size_t dataSize)
{
    newtObjRef	obj;

    NewtCheckGC(pool, size + dataSize);

    obj = (newtObjRef)NewtMemAlloc(pool, size);
    if (obj == NULL) return NULL;

	memset(&obj->header, 0, sizeof(obj->header));

    if (0 < dataSize)
    {
        uint8_t *	data;
    
        data = NewtMemAlloc(pool, dataSize);
    
        if (data == NULL)
        {
            NewtMemFree(obj);
            return NULL;
        }

        *((uint8_t **)(obj + 1)) = data;
    }
    else
    {
        obj->header.h = kNewtObjLiteral;
    }

    if (pool != NULL)
    {
        NewtPoolChain(pool, obj, dataSize == 0);
        pool->usesize += size + dataSize;
    }

    return obj;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** オブジェクトを解放する
 *
 * @param pool		[in] メモリプール
 * @param obj		[in] オブジェクト
 *
 * @return			なし
 */

void NewtObjFree(newtPool pool, newtObjRef obj)
{
    uint32_t	datasize;

    if (NewtObjIsLiteral(obj))
    {
        datasize = NewtAlign(sizeof(newtObj) + NewtObjSize(obj), 4);
    }
    else
    {
        datasize = sizeof(newtObj) + sizeof(uint8_t *);
        datasize += NewtObjCalcDataSize(NewtObjSize(obj));

        NewtMemFree(NewtObjData(obj));
    }

    NewtMemFree(obj);

    if (pool != NULL)
        pool->usesize -= datasize;
}


/*------------------------------------------------------------------------*/
/** オブジェクトデータにチェインされている全てのオブジェクトデータを解放する
 *
 * @param pool		[in] メモリプール
 * @param objp		[i/o]オブジェクトデータへのポインタ
 *
 * @return			なし
 */

void NewtObjChainFree(newtPool pool, newtObjRef * objp)
{
    newtObjRef	nextp;
    newtObjRef	obj;

    for (obj = *objp; obj != NULL; obj = nextp)
    {
        nextp = obj->header.nextp;
        NewtObjFree(pool, obj);
    }

    *objp = NULL;
}


/*------------------------------------------------------------------------*/
/** メモリプールの解放
 *
 * @param pool		[in] メモリプール
 *
 * @return			なし
 */

void NewtPoolRelease(newtPool pool)
{
    if (pool != NULL)
    {
        int32_t		usesize;

        usesize = pool->usesize;

        NewtObjChainFree(pool, &pool->obj);
        NewtObjChainFree(pool, &pool->literal);

        if (NEWT_DEBUG)
            NewtPoolSnap("RELEASE", pool, usesize);
    }
}


#if 0
#pragma mark -
#endif

#if 0
/*------------------------------------------------------------------------*/
/** オブジェクトのマークをクリアする
 *
 * @param pool		[in] メモリプール
 *
 * @return			なし
 */

void NewtPoolMarkClean(newtPool pool)
{
    if (pool != NULL)
    {
        newtObjRef	nextp;
        newtObjRef	obj;
        newtObjRef *	prevp = &pool->obj;
        int32_t		usesize;

        usesize = pool->usesize;

        for (obj = pool->obj; obj != NULL; obj = nextp)
        {
            nextp = obj->header.nextp;

            if (NewtObjIsLiteral(obj))
            {
                NewtObjChain(&pool->literal, obj);
                *prevp = nextp;

                continue;
            }

            obj->header.h &= ~ (uint32_t)kNewtObjSweep;
            prevp = &obj->header.nextp;
        }
    }
}

#endif

/*------------------------------------------------------------------------*/
/** メモリプール内のオブジェクトをスウィープ（掃除）する
 *
 * @param pool		[in] メモリプール
 * @param mark		[in] マークフラグ
 *
 * @return			なし
 */

void NewtPoolSweep(newtPool pool, bool mark)
{
    if (pool != NULL)
    {
        newtObjRef	nextp;
        newtObjRef	obj;
        newtObjRef *	prevp = &pool->obj;
        int32_t		usesize;

        usesize = pool->usesize;

        for (obj = pool->obj; obj != NULL; obj = nextp)
        {
            nextp = obj->header.nextp;

            if (NewtObjIsLiteral(obj))
            {
                NewtObjChain(&pool->literal, obj);
                *prevp = nextp;

                continue;
            }

            if (NewtObjIsSweep(obj, mark))
            {
                *prevp = nextp;
                NewtObjFree(pool, obj);

                continue;
            }

            prevp = &obj->header.nextp;
        }

        if (NEWT_DEBUG)
            NewtPoolSnap("GC", pool, usesize);
    }

    if (pool->maxspace < pool->usesize)
    {
        pool->maxspace = ((pool->usesize + pool->expandspace - 1) / pool->expandspace)
                            * pool->expandspace;
    }
}


/*------------------------------------------------------------------------*/
/** オブジェクトをマークする
 *
 * @param r			[in] オブジェクト
 * @param mark		[in] マークフラグ
 *
 * @return			なし
 */

void NewtGCRefMark(newtRefArg r, bool mark)
{
    if (NewtRefIsPointer(r))
    {
        newtObjRef	obj;
    
        obj = NewtRefToPointer(r);
    
        if (! NewtObjIsLiteral(obj) && NewtObjIsSweep(obj, mark))
        {
            if (mark)
                obj->header.h &= ~ (uint32_t)kNewtObjSweep;
            else
                obj->header.h |= kNewtObjSweep;

            if (NewtObjIsSlotted(obj))
            {
                newtRef *	slots;
                uint32_t	len;
                uint32_t	i;
    
                len = NewtObjSlotsLength(obj);
                slots = NewtObjToSlots(obj);
    
                for (i = 0; i < len; i++)
                {
                    NewtGCRefMark(slots[i], mark);
                }
            }
    
            if (NewtObjIsFrame(obj))
                NewtGCRefMark(obj->as.map, mark);
        }
    }
}


/*------------------------------------------------------------------------*/
/** レジスタ内のオブジェクトをマークする
 *
 * @param reg		[in] レジスタ
 * @param mark		[in] マークフラグ
 *
 * @return			なし
 */

void NewtGCRegMark(vm_reg_t * reg, bool mark)
{
    NewtGCRefMark(reg->func, mark);
    NewtGCRefMark(reg->locals, mark);
    NewtGCRefMark(reg->rcvr, mark);
    NewtGCRefMark(reg->impl, mark);
}


/*------------------------------------------------------------------------*/
/** スタック内のオブジェクトをマークする
 *
 * @param env		[in] 実行環境
 * @param mark		[in] マークフラグ
 *
 * @return			なし
 */

void NewtGCStackMark(vm_env_t * env, bool mark)
{
    newtRef *	stack;
    vm_reg_t *	callstack;
    uint32_t	i;

    // スタック
    stack = (newtRef *)env->stack.stackp;

    for (i = 0; i < env->reg.sp; i++)
    {
        NewtGCRefMark(stack[i], mark);
    }

    // 関数呼出しスタック
    callstack = (vm_reg_t *)env->callstack.stackp;

    for (i = 0; i < env->callstack.sp; i++)
    {
        NewtGCRegMark(&callstack[i], mark);
    }

    // 例外ハンドラ・スタック
    NewtGCRefMark(env->currexcp, mark);

/*
    {
        vm_excp_t *	excpstack;
        vm_excp_t *	excp;

        excpstack = (vm_excp_t *)env->excpstack.stackp;

        for (i = 0; i < env->excpsp; i++)
        {
            excp = &excpstack[i];
            NewtGCRefMark(excp->sym, mark);
        }
    }
*/
}


/*------------------------------------------------------------------------*/
/** 参照されているオブジェクトをマークする
 *
 * @param env		[in] 実行環境
 * @param mark		[in] マークフラグ
 *
 * @return			なし
 */

void NewtGCMark(vm_env_t * env, bool mark)
{
    NewtGCRefMark(NcGetRoot(), mark);
//    NewtGCRefMark(NSGetGlobals(), mark);
//    NewtGCRefMark(NSGetGlobalFns(), mark);
//    NewtGCRefMark(NSGetMagicPointers(), mark);

    // レジスタ
    NewtGCRegMark(&env->reg, mark);

    // スタック
    NewtGCStackMark(env, mark);
}


/*------------------------------------------------------------------------*/
/** ガベージコレクションの実行
 *
 * @return			なし
 */

void NewtGC(void)
{
//    NewtPoolMarkClean(NEWT_POOL);
	vm_env_t *	env;

	for (env = &vm_env; env; env = env->next)
	{
		NewtGCMark(&vm_env, NEWT_SWEEP);
	}

	NewtPoolSweep(NEWT_POOL, NEWT_SWEEP);

    NEWT_SWEEP = ! NEWT_SWEEP;
    NEWT_NEEDGC = false;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** スクリプトから GC　を呼出す（実際には GC を予約するだけ）
 *
 * @param rcvr		[in] レシーバ
 *
 * @return			NIL
 *
 * @note			スクリプトからの呼出し用
 */

newtRef	NsGC(newtRefArg rcvr)
{
	NEWT_NEEDGC = true;
    return kNewtRefNIL;
}
