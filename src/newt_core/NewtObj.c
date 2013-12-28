/*------------------------------------------------------------------------*/
/**
 * @file	NewtObj.c
 * @brief   オブジェクトシステム
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "NewtCore.h"
#include "NewtGC.h"
#include "NewtIO.h"


/* 関数プロトタイプ */
static newtRef		NewtMakeSymbol0(const char *s);
static bool			NewtBSearchSymTable(newtRefArg r, const char * name, uint32_t hash, int32_t st, int32_t * indexP);
static newtObjRef   NewtObjMemAlloc(newtPool pool, uint32_t n, bool literal);
static newtObjRef   NewtObjRealloc(newtPool pool, newtObjRef obj, uint32_t n);
static void			NewtGetObjData(newtRefArg r, uint8_t * data, uint32_t len);
static newtObjRef   NewtObjBinarySetLength(newtObjRef obj, uint32_t n);
static uint32_t		NewtObjSymbolLength(newtObjRef obj);
static uint32_t		NewtObjStringLength(newtObjRef obj);
static newtObjRef   NewtObjStringSetLength(newtObjRef obj, uint32_t n);
static void			NewtMakeInitSlots(newtRefArg r, uint32_t st, uint32_t n, uint32_t step, const newtRefVar v[]);
static newtObjRef   NewtObjSlotsSetLength(newtObjRef obj, uint32_t n, newtRefArg v);
static int			NewtInt32Compare(newtRefArg r1, newtRefArg r2);
static int			NewtRealCompare(newtRefArg r1, newtRefArg r2);
static int			NewtStringCompare(newtRefArg r1, newtRefArg r2);
static int			NewtBinaryCompare(newtRefArg r1, newtRefArg r2);
static uint16_t		NewtArgsType(newtRefArg r1, newtRefArg r2);

static newtRef		NewtMakeThrowSymbol(int32_t err);

static bool			NewtObjHasProto(newtObjRef obj);
static bool			NewtMapIsSorted(newtRefArg r);
static void			NewtObjRemoveArraySlot(newtObjRef obj, int32_t n);
static void			NewtDeeplyCopyMap(newtRef * dst, int32_t * pos, newtRefArg src);
static newtRef		NewtDeeplyCloneMap(newtRefArg map, int32_t len);
static void			NewtObjRemoveFrameSlot(newtObjRef obj, newtRefArg slot);
static bool			NewtStrNBeginsWith(char * str, uint32_t len, char * sub, uint32_t sublen);
static bool			NewtStrIsSubclass(char * sub, uint32_t sublen, char * supr, uint32_t suprlen);
static bool			NewtStrHasSubclass(char * sub, uint32_t sublen, char * supr, uint32_t suprlen);


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** シンボルのハッシュ値を計算
 *
 * @param name		[in] シンボル名
 *
 * @return			ハッシュ値
 */

uint32_t NewtSymbolHashFunction(const char * name)
{
    uint32_t result = 0;
    char c;

    while (*name)
    {
        c = *name;

        if (c >= 'a' && c <= 'z')
            result = result + c - ('a' - 'A');
        else
            result = result + c;

        name++;
    }

    return result * 2654435769U;
}


/*------------------------------------------------------------------------*/
/** シンボルオブジェクトの作成
 *
 * @param s			[in] 文字列
 *
 * @return			シンボルオブジェクト
 */

newtRef NewtMakeSymbol0(const char *s)
{
    newtObjRef	obj;
    uint32_t	size;

    size = sizeof(uint32_t) + strlen(s) + 1;
    obj = NewtObjAlloc(kNewtSymbolClass, size, 0, true);

    if (obj != NULL)
    {
        newtSymDataRef	objData;
        uint32_t	setlen;

        objData = NewtObjToSymbol(obj);

        setlen = NewtObjSize(obj) - size;

        if (0 < setlen)
            memset(objData + size, 0, setlen);

        objData->hash = NewtSymbolHashFunction(s);
        strcpy(objData->name, s);

        return NewtMakePointer(obj);
    }

    return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** シンボルテーブルの位置検索
 *
 * @param r			[in] シンボルテーブル
 * @param name		[in] シンボル文字列
 * @param hash		[in] ハッシュ値
 * @param st		[in] 開始位置
 * @param indexP	[out]位置
 *
 * @retval			true	成功
 * @retval			false   失敗
 */

bool NewtBSearchSymTable(newtRefArg r, const char * name, uint32_t hash,
    int32_t st, int32_t * indexP)
{
    newtSymDataRef	sym;
    newtRef *	slots;
    int32_t	len;
    int32_t	ed;
    int32_t	md = st;
    int16_t	comp;

    slots = NewtRefToSlots(r);

    if (hash == 0)
        hash = NewtSymbolHashFunction(name);

    len = NewtArrayLength(r);
    ed = len - 1;

    while (st <= ed)
    {
        md = (st + ed) / 2;

        sym = NewtRefToSymbol(slots[md]);

		if (hash < sym->hash)
			comp = -1;
		else if (hash > sym->hash)
			comp = 1;
		else
			comp = 0;

        if (comp == 0)
            comp = strcasecmp(name, sym->name);

        if (comp == 0)
        {
            *indexP = md;
            return true;
        }

        if (comp < 0)
            ed = md - 1;
        else
            st = md + 1;
    }

    if (len < st)
        *indexP = len;
    else
        *indexP = st;

    return false;
}


/*------------------------------------------------------------------------*/
/** シンボルのルックアップ
 *
 * @param r			[in] シンボルテーブル
 * @param name		[in] シンボル文字列
 * @param hash		[in] ハッシュ値
 * @param st		[in] 開始位置
 *
 * @return			シンボルオブジェクト
 *
 * @note			未登録の場合はシンボルオブジェクトを作成しシンボルテーブルに登録する
 */

newtRef NewtLookupSymbol(newtRefArg r, const char * name, uint32_t hash, int32_t st)
{
    newtRefVar	sym;
    int32_t	index;

    if (NewtBSearchSymTable(r, name, 0, st, &index))
        return NewtGetArraySlot(r, index);

    sym = NewtMakeSymbol0(name);
    NewtInsertArraySlot(r, index, sym);

    return sym;
}


/*------------------------------------------------------------------------*/
/** シンボルのルックアップ
 *
 * @param r			[in] シンボルテーブル
 * @param name		[in] シンボル文字列
 * @param st		[in] 開始位置
 *
 * @return			シンボルオブジェクト
 */

newtRef NewtLookupSymbolArray(newtRefArg r, newtRefArg name, int32_t st)
{
    newtSymDataRef	sym;

    sym = NewtRefToSymbol(name);

    if (sym != NULL)
        return NewtLookupSymbol(r, sym->name, sym->hash, st);
    else
        return kNewtRefUnbind;
}


/**
 * Return the ASCII string associated with a symbol ref.
 * Remark: doesn't check that the passed object is indeed a symbol.
 *
 * @param inSymbol	symbol object
 * @return a pointer to the name of the symbol
 */
 
const char*	NewtSymbolGetName(newtRefArg inSymbol)
{
	return NewtRefToSymbol(inSymbol)->name;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** オブジェクトのオブジェクトタイプの取得
 *
 * @param r			[in] オブジェクト
 * @param detail	[in] リテラルフラグ
 *
 * @return			オブジェクトタイプ
 */

uint16_t NewtGetRefType(newtRefArg r, bool detail)
{
    uint16_t	type = kNewtUnknownType;

    switch (r & 3)
    {
        case 0:	// Integer
            type = kNewtInt30;
            break;

        case 1:	// Pointer
            if (detail)
                type = NewtGetObjectType(NewtRefToPointer(r), true);
            else
                type = kNewtPointer;
            break;

        case 2:	// Character or Special
            switch (r)
            {
                case kNewtRefNIL:
                    type = kNewtNil;
                    break;

                case kNewtRefTRUE:
                    type = kNewtTrue;
                    break;

                case kNewtRefUnbind:
                    type = kNewtUnbind;
                    break;

                case kNewtSymbolClass:
                    type = kNewtSymbol;
                    break;

                default:
                    if ((r & 0xF) == 6)
                        type = kNewtCharacter;
                    else
                        type = kNewtSpecial;
                    break;
            }
            break;

        case 3:	// Magic Pointer
            type = kNewtMagicPointer;
            break;
    }

    return type;
}


/*------------------------------------------------------------------------*/
/** オブジェクトデータのオブジェクトタイプの取得
 *
 * @param obj		[in] オブジェクトデータ
 * @param detail	[in] ディテイルフラグ
 *
 * @return			オブジェクトタイプ
 */

uint16_t NewtGetObjectType(newtObjRef obj, bool detail)
{
    uint16_t	type = kNewtUnknownType;

    switch (NewtObjType(obj))
    {
        case 0:	// binary
            type = kNewtBinary;

            if (detail)
            {
                if (obj->as.klass == kNewtSymbolClass)
                    type = kNewtSymbol;
                else if (NewtRefEqual(obj->as.klass, NSSYM0(string)))
                    type = kNewtString;
                else if (NewtRefEqual(obj->as.klass, NSSYM0(int32)))
                    type = kNewtInt32;
                else if (NewtRefEqual(obj->as.klass, NSSYM0(real)))
                    type = kNewtReal;
				else if (NewtIsSubclass(obj->as.klass, NSSYM0(string)))
                    type = kNewtString;
            }
            break;

        case 1:	// array
            type = kNewtArray;
            break;

        case 3:	// frame
            type = kNewtFrame;
            break;
    }

    return type;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** オブジェクトデータの実データサイズを計算
 *
 * @param n			[in] データサイズ
 *
 * @return			実データサイズ
 */

uint32_t NewtObjCalcDataSize(uint32_t n)
{
    if (n < 4)
        return 4;
    else
        return n;
}


/*------------------------------------------------------------------------*/
/** オブジェクトデータのメモリ確保
 *
 * @param pool		[in] メモリプール
 * @param n			[in] データサイズ
 * @param literal	[in] リテラルフラグ
 *
 * @return			オブジェクトデータ
 */

newtObjRef NewtObjMemAlloc(newtPool pool, uint32_t n, bool literal)
{
    newtObjRef	obj;
    uint32_t	newSize;

    if (literal)
    {
        newSize = NewtAlign(sizeof(newtObj) + n, 4);
        obj = NewtObjChainAlloc(pool, newSize, 0);
    }
    else
    {
        newSize = NewtObjCalcDataSize(n);
        obj = NewtObjChainAlloc(pool, sizeof(newtObj) + sizeof(uint8_t *), newSize);
    }

    return obj;
}


/*------------------------------------------------------------------------*/
/** オブジェクトのメモリ確保
 *
 * @param r			[in] クラス／マップ
 * @param n			[in] サイズ
 * @param type		[in] オブジェクトタイプ
 * @param literal	[in] リテラルフラグ
 *
 * @return			オブジェクト
 */

newtObjRef NewtObjAlloc(newtRefArg r, uint32_t n, uint16_t type, bool literal)
{
    newtObjRef	obj;

    obj = NewtObjMemAlloc(NEWT_POOL, n, literal);
    if (obj == NULL) return NULL;

    obj->header.h |= (n << 8) | type;

    if (NEWT_SWEEP)
        obj->header.h |= kNewtObjSweep;

    if ((type & kNewtObjFrame) != 0)
        obj->as.map = r;
    else
        obj->as.klass = r;

    return obj;
}


/*------------------------------------------------------------------------*/
/** オブジェクトデータのメモリ再確保
 *
 * @param pool		[in] メモリプール
 * @param obj		[in] オブジェクトデータ
 * @param n			[in] サイズ
 *
 * @return			オブジェクトデータ
 */

newtObjRef NewtObjRealloc(newtPool pool, newtObjRef obj, uint32_t n)
{
    uint8_t **	datap;
    uint8_t *	data;
    int32_t	oldSize;
    int32_t	newSize;
    int32_t	addSize;

    oldSize = NewtObjCalcDataSize(NewtObjSize(obj));
    newSize = NewtObjCalcDataSize(n);
    addSize = newSize - oldSize;

    if (0 < addSize)
        NewtCheckGC(pool, addSize);

    datap = (uint8_t **)(obj + 1);
    data = NewtMemRealloc(pool, *datap, newSize);
    if (data == NULL) return NULL;

    pool->usesize += addSize;

    if (data != *datap)
        *datap = data;

    obj->header.h = ((n << 8) | (obj->header.h & 0xff));

    return obj;
}


/*------------------------------------------------------------------------*/
/** オブジェクトデータのサイズ変更
 *
 * @param obj		[in] オブジェクトデータ
 * @param n			[in] サイズ
 *
 * @return			オブジェクトデータ
 */

newtObjRef NewtObjResize(newtObjRef obj, uint32_t n)
{
    if (NewtObjIsReadonly(obj))
    {
        NewtThrow0(kNErrObjectReadOnly);
        return NULL;
    }

    return NewtObjRealloc(NEWT_POOL, obj, n);
}


/*------------------------------------------------------------------------*/
/** オブジェクトデータのデータ部を取得
 *
 * @param obj		[in] オブジェクトデータ
 *
 * @return			データ部
 */

void * NewtObjData(newtObjRef obj)
{
    void *	data;

    data = (void *)(obj + 1);

    if (NewtObjIsLiteral(obj))
        return data;
    else
        return *((void **)data);
}


/*------------------------------------------------------------------------*/
/** オブジェクのクローン複製
 *
 * @param r			[in] オブジェクト
 *
 * @return			クローン複製されたオブジェクト
 */

newtRef NewtObjClone(newtRefArg r)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(r);

    if (obj != NULL)
    {
        newtObjRef	newObj = NULL;
        uint32_t	size;
        uint16_t	type;

        size = NewtObjSize(obj);
        type = NewtObjType(obj);

        switch (NewtGetObjectType(obj, true))
        {
            case kNewtSymbol:
            case kNewtReal:
                return r;

            case kNewtFrame:
                {
                    newtRefVar	map;

                    if (NewtRefIsLiteral(obj->as.map))
                        map = obj->as.map;
                    else
                        map = NcClone(obj->as.map);

                    newObj = NewtObjAlloc(map, size, type, false);
                }
                break;

            default:
                newObj = NewtObjAlloc(obj->as.klass, size, type, false);
                break;
        }

        if (newObj != NULL)
        {
            uint8_t *	src;
            uint8_t *	dst;

            src = NewtObjToBinary(obj);
            dst = NewtObjToBinary(newObj);
            memcpy(dst, src, size);

            return NewtMakePointer(newObj);
        }
    }

    return (newtRef)r;
}


/*------------------------------------------------------------------------*/
/** オブジェクのリテラル化
 *
 * @param r			[in] オブジェクト
 *
 * @return			リテラル化されたオブジェクト
 */

newtRef NewtPackLiteral(newtRefArg r)
{
    newtObjRef	obj;

    if (NewtRefIsLiteral(r))
        return r;

    obj = NewtRefToPointer(r);

    if (obj != NULL)
    {
        newtObjRef	newObj = NULL;
        uint32_t	size;
        uint16_t	type;

        size = NewtObjSize(obj);
        type = NewtObjType(obj);

        if (NewtObjIsFrame(obj))
        {
            obj->as.map = NewtPackLiteral(obj->as.map);
            newObj = NewtObjAlloc(obj->as.map, size, type, true);
		}
        else
        {
            newObj = NewtObjAlloc(obj->as.klass, size, type, true);
        }

        if (newObj != NULL)
        {
            uint8_t *	src;
            uint8_t *	dst;

            src = NewtObjToBinary(obj);
            dst = NewtObjToBinary(newObj);
            memcpy(dst, src, size);

            if (NewtObjIsSlotted(newObj))
            {
                newtRef *	slots;
                uint32_t	len;
                uint32_t	i;

                len = NewtObjSlotsLength(newObj);
                slots = NewtObjToSlots(newObj);

                for (i = 0; i < len; i++)
                {
                    slots[i] = NewtPackLiteral(slots[i]);
                }
            }

            newObj->header.h |= kNewtObjLiteral;

            // obj を free してはいけない
            // GC にまかせる

            return NewtMakePointer(newObj);
        }
    }

    return (newtRef)r;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** オブジェクのデータ部をバッファに取出す
 *
 * @param r			[in] オブジェクト
 * @param data		[out]バッファ
 * @param len		[in] バッファ長
 *
 * @return			なし
 */

void NewtGetObjData(newtRefArg r, uint8_t * data, uint32_t len)
{
    newtObjRef	obj;
    uint8_t *	objData;

    obj = NewtRefToPointer(r);
    objData = NewtObjToBinary(obj);

    memcpy(data, objData, len);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** オブジェクトがリテラルかチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	リテラル
 * @retval			false   リテラルでない
 */

bool NewtRefIsLiteral(newtRefArg r)
{
    if (NewtRefIsPointer(r))
    {
        newtObjRef	obj;

        obj = NewtRefToPointer(r);

        return NewtObjIsLiteral(obj);
    }

    return true;
}


/*------------------------------------------------------------------------*/
/** オブジェクにスウィープフラグが立っているかチェックする
 *
 * @param r			[in] オブジェクト
 * @param mark		[in] マーク
 *
 * @retval			true	スウィープフラグが立っている
 * @retval			false   スウィープフラグが立っていない
 */

bool NewtRefIsSweep(newtRefArg r, bool mark)
{
    if (NewtRefIsPointer(r))
    {
        newtObjRef	obj;

        obj = NewtRefToPointer(r);

        return NewtObjIsSweep(obj, mark);
    }

    return true;
}


/*------------------------------------------------------------------------*/
/** オブジェクトが NIL かチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	NIL または #UNBIND
 * @retval			false   NIL でない
 */

bool NewtRefIsNIL(newtRefArg r)
{
    return (kNewtRefNIL == r || kNewtRefUnbind == r);
}


/*------------------------------------------------------------------------*/
/** オブジェクトがシンボルかチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	シンボル
 * @retval			false   シンボルでない
 */

bool NewtRefIsSymbol(newtRefArg r)
{
    return (kNewtSymbol == NewtGetRefType(r, true));
}


/*------------------------------------------------------------------------*/
/** オブジェクトのハッシュ値を取得する
 *
 * @param r			[in] オブジェクト
 *
 * @return			ハッシュ値
 */

uint32_t NewtRefToHash(newtRefArg r)
{
    newtSymDataRef	sym;

    sym = NewtRefToSymbol(r);

    if (sym != NULL)
        return sym->hash;
    else
        return 0;
}


/*------------------------------------------------------------------------*/
/** オブジェクトが文字列かチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	文字列
 * @retval			false   文字列でない
 */

bool NewtRefIsString(newtRefArg r)
{
    return (kNewtString == NewtGetRefType(r, true));
}


/*------------------------------------------------------------------------*/
/** オブジェクトが整数かチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	整数
 * @retval			false   整数でない
 */

bool NewtRefIsInteger(newtRefArg r)
{
	return (NewtRefIsInt30(r) || NewtRefIsInt32(r));
}


/*------------------------------------------------------------------------*/
/** 整数オブジェクを整数にする
 *
 * @param r			[in] オブジェクト
 *
 * @return			整数
 */

int32_t NewtRefToInteger(newtRefArg r)
{
    int32_t	v = 0;

    if (NewtRefIsInt30(r))
        v = NewtRefToInt30(r);
    else if (NewtRefIsNIL(r))
      v = NewtMakeInt30(0);
    else
        NewtGetObjData(r, (uint8_t *)&v, sizeof(v));

    return v;
}


/*------------------------------------------------------------------------*/
/** オブジェクトが32bit整数かチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	32bit整数
 * @retval			false   32bit整数でない
 */

bool NewtRefIsInt32(newtRefArg r)
{
    return (kNewtInt32 == NewtGetRefType(r, true));
}


/*------------------------------------------------------------------------*/
/** オブジェクトが浮動小数点かチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	浮動小数点
 * @retval			false   浮動小数点でない
 */

bool NewtRefIsReal(newtRefArg r)
{
    return (kNewtReal == NewtGetRefType(r, true));
}


/*------------------------------------------------------------------------*/
/** 数値オブジェクを浮動小数点にする
 *
 * @param r			[in] オブジェクト
 *
 * @return			浮動小数点
 */

double NewtRefToReal(newtRefArg r)
{
    double	v = 0.0;

    if (NewtRefIsInteger(r))
        v = NewtRefToInteger(r);
    else
        NewtGetObjData(r, (uint8_t *)&v, sizeof(v));

    return v;
}


/*------------------------------------------------------------------------*/
/** オブジェクトがバイナリオブジェクトかチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	バイナリオブジェクト
 * @retval			false   バイナリオブジェクトでない
 */

bool NewtRefIsBinary(newtRefArg r)
{
    if (NewtRefIsPointer(r))
    {
        uint16_t	type;

        type = NewtGetObjectType(NewtRefToPointer(r), false);

        return (type == kNewtBinary);
    }

    return false;
}


/*------------------------------------------------------------------------*/
/** オブジェクトのオブジェクトデータを取得する
 *
 * @param r			[in] オブジェクト
 *
 * @return			オブジェクトデータ
 */

void * NewtRefToData(newtRefArg r)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(r);

    return NewtObjData(obj);
}


/*------------------------------------------------------------------------*/
/** オブジェクトが配列オブジェクトかチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	配列オブジェクト
 * @retval			false   配列オブジェクトでない
 */

bool NewtRefIsArray(newtRefArg r)
{
    return (NewtGetRefType(r, true) == kNewtArray);
}


/*------------------------------------------------------------------------*/
/** オブジェクトがフレームオブジェクトかチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	フレームオブジェクト
 * @retval			false   フレームオブジェクトでない
 */

bool NewtRefIsFrame(newtRefArg r)
{
    return (NewtGetRefType(r, true) == kNewtFrame);
}


/*------------------------------------------------------------------------*/
/** オブジェクトがフレームまたは配列オブジェクトかチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	フレームまたは配列オブジェクト
 * @retval			false   フレームまたは配列オブジェクトでない
 */

bool NewtRefIsFrameOrArray(newtRefArg r)
{
    uint16_t	type;

    type = NewtGetRefType(r, true);
    return (type == kNewtFrame || type == kNewtArray);
}


/*------------------------------------------------------------------------*/
/** オブジェクトがイミディエイト（即値）かチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	イミディエイトである
 * @retval			false   イミディエイトでない
 */

bool NewtRefIsImmediate(newtRefArg r)
{
#ifdef __NAMED_MAGIC_POINTER__
    if (NewtRefIsNamedMP(r))
		return false;
#endif /* __NAMED_MAGIC_POINTER__ */

    return ! NewtRefIsPointer(r);
}


/*------------------------------------------------------------------------*/
/** オブジェクトがコードブロック（関数オブジェクト）かチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	コードブロック
 * @retval			false   コードブロックでない
 */

bool NewtRefIsCodeBlock(newtRefArg r)
{
    if (NewtRefIsFrame(r))
    {
        newtRefVar	klass;

        klass = NcClassOf(r);

        if (NewtRefEqual(klass, NSSYM0(CodeBlock)))
            return true;
    }

    return false;
}


/*------------------------------------------------------------------------*/
/** オブジェクトがネイティブ関数（rcvrなし関数オブジェクト）かチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	ネイティブ関数
 * @retval			false   ネイティブ関数でない
 */

bool NewtRefIsNativeFn(newtRefArg r)
{
    if (NewtRefIsFrame(r))
        return NewtRefEqual(NcClassOf(r), NSSYM0(_function.native0));
	else
		return false;
}


/*------------------------------------------------------------------------*/
/** オブジェクトがネイティブ関数（rcvrあり関数オブジェクト）かチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	ネイティブ関数
 * @retval			false   ネイティブ関数でない
 */

bool NewtRefIsNativeFunc(newtRefArg r)
{
    if (NewtRefIsFrame(r))
        return NewtRefEqual(NcClassOf(r), NSSYM0(_function.native));
	else
		return false;
}


/*------------------------------------------------------------------------*/
/** オブジェクトが関数オブジェクトかチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	関数オブジェクト
 * @retval			false   関数オブジェクトでない
 */

bool NewtRefIsFunction(newtRefArg r)
{
	return (NewtRefFunctionType(r) != kNewtNotFunction);
}


/*------------------------------------------------------------------------*/
/** 関数オブジェクトのタイプを取得する
 *
 * @param r			[in] オブジェクト
 *
 * @retval			kNewtNotFunction	関数オブジェクトでない
 * @retval			kNewtCodeBlock		バイトコード関数
 * @retval			kNewtNativeFn		ネイティブ関数（rcvrなし、Old Style）
 * @retval			kNewtNativeFunc		ネイティブ関数（rcvrあり、New Style）
 */

int NewtRefFunctionType(newtRefArg r)
{
    if (NewtRefIsFrame(r))
    {
        newtRefVar	klass;

        klass = NcClassOf(r);

        if (NewtRefEqual(klass, NSSYM0(CodeBlock)))
			return kNewtCodeBlock;

		if (NewtRefEqual(klass, NSSYM0(_function.native0)))
			return kNewtNativeFn;

		if (NewtRefEqual(klass, NSSYM0(_function.native)))
			return kNewtNativeFunc;
    }

    return kNewtNotFunction;
}


/*------------------------------------------------------------------------*/
/** オブジェクトが正規表現オブジェクトかチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	正規表現オブジェクト
 * @retval			false   正規表現オブジェクトでない
 */

bool NewtRefIsRegex(newtRefArg r)
{
    if (NewtRefIsFrame(r))
    {
        newtRefVar	klass;

        klass = NcClassOf(r);

        if (NewtRefEqual(klass, NSSYM0(regex)))
            return true;
    }

    return false;
}


/*------------------------------------------------------------------------*/
/** 整数オブジェクトをアドレスに変換する
 *
 * @param r			[in] オブジェクト
 *
 * @return			アドレス
 */

void * NewtRefToAddress(newtRefArg r)
{
	if (NewtRefIsInteger(r))
		return (void *)(((uint32_t)NewtRefToInteger(r)) << NOBJ_ADDR_SHIFT);
	else
		return NULL;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** バイナリオブジェクトを作成する
 *
 * @param klass		[in] クラス
 * @param data		[in] 初期化データ
 * @param size		[in] サイズ
 * @param literal	[in] リテラルフラグ
 *
 * @return			バイナリオブジェクト
 */

newtRef NewtMakeBinary(newtRefArg klass, uint8_t * data, uint32_t size, bool literal)
{
    newtObjRef	obj;

    obj = NewtObjAlloc(klass, size, 0, literal);

    if (obj != NULL)
    {
        uint8_t *	objData;

        objData = NewtObjToBinary(obj);

        if (data != NULL && 0 < size)
        {
            uint32_t	setlen;

            setlen = NewtObjSize(obj) - size;

            if (0 < setlen)
                memset(objData + size, 0, setlen);

            memcpy(objData, data, size);
        }
        else
        {
            memset(objData, 0, NewtObjSize(obj));
        }

        return NewtMakePointer(obj);
    }

    return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** Make a binary object from a string of hexadecimal numbers.
 *
 * @param klass         [in] new class 
 * @param hex           [in] string of hexadecimal numbers
 * @param literal       [in] make new object a literal
 *
 * @return                      new binary object
 */

newtRef NewtMakeBinaryFromHex(newtRefArg klass, const char *hex, bool literal)
{
    uint32_t size = strlen(hex)/2;
    newtRef obj = NewtMakeBinary(klass, 0, size, literal);
    if (obj) {
        uint32_t i;
        const char *src = hex;
        uint8_t *dst = NewtRefToBinary(obj);
        for (i=0; i<size; i++) {
            uint8_t c = *src++, v = 0;
            if (c>='a') v = c-'a'+10; else if (c>='A') v = c-'A'+10; else v = c-'0';
            c = *src++; v = v<<4;
            if (c>='a') v += c-'a'+10; else if (c>='A') v += c-'A'+10; else v += c-'0';
            *dst++ = v;
        }
        return obj;
    }
    return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** バイナリオブジェクトのオブジェクトデータのサイズを変更する
 *
 * @param obj		[in] オブジェクトデータ
 * @param n			[in] サイズ
 *
 * @return			サイズの変更されたオブジェクトデータ
 */

newtObjRef NewtObjBinarySetLength(newtObjRef obj, uint32_t n)
{
    return NewtObjResize(obj, n);
}


/*------------------------------------------------------------------------*/
/** バイナリオブジェクトのサイズを変更する
 *
 * @param r			[in] オブジェクト
 * @param n			[in] サイズ
 *
 * @return			オブジェクト
 */

newtRef NewtBinarySetLength(newtRefArg r, uint32_t n)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(r);
    NewtObjBinarySetLength(obj, n);

    return r;
}


/*------------------------------------------------------------------------*/
/** シンボルオブジェクトを作成する
 *
 * @param s			[in] 文字列
 *
 * @return			シンボルオブジェクト
 *
 * @note			シンボルが既に存在する場合は作成せずに既にあるシンボルオブジェクトを返す
 */

newtRef NewtMakeSymbol(const char *s)
{
    return NewtLookupSymbolTable(s);
}


/*------------------------------------------------------------------------*/
/** シンボルのオブジェクトデータの長さを取得する
 *
 * @param obj		[in] オブジェクトデータ
 *
 * @return			長さ
 */

uint32_t NewtObjSymbolLength(newtObjRef obj)
{
    newtSymDataRef	sym;

    sym = NewtObjToSymbol(obj);
    return strlen(sym->name);
}


/*------------------------------------------------------------------------*/
/** 文字列オブジェクトを作成する
 *
 * @param s			[in] 文字列
 * @param literal	[in] リテラルフラグ
 *
 * @return			文字列オブジェクト
 */

newtRef NewtMakeString(const char *s, bool literal)
{
    return NewtMakeBinary(NSSYM0(string), (uint8_t *)s, strlen(s) + 1, literal); 
}


/*------------------------------------------------------------------------*/
/** 長さを指定して文字列オブジェクトを作成する
 *
 * @param s			[in] 文字列
 * @param len		[in] 文字列の長さ
 * @param literal	[in] リテラルフラグ
 *
 * @return			文字列オブジェクト
 */

newtRef NewtMakeString2(const char *s, uint32_t len, bool literal)
{
	newtRefVar  r;

    r = NewtMakeBinary(NSSYM0(string), (uint8_t *)s, len + 1, literal); 

	if (NewtRefIsNotNIL(r))
	{
        char *	objData;

        objData = NewtRefToString(r);

		if (s != NULL && 0 < len)
		{
//			strncpy(objData, s, len);
			objData[len] = '\0';
		}
		else
		{
			objData[0] = '\0';
		}
	}

	return r;
}


/*------------------------------------------------------------------------*/
/** 文字列オブジェクトのオブジェクトデータの長さを取得する
 *
 * @param obj		[in] オブジェクトデータ
 *
 * @return			長さ
 */

uint32_t NewtObjStringLength(newtObjRef obj)
{
    char *	s;

    s = NewtObjToString(obj);
    return strlen(s);
}


/*------------------------------------------------------------------------*/
/** 文字列オブジェクトのオブジェクトデータの長さを変更する
 *
 * @param obj		[in] オブジェクトデータ
 * @param len		[in] 長さ
 *
 * @return			長さが変更されたオブジェクトデータ
 */

newtObjRef NewtObjStringSetLength(newtObjRef obj, uint32_t n)
{
    return NewtObjBinarySetLength(obj, n + 1);
}


/*------------------------------------------------------------------------*/
/** 文字列オブジェクトの長さを変更する
 *
 * @param r			[in] オブジェクト
 * @param n			[in] 長さ
 *
 * @return			オブジェクト
 */

newtRef NewtStringSetLength(newtRefArg r, uint32_t n)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(r);
    NewtObjStringSetLength(obj, n);

    return r;
}


/*------------------------------------------------------------------------*/
/** 整数オブジェクトを作成する
 *
 * @param v			[in] 整数
 *
 * @return			整数オブジェクト
 */

newtRef NewtMakeInteger(int32_t v)
{
	if (-536870912 <= v && v <= 536870911)
	{   // 30bit 以内の場合
		return NewtMakeInt30(v);
	}
	else
	{
		return NewtMakeInt32(v);
	}
}


/*------------------------------------------------------------------------*/
/** 32bit整数オブジェクトを作成する
 *
 * @param v			[in] 整数
 *
 * @return			32bit整数オブジェクト
 */

newtRef NewtMakeInt32(int32_t v)
{
    return NewtMakeBinary(NSSYM0(int32), (uint8_t *)&v, sizeof(v), true); 
}


/*------------------------------------------------------------------------*/
/** 浮動小数点オブジェクトを作成する
 *
 * @param v			[in] 浮動小数点
 *
 * @return			浮動小数点オブジェクト
 */

newtRef NewtMakeReal(double v)
{
    return NewtMakeBinary(NSSYM0(real), (uint8_t *)&v, sizeof(v), true); 
}


/*------------------------------------------------------------------------*/
/** 配列オブジェクトを作成する
 *
 * @param klass		[in] クラス
 * @param n			[in] 長さ
 *
 * @return			配列オブジェクト
 */

newtRef NewtMakeArray(newtRefArg klass, uint32_t n)
{
    return NewtMakeSlotsObj(klass, n, 0);
}

void NewtMakeInitSlots(newtRefArg r, uint32_t st, uint32_t n, uint32_t step, const newtRefVar v[])
{
    if (v != NULL)
    {
        newtRef *	slots;
        uint32_t	i;
    
        slots = NewtRefToSlots(r);
    
        for (i = 0; i < n; i++)
        {
            slots[st + i] = *v;
            v += step;
        }
    }
}


/*------------------------------------------------------------------------*/
/** 配列オブジェクトを作成して初期化する
 *
 * @param klass		[in] クラス
 * @param n			[in] 長さ
 * @param v			[in] 初期化データ
 *
 * @return			配列オブジェクト
 */

newtRef NewtMakeArray2(newtRefArg klass, uint32_t n, const newtRefVar v[])
{
    newtRefVar	r;

    r = NewtMakeSlotsObj(klass, n, 0);

    if (NewtRefIsNotNIL(r))
        NewtMakeInitSlots(r, 0, n, 1, v);

    return r;
}


/*------------------------------------------------------------------------*/
/** マップを作成して初期化する
 *
 * @param superMap	[in] スーパマップ
 * @param n			[in] 長さ
 * @param v			[in] 初期化データ
 *
 * @return			マップオブジェクト
 */

newtRef NewtMakeMap(newtRefArg superMap, uint32_t n, newtRefVar v[])
{
    newtRefVar	r;
    int32_t	flags = 0;

    r = NewtMakeSlotsObj(NewtMakeInteger(flags), n + 1, 0);
    NewtSetArraySlot(r, 0, superMap);

    if (NewtRefIsNotNIL(superMap))
    {
        flags = NewtRefToInteger(NcClassOf(superMap));
        flags &= ~ kNewtMapSorted;
    }

    if (NewtRefIsNotNIL(r) && v != NULL)
    {
//        NewtMakeInitSlots(r, 1, n, 2, v);

        newtRef *	slots;
        uint32_t	i;
    
        slots = NewtRefToSlots(r);
    
        for (i = 1; i <= n; i++)
        {
            slots[i] = *v;

            if (slots[i] == NSSYM0(_proto))
                flags |= kNewtMapProto;

            v += 2;
        }
    }

    NcSetClass(r, NewtMakeInteger(flags));

    return r;
}


/*------------------------------------------------------------------------*/
/** マップにフラグをセットする
 *
 * @param map		[in] マップオブジェクト
 * @param bit		[in] フラグ
 *
 * @return			なし
 */

void NewtSetMapFlags(newtRefArg map, int32_t bit)
{
    int32_t	flags;

    flags = NewtRefToInteger(NcClassOf(map));
    flags |= bit;
    NcSetClass(map, NewtMakeInteger(flags));
}


/*------------------------------------------------------------------------*/
/** マップのフラグをクリアする
 *
 * @param map		[in] マップオブジェクト
 * @param bit		[in] フラグ
 *
 * @return			なし
 */

void NewtClearMapFlags(newtRefArg map, int32_t bit)
{
    int32_t	flags;

    flags = NewtRefToInteger(NcClassOf(map));
    flags &= ~ bit;
    NcSetClass(map, NewtMakeInteger(flags));
}


/*------------------------------------------------------------------------*/
/** マップの長さを取得する
 *
 * @param map		[in] マップオブジェクト
 *
 * @return			長さ
 */

uint32_t NewtMapLength(newtRefArg map)
{
    uint32_t	len = 0;
    newtRefVar	r;

    r = (newtRef)map;

    while (NewtRefIsNotNIL(r))
    {
        len += NewtLength(r) - 1;
        r = NewtGetArraySlot(r, 0);
    }

    return len;
}


/*------------------------------------------------------------------------*/
/** フレームオブジェクトを作成する
 *
 * @param map		[in] マップ
 * @param n			[in] 長さ
 *
 * @return			フレームオブジェクト
 */

newtRef NewtMakeFrame(newtRefArg map, uint32_t n)
{
    newtRefVar	m;

    m = (newtRef)map;

    if (NewtRefIsNIL(m))
        m = NewtMakeMap(kNewtRefNIL, n, NULL);

    return NewtMakeSlotsObj(m, n, kNewtObjFrame);
}


/*------------------------------------------------------------------------*/
/** フレームオブジェクトを作成して初期化する
 *
 * @param n			[in] 長さ
 * @param v			[in] 初期化データ
 *
 * @return			フレームオブジェクト
 */

newtRef NewtMakeFrame2(uint32_t n, newtRefVar v[])
{
    newtRefVar	m;
    newtRefVar	r;

    m = NewtMakeMap(kNewtRefNIL, n, v);
    r = NewtMakeFrame(m, n);

    if (NewtRefIsNotNIL(r))
        NewtMakeInitSlots(r, 0, n, 2, v + 1);

    return r;
}


/*------------------------------------------------------------------------*/
/** スロットオブジェクトを作成する
 *
 * @param r			[in] クラス／マップ
 * @param n			[in] 長さ
 * @param type		[in] タイプ
 *
 * @return			オブジェクト
 */

newtRef NewtMakeSlotsObj(newtRefArg r, uint32_t n, uint16_t type)
{
    newtObjRef	obj;
    uint32_t	size;

    size = sizeof(newtRef) * n;
    obj = NewtObjAlloc(r, size, kNewtObjSlotted | type, false);

    if (obj != NULL)
    {
        newtRef *	slots;
        uint32_t	i;

        slots = NewtObjToSlots(obj);

        for (i = 0; i < n; i++)
        {
            slots[i] = kNewtRefUnbind;
        }

        return NewtMakePointer(obj);
    }

    return kNewtRefNIL;
}


/*------------------------------------------------------------------------*/
/** スロットオブジェクトのオブジェクトデータの長さを取得する
 *
 * @param obj		[in] オブジェクトデータ
 *
 * @return			長さ
 */

uint32_t NewtObjSlotsLength(newtObjRef obj)
{
    return NewtObjSize(obj) / sizeof(newtRef);
}


/*------------------------------------------------------------------------*/
/** スロットオブジェクトのオブジェクトデータの長さを変更する
 *
 * @param obj		[in] オブジェクトデータ
 * @param n			[in] 長さ
 * @param v			[in] 初期化データ
 *
 * @return			長さの変更されたオブジェクトデータ
 */

newtObjRef NewtObjSlotsSetLength(newtObjRef obj, uint32_t n, newtRefArg v)
{
    uint32_t	size;
    uint32_t	len;

    len = NewtObjSlotsLength(obj);
    size = sizeof(newtRef) * n;
    obj = NewtObjResize(obj, size);

    if (obj != NULL)
    {
        newtRef *	slots;
        uint32_t	i;

        slots = NewtObjToSlots(obj);

        for (i = len; i < n; i++)
        {
            slots[i] = v;
        }
    }

    return obj;
}


/*------------------------------------------------------------------------*/
/** スロットオブジェクトのオブジェクトデータに値を追加する
 *
 * @param obj		[in] オブジェクトデータ
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NewtObjAddArraySlot(newtObjRef obj, newtRefArg v)
{
    uint32_t	len;

    len = NewtObjSlotsLength(obj);
    NewtObjSlotsSetLength(obj, len + 1, v);

    return v;
}


/*------------------------------------------------------------------------*/
/** スロットオブジェクトの長さを変更する
 *
 * @param r			[in] オブジェクト
 * @param n			[in] 長さ
 * @param v			[in] 初期化データ
 *
 * @return			長さの変更されたオブジェクト
 */

newtRef NewtSlotsSetLength(newtRefArg r, uint32_t n, newtRefArg v)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(r);
    NewtObjSlotsSetLength(obj, n, v);

    return r;
}


/*------------------------------------------------------------------------*/
/** オブジェクトの長さを変更する
 *
 * @param r			[in] オブジェクト
 * @param n			[in] 長さ
 *
 * @return			長さの変更されたオブジェクト
 */

newtRef NewtSetLength(newtRefArg r, uint32_t n)
{
    uint16_t	type;

    type = NewtGetRefType(r, true);

    switch (type)
    {
        case kNewtBinary:
            NewtBinarySetLength(r, n);
            break;

        case kNewtString:
            NewtStringSetLength(r, n);
            break;

        case kNewtArray:
        case kNewtFrame:
            NewtSlotsSetLength(r, n, kNewtRefUnbind);
            break;
    }

    return r;
}


/*------------------------------------------------------------------------*/
/** アドレスから整数オブジェクトを作成する
 *
 * @param addr		[in] アドレス
 *
 * @return			整数オブジェクト
 */

newtRef NewtMakeAddress(void * addr)
{
	return NewtMakeInteger(((uint32_t)addr) >> NOBJ_ADDR_SHIFT);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** エラー番号の例外を発生する
 *
 * @param err		[in] エラー番号
 *
 * @return			kNewtRefUnbind
 */

newtRef NewtThrow0(int32_t err)
{
	return NewtThrow(err, kNewtRefUnbind);
}


/*------------------------------------------------------------------------*/
/** エラー番号に対応する例外シンボルを作成する
 *
 * @param err		[in] エラー番号
 *
 * @return			例外シンボル
 */

newtRef NewtMakeThrowSymbol(int32_t err)
{
	newtRefVar  symstr;
	int32_t		errbase;

	symstr = NSSTR("evt.ex.fr");
	errbase = (err % 100) * 100;

	switch (errbase)
	{
		case kNErrObjectBase:
			NewtStrCat(symstr, ".obj");
			break;

		case kNErrBadTypeBase:
			NewtStrCat(symstr, ".type");
			break;

		case kNErrCompilerBase:
			NewtStrCat(symstr, ".compr");
			break;

		case kNErrInterpreterBase:
			NewtStrCat(symstr, ".intrp");
			break;

		case kNErrFileBase:
			NewtStrCat(symstr, ".file");
			break;

		case kNErrMiscBase:
			break;
	}

	NewtStrCat(symstr, ";type.ref.frame");

	return NcMakeSymbol(symstr);
}


/*------------------------------------------------------------------------*/
/** エラー番号と値オブジェクトをデータに例外を発生する
 *
 * @param err		[in] エラー番号
 * @param value		[in] 値オブジェクト
 *
 * @return			kNewtRefUnbind
 */

newtRef NewtThrow(int32_t err, newtRefArg value)
{
    newtRefVar	sym;
    newtRefVar	data;

	sym = NewtMakeThrowSymbol(err);

    data = NcMakeFrame();
    NcSetSlot(data, NSSYM0(errorCode), NewtMakeInteger(err));
  NcSetSlot(data, NSSYM(errorMessage), NewtMakeString(NewtErrorMessage(err), false));

	if (value != kNewtRefUnbind)
		NcSetSlot(data, NSSYM0(value), value);

    return NcThrow(sym, data);
}


/*------------------------------------------------------------------------*/
/** エラー番号とシンボルをデータに例外を発生する
 *
 * @param err		[in] エラー番号
 * @param symbol	[in] シンボル
 *
 * @return			kNewtRefUnbind
 */

newtRef NewtThrowSymbol(int32_t err, newtRefArg symbol)
{
    newtRefVar	sym;
    newtRefVar	data;

	sym = NewtMakeThrowSymbol(err);

    data = NcMakeFrame();
    NcSetSlot(data, NSSYM0(errorCode), NewtMakeInteger(err));
  NcSetSlot(data, NSSYM(errorMessage), NewtMakeString(NewtErrorMessage(err), false));

	if (symbol != kNewtRefUnbind)
		NcSetSlot(data, NSSYM0(symbol), symbol);

    return NcThrow(sym, data);
}


/*------------------------------------------------------------------------*/
/** Out Of Bounds エラーを発生する
 *
 * @param value		[in] 値オブジェクト
 * @param index		[in] 位置
 *
 * @return			kNewtRefUnbind
 */

newtRef NewtErrOutOfBounds(newtRefArg value, int32_t index)
{
	newtRefVar  symstr;
    newtRefVar	data;

	symstr = NSSTR("evt.ex.fr");
	NewtStrCat(symstr, ";type.ref.frame");

    data = NcMakeFrame();
    NcSetSlot(data, NSSYM0(errorCode), NewtMakeInteger(kNErrOutOfBounds));
  NcSetSlot(data, NSSYM(errorMessage), NewtMakeString(NewtErrorMessage(kNErrOutOfBounds), false));
	NcSetSlot(data, NSSYM0(value), value);
	NcSetSlot(data, NSSYM0(index), NewtMakeInteger(index));

    return NcThrow(NcMakeSymbol(symstr), data);
}


/*------------------------------------------------------------------------*/
/** エラーメッセージを表示する
 *
 * @param err		[in] エラー番号
 *
 * @return			なし
 */

void NewtErrMessage(int32_t err)
{
    switch (err)
    {
        case kNErrNone:
            break;

        case kNErrObjectReadOnly:
            NewtFprintf(stderr, "*** Object Read Only\n");
            break;
    }
}

const char * NewtErrorMessage(int32_t err) {
  const char *result = NULL;
  
  switch (err) {
    case kNErrObjectPointerOfNonPtr:
      result = "kNErrObjectPointerOfNonPtr";
      break;
    case kNErrBadMagicPointer:
      result = "kNErrBadMagicPointer";
      break;
    case kNErrEmptyPath:
      result = "kNErrEmptyPath";
      break;
    case kNErrBadSegmentInPath:
      result = "kNErrBadSegmentInPath";
      break;
    case kNErrPathFailed:
      result = "kNErrPathFailed";
      break;
    case kNErrOutOfBounds:
      result = "kNErrOutOfBounds";
      break;
    case kNErrObjectsNotDistinct:
      result = "kNErrObjectsNotDistinct";
      break;
    case kNErrLongOutOfRange:
      result = "kNErrLongOutOfRange";
      break;
    case kNErrSettingHeapSizeTwice:
      result = "kNErrSettingHeapSizeTwice";
      break;
    case kNErrGcDuringGc:
      result = "kNErrGcDuringGc";
      break;
    case kNErrBadArgs:
      result = "kNErrBadArgs";
      break;
    case kNErrStringTooBig:
      result = "kNErrStringTooBig";
      break;
    case kNErrTFramesObjectPtrOfNil:
      result = "kNErrTFramesObjectPtrOfNil";
      break;
    case kNErrUnassignedTFramesObjectPtr:
      result = "kNErrUnassignedTFramesObjectPtr";
      break;
    case kNErrObjectReadOnly:
      result = "kNErrObjectReadOnly";
      break;
    case kNErrOutOfObjectMemory:
      result = "kNErrOutOfObjectMemory";
      break;
    case kNErrDerefMagicPointer:
      result = "kNErrDerefMagicPointer";
      break;
    case kNErrNegativeLength:
      result = "kNErrNegativeLength";
      break;
    case kNErrOutOfRange:
      result = "kNErrOutOfRange";
      break;
    case kNErrCouldntResizeLockedObject:
      result = "kNErrCouldntResizeLockedObject";
      break;
    case kNErrBadPackageRef:
      result = "kNErrBadPackageRef";
      break;
    case kNErrBadExceptionName:
      result = "kNErrBadExceptionName";
      break;
    case kNErrNotAFrame:
      result = "kNErrNotAFrame";
      break;
    case kNErrNotAnArray:
      result = "kNErrNotAnArray";
      break;
    case kNErrNotAString:
      result = "kNErrNotAString";
      break;
    case kNErrNotAPointer:
      result = "kNErrNotAPointer";
      break;
    case kNErrNotANumber:
      result = "kNErrNotANumber";
      break;
    case kNErrNotAReal:
      result = "kNErrNotAReal";
      break;
    case kNErrNotAnInteger:
      result = "kNErrNotAnInteger";
      break;
    case kNErrNotACharacter:
      result = "kNErrNotACharacter";
      break;
    case kNErrNotABinaryObject:
      result = "kNErrNotABinaryObject";
      break;
    case kNErrNotAPathExpr:
      result = "kNErrNotAPathExpr";
      break;
    case kNErrNotASymbol:
      result = "kNErrNotASymbol";
      break;
    case kNErrNotAFunction:
      result = "kNErrNotAFunction";
      break;
    case kNErrNotAFrameOrArray:
      result = "kNErrNotAFrameOrArray";
      break;
    case kNErrNotAnArrayOrNil:
      result = "kNErrNotAnArrayOrNil";
      break;
    case kNErrNotAStringOrNil:
      result = "kNErrNotAStringOrNil";
      break;
    case kNErrNotABinaryObjectOrNil:
      result = "kNErrNotABinaryObjectOrNil";
      break;
    case kNErrUnexpectedFrame:
      result = "kNErrUnexpectedFrame";
      break;
    case kNErrUnexpectedBinaryObject:
      result = "kNErrUnexpectedBinaryObject";
      break;
    case kNErrUnexpectedImmediate:
      result = "kNErrUnexpectedImmediate";
      break;
    case kNErrNotAnArrayOrString:
      result = "kNErrNotAnArrayOrString";
      break;
    case kNErrNotAVBO:
      result = "kNErrNotAVBO";
      break;
    case kNErrNotAPackage:
      result = "kNErrNotAPackage";
      break;
    case kNErrNotNil:
      result = "kNErrNotNil";
      break;
    case kNErrNotASymbolOrNil:
      result = "kNErrNotASymbolOrNil";
      break;
    case kNErrNotTrueOrNil:
      result = "kNErrNotTrueOrNil";
      break;
    case kNErrNotAnIntegerOrArray:
      result = "kNErrNotAnIntegerOrArray";
      break;
    case kNErrSyntaxError:
      result = "kNErrSyntaxError";
      break;
    case kNErrAssignToConstant:
      result = "kNErrAssignToConstant";
      break;
    case kNErrCantTest:
      result = "kNErrCantTest";
      break;
    case kNErrGlobalVarNotAllowed:
      result = "kNErrGlobalVarNotAllowed";
      break;
    case kNErrCantHaveSameName:
      result = "kNErrCantHaveSameName";
      break;
    case kNErrCantRedefineConstant:
      result = "kNErrCantRedefineConstant";
      break;
    case kNErrCantHaveSameNameInScope:
      result = "kNErrCantHaveSameNameInScope";
      break;
    case kNErrNonLiteralExpression:
      result = "kNErrNonLiteralExpression";
      break;
    case kNErrEndOfInputString:
      result = "kNErrEndOfInputString";
      break;
    case kNErrOddNumberOfDigits:
      result = "kNErrOddNumberOfDigits";
      break;
    case kNErrNoEscapes:
      result = "kNErrNoEscapes";
      break;
    case kNErrInvalidHexCharacter:
      result = "kNErrInvalidHexCharacter";
      break;
    case kNErrNotTowDigitHex:
      result = "kNErrNotTowDigitHex";
      break;
    case kNErrNotFourDigitHex:
      result = "kNErrNotFourDigitHex";
      break;
    case kNErrIllegalCharacter:
      result = "kNErrIllegalCharacter";
      break;
    case kNErrInvalidHexadecimal:
      result = "kNErrInvalidHexadecimal";
      break;
    case kNErrInvalidReal:
      result = "kNErrInvalidReal";
      break;
    case kNErrInvalidDecimal:
      result = "kNErrInvalidDecimal";
      break;
    case kNErrNotConstant:
      result = "kNErrNotConstant";
      break;
    case kNErrNotDecimalDigit:
      result = "kNErrNotDecimalDigit";
      break;
    case kNErrNotInBreakLoop:
      result = "kNErrNotInBreakLoop";
      break;
    case kNErrTooManyArgs:
      result = "kNErrTooManyArgs";
      break;
    case kNErrWrongNumberOfArgs:
      result = "kNErrWrongNumberOfArgs";
      break;
    case kNErrZeroForLoopIncr:
      result = "kNErrZeroForLoopIncr";
      break;
    case kNErrNoCurrentException:
      result = "kNErrNoCurrentException";
      break;
    case kNErrUndefinedVariable:
      result = "kNErrUndefinedVariable";
      break;
    case kNErrUndefinedGlobalFunction:
      result = "kNErrUndefinedGlobalFunction";
      break;
    case kNErrUndefinedMethod:
      result = "kNErrUndefinedMethod";
      break;
    case kNErrMissingProtoForResend:
      result = "kNErrMissingProtoForResend";
      break;
    case kNErrNilContext:
      result = "kNErrNilContext";
      break;
    case kNErrBadCharForString:
      result = "kNErrBadCharForString";
      break;
    case kNErrInvalidFunc:
      result = "kNErrInvalidFunc";
      break;
    case kNErrInvalidInstruction:
      result = "kNErrInvalidInstruction";
      break;
    case kNErrFileNotFound:
      result = "kNErrFileNotFound";
      break;
    case kNErrFileNotOpen:
      result = "kNErrFileNotOpen";
      break;
    case kNErrDylibNotOpen:
      result = "kNErrDylibNotOpen";
      break;
    case kNErrSystemError:
      result = "kNErrSystemError";
      break;
    case kNErrDiv0:
      result = "kNErrDiv0";
      break;
    case kNErrRegcomp:
      result = "kNErrRegcomp";
      break;
    case kNErrNSOFWrite:
      result = "kNErrNSOFWrite";
      break;
    case kNErrNSOFRead:
      result = "kNErrNSOFRead";
      break;
  }
  
  return result;
}

#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 32bit整数の比較
 *
 * @param r1		[in] 32bit整数１
 * @param r2		[in] 32bit整数２
 *
 * @retval			-1		r1 < r2
 * @retval			0		r1 = r2
 * @retval			1		r1 > r2
 */

int NewtInt32Compare(newtRefArg r1, newtRefArg r2)
{
    int32_t	i1;
    int32_t	i2;

    i1 = NewtRefToInteger(r1);
    i2 = NewtRefToInteger(r2);

    if (i1 < i2)
        return -1;
    else if (i1 > i2)
        return 1;
    else
        return 0;
}


/*------------------------------------------------------------------------*/
/** 浮動小数点の比較
 *
 * @param r1		[in] 浮動小数点１
 * @param r2		[in] 浮動小数点２
 *
 * @retval			-1		r1 < r2
 * @retval			0		r1 = r2
 * @retval			1		r1 > r2
 */

int NewtRealCompare(newtRefArg r1, newtRefArg r2)
{
    double real1;
    double real2;

    real1 = NewtRefToReal(r1);
    real2 = NewtRefToReal(r2);

    if (real1 < real2)
        return -1;
    else if (real1 > real2)
        return 1;
    else
        return 0;
}


/*------------------------------------------------------------------------*/
/** シンボルを字句的に比較（大文字小文字は区別されない）
 *
 * @param r1		[in] シンボル１
 * @param r2		[in] シンボル２
 *
 * @retval			負の整数	r1 < r2
 * @retval			0		r1 = r2
 * @retval			正の整数	r1 > r2
 */

int NewtSymbolCompareLex(newtRefArg r1, newtRefArg r2)
{
    newtSymDataRef	sym1;
    newtSymDataRef	sym2;

	if (r1 == r2)
		return 0;

    sym1 = NewtRefToSymbol(r1);
    sym2 = NewtRefToSymbol(r2);

    return strcasecmp(sym1->name, sym2->name);
}


/*------------------------------------------------------------------------*/
/** 文字列オブジェクトの比較
 *
 * @param r1		[in] 文字列オブジェクト１
 * @param r2		[in] 文字列オブジェクト２
 *
 * @retval			負の整数	r1 < r2
 * @retval			0		r1 = r2
 * @retval			正の整数	r1 > r2
 */

int NewtStringCompare(newtRefArg r1, newtRefArg r2)
{
    char *	s1;
    char *	s2;

    s1 = NewtRefToString(r1);
    s2 = NewtRefToString(r2);

    return strcmp(s1, s2);
}


/*------------------------------------------------------------------------*/
/** バイナリオブジェクトの比較
 *
 * @param r1		[in] バイナリオブジェクト１
 * @param r2		[in] バイナリオブジェクト２
 *
 * @retval			-1		r1 < r2
 * @retval			0		r1 = r2
 * @retval			1		r1 > r2
 */

int NewtBinaryCompare(newtRefArg r1, newtRefArg r2)
{
    int32_t	len1;
    int32_t	len2;
    int32_t	len;
    uint8_t *	d1;
    uint8_t *	d2;
    int		r;

    len1 = NewtBinaryLength(r1);
    len2 = NewtBinaryLength(r2);

    if (len1 == 0 || len2 == 0)
        return (len1 - len2);

    d1 = NewtRefToBinary(r1);
    d2 = NewtRefToBinary(r2);

    if (len1 < len2)
        len = len1;
    else
        len = len2;

    r = memcmp(d1, d2, len);

    if (r == 0)
    {
        if (len1 < len2)
            r = -1;
        else if (len1 > len2)
            r = 1;
    }

    return r;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 計算可能な引数ならば計算結果のオブジェクトタイプを返す
 *
 * @param r1		[in] オブジェクト１
 * @param r2		[in] オブジェクト２
 *
 * @retval			オブジェクトタイプ		計算可能
 * @retval			kNewtUnknownType	計算不可
 */

uint16_t NewtArgsType(newtRefArg r1, newtRefArg r2)
{
    uint16_t	type1;
    uint16_t	type2;

    type1 = NewtGetRefType(r1, true);
    type2 = NewtGetRefType(r2, true);

	if (type1 == type2)
		return type1;

	if (type1 == kNewtInt30)
		type1 = kNewtInt32;

	if (type2 == kNewtInt30)
		type2 = kNewtInt32;

    if (type1 == kNewtInt32 && type2 == kNewtReal)
        type1 = kNewtReal;
    else if (type1 == kNewtReal && type2 == kNewtInt32)
        type2 = kNewtReal;

    if (type1 == type2)
        return type1;
    else
        return kNewtUnknownType;
}


/*------------------------------------------------------------------------*/
/** オブジェクトの大小比較
 *
 * @param r1		[in] オブジェクト１
 * @param r2		[in] オブジェクト２
 *
 * @retval			1		r1 > r2
 * @retval			0		r1 = r2
 * @retval			-1		r1 < r2
 */

int16_t NewtObjectCompare(newtRefArg r1, newtRefArg r2)
{
    int	r = -1;

    switch (NewtArgsType(r1, r2))
    {
        case kNewtInt30:
            if ((int32_t)r1 < (int32_t)r2)
                r = -1;
            else if ((int32_t)r1 > (int32_t)r2)
                r = 1;
            else
                r = 0;
            break;

        case kNewtCharacter:
            if (r1 < r2)
                r = -1;
            else if (r1 > r2)
                r = 1;
            else
                r = 0;
            break;

        case kNewtInt32:
            r = NewtInt32Compare(r1, r2);
            break;

        case kNewtReal:
            r = NewtRealCompare(r1, r2);
            break;

        case kNewtSymbol:
            r = NewtSymbolCompareLex(r1, r2);
            break;

        case kNewtString:
            r = NewtStringCompare(r1, r2);
            break;

        case kNewtBinary:
            r = NewtBinaryCompare(r1, r2);
            break;
    }

    return r;
}


/*------------------------------------------------------------------------*/
/** 参照の比較
 *
 * @param r1		[in] 参照１
 * @param r2		[in] 参照２
 *
 * @retval			true	同値
 * @retval			false   同値でない
 */

bool NewtRefEqual(newtRefArg r1, newtRefArg r2)
{
    int	r = -1;

    if (r1 == r2)
        return true;
    else if (NewtRefIsSymbol(r1))
        return false;

    switch (NewtArgsType(r1, r2))
    {
        case kNewtInt32:
            r = NewtInt32Compare(r1, r2);
            break;

        case kNewtReal:
            r = NewtRealCompare(r1, r2);
            break;

		default:
			return false;
    }

	return (r == 0);
}


/*------------------------------------------------------------------------*/
/** オブジェクトの比較
 *
 * @param r1		[in] オブジェクト１
 * @param r2		[in] オブジェクト２
 *
 * @retval			true	同値
 * @retval			false   同値でない
 */

bool NewtObjectEqual(newtRefArg r1, newtRefArg r2)
{
    if (r1 == r2)
        return true;
    else if (NewtRefIsSymbol(r1))
        return false;
//        return NewtSymbolEqual(r1, r2);
    else
        return (NewtObjectCompare(r1, r2) == 0);
}


/*------------------------------------------------------------------------*/
/** シンボルオブジェクトの比較
 *
 * @param r1		[in] シンボルオブジェクト１
 * @param r2		[in] シンボルオブジェクト２
 *
 * @retval			true	同値
 * @retval			false   同値でない
 */

bool NewtSymbolEqual(newtRefArg r1, newtRefArg r2)
{
    newtSymDataRef	sym1;
    newtSymDataRef	sym2;

    if (r1 == r2)
        return true;

    if (! NewtRefIsSymbol(r1))
        return false;

    if (! NewtRefIsSymbol(r2))
        return false;

    sym1 = NewtRefToSymbol(r1);
    sym2 = NewtRefToSymbol(r2);

    if (sym1->hash == sym2->hash)
        return (strcasecmp(sym1->name, sym2->name) == 0);
    else
        return false;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** オブジェクトの長さを取得する
 *
 * @param r			[in] オブジェクト
 *
 * @return			長さ
 */

uint32_t NewtLength(newtRefArg r)
{
    uint32_t	len = 0;

    switch (NewtGetRefType(r, true))
    {
        case kNewtSymbol:
        case kNewtString:
        case kNewtBinary:
            len = NewtBinaryLength(r);
            break;

        case kNewtArray:
            len = NewtArrayLength(r);
            break;

        case kNewtFrame:
            len = NewtFrameLength(r);
            break;
    }

    return len;
}


/*------------------------------------------------------------------------*/
/** オブジェクトの（深い）長さを取得
 *
 * @param r			[in] オブジェクト
 *
 * @return			オブジェクトの長さ
 *
 * @note			フレームの場合はプロト継承で長さを計算する
 */

uint32_t NewtDeeplyLength(newtRefArg r)
{
    uint32_t	len = 0;

    switch (NewtGetRefType(r, true))
    {
        case kNewtFrame:
            len = NewtDeeplyFrameLength(r);
            break;

        default:
            len = NewtLength(r);
            break;
    }

    return len;
}


/*------------------------------------------------------------------------*/
/** バイナリオブジェクトの長さを取得
 *
 * @param r			[in] オブジェクト
 *
 * @return			オブジェクトの長さ
 */

uint32_t NewtBinaryLength(newtRefArg r)
{
    uint32_t	len = 0;

//    if (NewtIsBinary(r))
    {
        newtObjRef	obj;

        obj = NewtRefToPointer(r);
        len = NewtObjSize(obj);
    }

    return len;
}


/*------------------------------------------------------------------------*/
/** シンボルオブジェクトの長さを取得
 *
 * @param r			[in] オブジェクト
 *
 * @return			オブジェクトの長さ
 */

uint32_t NewtSymbolLength(newtRefArg r)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(r);
    return NewtObjSymbolLength(obj);
}


/*------------------------------------------------------------------------*/
/** 文字列オブジェクトの長さを取得
 *
 * @param r			[in] オブジェクト
 *
 * @return			オブジェクトの長さ
 */

uint32_t NewtStringLength(newtRefArg r)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(r);
    return NewtObjStringLength(obj);
}


/*------------------------------------------------------------------------*/
/** スロットオブジェクトの長さ（スロットの数）を取得
 *
 * @param r			[in] オブジェクト
 *
 * @return			オブジェクトの長さ
 */

uint32_t NewtSlotsLength(newtRefArg r)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(r);
    return NewtObjSlotsLength(obj);
}


/*------------------------------------------------------------------------*/
/** プロト継承でフレームオブジェクトの長さ（スロットの数）を取得
 *
 * @param r			[in] オブジェクト
 *
 * @return			オブジェクトの長さ
 */

uint32_t NewtDeeplyFrameLength(newtRefArg r)
{
    newtRefVar	f;
    uint32_t	total = 0;
    uint32_t	len;

    f = r;

    while (true)
    {
        len = NewtFrameLength(f);
        total += len;

        if (len == 0) break;
        f = NcGetSlot(f, NSSYM0(_proto));
        if (NewtRefIsNIL(f)) break;

        total--;
    }

    return total;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** フレームのオブジェクトデータが _proto スロットを持つかチェックする
 *
 * @param obj		[in] フレームのオブジェクトデータ
 *
 * @retval			true	_proto スロットを持つ
 * @retval			false	_proto スロットを持たない
 */

bool NewtObjHasProto(newtObjRef obj)
{
    int32_t	flags;

    if (NewtRefIsNIL(obj->as.map))
        return false;

    flags = NewtRefToInteger(NcClassOf(obj->as.map));

    return ((flags & kNewtMapProto) != 0);
}


/*------------------------------------------------------------------------*/
/** フレームのオブジェクトデータからスロットの値を取出す
 *
 * @param obj		[in] フレームのオブジェクトデータ
 * @param slot		[in] スロットシンボル
 *
 * @return			値オブジェクト
 */

newtRef NewtObjGetSlot(newtObjRef obj, newtRefArg slot)
{
    uint32_t	i;

	if (! NewtObjIsFrame(obj))
		return kNewtRefUnbind;

    if (slot == NSSYM0(_proto) && ! NewtObjHasProto(obj))
        return kNewtRefUnbind;

    if (NewtFindMapIndex(obj->as.map, slot, &i))
    {
        newtRef *	slots;

        slots = NewtObjToSlots(obj);
        return slots[i];
    }

    return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** マップのソートフラグをチェックする
 *
 * @param r			[in] マップオブジェクト
 *
 * @retval			true	ソートフラグが ON
 * @retval			false   ソートフラグが OFF
 */

bool NewtMapIsSorted(newtRefArg r)
{
    newtRefVar	klass;
    uint32_t	flags;

    klass = NcClassOf(r);
    if (! NewtRefIsInteger(klass)) return false;

    flags = NewtRefToInteger(klass);

    return ((flags & kNewtMapSorted) != 0);
}


/*------------------------------------------------------------------------*/
/** フレームのオブジェクトデータにスロットの値をセットする
 *
 * @param obj		[in] フレームのオブジェクトデータ
 * @param slot		[in] スロットシンボル
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NewtObjSetSlot(newtObjRef obj, newtRefArg slot, newtRefArg v)
{
    uint32_t	i;

/*
    if (NewtObjIsReadonly(obj))
        return NewtThrow0(kNErrObjectReadOnly);
*/

    if (NewtFindMapIndex(obj->as.map, slot, &i))
    {
        newtRef *	slots;

        slots = NewtObjToSlots(obj);
        slots[i] = v;
    }
    else
    {
        uint32_t	len;

        if (NewtRefIsLiteral(obj->as.map))
        {
            newtRefVar	map;

            map = NewtMakeMap(kNewtRefNIL, 1, NULL);

            NewtSetArraySlot(map, 0, obj->as.map);
            NewtSetArraySlot(map, 1, slot);

			if (NewtObjHasProto(obj))
				NewtSetMapFlags(map, kNewtMapProto);

            obj->as.map = map;
        }
        else
        {
            if (NewtMapIsSorted(obj->as.map))
            {
                // マップがソートされている場合...

                newtSymDataRef	sym;
                int32_t	index;
            
                sym = NewtRefToSymbol(slot);
    
                NewtBSearchSymTable(obj->as.map, sym->name, sym->hash, 1, &index);
                NewtInsertArraySlot(obj->as.map, index, slot);
            }
            else
            {
                NcAddArraySlot(obj->as.map, slot);
            }
        }

        len = NewtObjSlotsLength(obj);
        NewtObjSlotsSetLength(obj, len + 1, v);

        if (slot == NSSYM0(_proto))
            NewtSetMapFlags(obj->as.map, kNewtMapProto);
    }

    return v;
}


/*------------------------------------------------------------------------*/
/** 配列のオブジェクトデータから指定位置の要素を削除する
 *
 * @param obj		[in] 配列のオブジェクトデータ
 * @param n			[in] 位置
 *
 * @return			なし
 */

void NewtObjRemoveArraySlot(newtObjRef obj, int32_t n)
{
    newtRef *	slots;
    uint32_t	len;
    uint32_t	i;

    if (NewtObjIsReadonly(obj))
    {
        NewtThrow0(kNErrObjectReadOnly);
        return;
    }

    slots = NewtObjToSlots(obj);
    len = NewtObjSlotsLength(obj);

    for (i = n + 1; i < len; i++)
    {
        slots[i - 1] = slots[i] ;
    }

    NewtObjSlotsSetLength(obj, len - 1, kNewtRefUnbind);
}


/*------------------------------------------------------------------------*/
/** マップを深くコピーする
 *
 * @param dst		[out]コピー先
 * @param pos		[i/o]コピー位置
 * @param src		[in] コピー元
 *
 * @return			なし
 */

void NewtDeeplyCopyMap(newtRef * dst, int32_t * pos, newtRefArg src)
{
    newtRefVar	superMap;
    newtRef *	slots;
    int32_t	len;
    int32_t	p;
    int32_t	i;

    superMap = NewtGetArraySlot(src, 0);
    len = NewtLength(src);

    if (NewtRefIsNotNIL(superMap))
        NewtDeeplyCopyMap(dst, pos, superMap);

    slots = NewtRefToSlots(src);
    p = *pos;

    for (i = 1; i < len; i++, p++)
    {
        dst[p] = slots[i];
    }

    *pos = p;
}


/*------------------------------------------------------------------------*/
/** マップを深くクローン複製する
 *
 * @param map		[in] マップオブジェクト
 * @param len		[in] 長さ
 *
 * @return			クローン複製されたオブジェクト
 */

newtRef NewtDeeplyCloneMap(newtRefArg map, int32_t len)
{
    newtRefVar	newMap;
    int32_t	flags;
    int32_t	i = 1;

    flags = NewtRefToInteger(NcClassOf(map));
    newMap = NewtMakeMap(kNewtRefNIL, len, NULL);
    NcSetClass(newMap, NewtMakeInteger(flags));

    NewtDeeplyCopyMap(NewtRefToSlots(newMap), &i, map);

    return newMap;
}


/*------------------------------------------------------------------------*/
/** フレームのオブジェクトデータからスロットを削除する
 *
 * @param obj		[in] フレームのオブジェクトデータ
 * @param slot		[in] スロットシンボル
 *
 * @return			なし
 */

void NewtObjRemoveFrameSlot(newtObjRef obj, newtRefArg slot)
{
    uint32_t	i;

    if (NewtObjIsReadonly(obj))
    {
        NewtThrow0(kNErrObjectReadOnly);
        return;
    }

    if (NewtFindMapIndex(obj->as.map, slot, &i))
    {
        int32_t	mapIndex;

        mapIndex = NewtFindArrayIndex(obj->as.map, slot, 1);

        if (mapIndex == -1)
        {
            obj->as.map = NewtDeeplyCloneMap(obj->as.map, NewtObjSlotsLength(obj));
            mapIndex = NewtFindArrayIndex(obj->as.map, slot, 1);
        }
        else if (NewtRefIsLiteral(obj->as.map))
        {
            obj->as.map = NcClone(obj->as.map);
        }

        NewtObjRemoveArraySlot(obj, i);
        NewtObjRemoveArraySlot(NewtRefToPointer(obj->as.map), mapIndex);

        if (slot == NSSYM0(_proto))
            NewtClearMapFlags(obj->as.map, kNewtMapProto);
    }
}


/*------------------------------------------------------------------------*/
/** フレームまたは配列のオブジェクトデータからスロットまたは指定位置の要素を削除する
 *
 * @param obj		[in] フレームのオブジェクトデータ
 * @param slot		[in] スロットシンボル／位置
 *
 * @return			なし
 */

void NewtObjRemoveSlot(newtObjRef obj, newtRefArg slot)
{
    if (NewtObjIsFrame(obj))
    {
        NewtObjRemoveFrameSlot(obj, slot);
    }
    else
    {
      if (NewtRefIsInteger(slot) == false) {
        NewtThrow(kNErrNotAnInteger, slot);
        return;
      }

      int32_t	i;
        i = NewtRefToInteger(slot);
        NewtObjRemoveArraySlot(obj, i);
    }
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** マップから指定位置のスロットシンボルを取出す
 *
 * @param r			[in] マップオブジェクト
 * @param index		[in] 位置
 * @param indexP	[i/o]マップ全体からみた現在の開始位置
 *
 * @return			スロットシンボル
 */

newtRef NewtGetMapIndex(newtRefArg r, uint32_t index, uint32_t * indexP)
{
    newtRefVar	superMap;
    newtRefVar	v;
    int32_t	len;
    int32_t	n;

    superMap = NewtGetArraySlot(r, 0);

    if (NewtRefIsNIL(superMap))
    {
        *indexP = 0;
    }
    else
    {
        v = NewtGetMapIndex(superMap, index, indexP);

        if (v != kNewtRefUnbind)
            return v;
    }

    len = NewtArrayLength(r);
    n = index - *indexP;

    if (n < 0)
        return kNewtRefUnbind;

    if (n + 1 < len)
        return NewtGetArraySlot(r, n + 1);

    *indexP += len - 1;

    return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** 配列から値を検索する
 *
 * @param r			[in] 配列
 * @param v			[in] 値オブジェクト
 * @param st		[in] 開始位置
 *
 * @retval			位置		成功
 * @retval			-1		失敗
 */

int32_t NewtFindArrayIndex(newtRefArg r, newtRefArg v, uint16_t st)
{
    uint32_t	len;

    len = NewtArrayLength(r);

    if (st < len)
    {
        newtRef *	slots;

        slots = NewtRefToSlots(r);

        if (NewtMapIsSorted(r))
        {
            // マップがソートされている場合...

            newtSymDataRef	sym;
            int32_t	index;
        
            sym = NewtRefToSymbol(v);

            if (NewtBSearchSymTable(r, sym->name, sym->hash, st, &index))
                return index;
        }
        else
        {
            uint32_t	i;

            for (i = st; i < len; i++)
            {
//                if (NewtRefEqual(slots[i], v))
                if (slots[i] == v)
                    return i;
            }
        }
    }

    return -1;
}


/*------------------------------------------------------------------------*/
/** マップから値を検索する
 *
 * @param r			[in] マップオブジェクト
 * @param v			[in] スロットシンボル
 * @param indexP	[out]位置
 *
 * @retval			true	成功
 * @retval			false	失敗
 */

bool NewtFindMapIndex(newtRefArg r, newtRefArg v, uint32_t * indexP)
{
    newtRefVar	superMap;
    int32_t	i;

    superMap = NewtGetArraySlot(r, 0);

    if (NewtRefIsNIL(superMap))
    {
        *indexP = 0;
    }
    else
    {
        if (NewtFindMapIndex(superMap, v, indexP))
            return true;
    }

    i = NewtFindArrayIndex(r, v, 1);

    if (0 <= i)
    {
        *indexP += i - 1;
        return true;
    }

    *indexP += NewtArrayLength(r) - 1;

    return false;
}


/*------------------------------------------------------------------------*/
/** フレームオブジェクトのマップを取得
 *
 * @param r			[in] フレーム
 *
 * @return			マップ
 */

newtRef NewtFrameMap(newtRefArg r)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(r);

    if (obj != NULL)
		return obj->as.map;
    else
        return kNewtRefNIL;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** フレームオブジェクトからスロットの位置を探す
 *
 * @param frame		[in] フレーム
 * @param slot		[in] スロットシンボル
 *
 * @retval			スロットの位置		みつかった場合
 * @retval			-1				みつからなかった場合
 */

int32_t NewtFindSlotIndex(newtRefArg frame, newtRefArg slot)
{
    newtRefVar	map;
    uint32_t	i;

    map = NewtFrameMap(frame);

    if (NewtRefIsNIL(map))
        return -1;
    else if (NewtFindMapIndex(map, slot, &i))
        return i;
    else
        return -1;
}


/*------------------------------------------------------------------------*/
/** フレームオブジェクトが _proto スロットを持つかチェックする
 *
 * @param frame		[in] フレーム
 *
 * @retval			true	_proto スロットを持つ
 * @retval			false	_proto スロットを持たない
 */

bool NewtHasProto(newtRefArg frame)
{
    newtObjRef obj;

    obj = NewtRefToPointer(frame);

    return NewtObjHasProto(obj);
}


/*------------------------------------------------------------------------*/
/** フレーム内のスロットの有無を調べる
 *
 * @param frame		[in] フレーム
 * @param slot		[in] スロットシンボル
 *
 * @retval			true	スロットが存在する
 * @retval			false	スロットが存在しない
 */

bool NewtHasSlot(newtRefArg frame, newtRefArg slot)
{
    newtRefVar	map;
    uint32_t	i;

    map = NewtFrameMap(frame);

    if (NewtRefIsNIL(map))
        return false;
    else if (slot == NSSYM0(_proto))
        return NewtHasProto(frame);
    else
        return NewtFindMapIndex(map, slot, &i);
}


/*------------------------------------------------------------------------*/
/** スロットオブジェクトのアクセスパスから値を取得する
 *
 * @param r			[in] オブジェクト
 * @param p			[in] アクセスパス
 *
 * @return			値オブジェクト
 */

newtRef NewtSlotsGetPath(newtRefArg r, newtRefArg p)
{
    if (NewtRefIsArray(r))
        return NewtGetArraySlot(r, p);
    else
        return NcFullLookup(r, p);
}


/*------------------------------------------------------------------------*/
/** オブジェクト内のアクセスパスの有無を調べる
 *
 * @param r			[in] オブジェクト
 * @param p			[in] アクセスパス
 *
 * @retval			true	アクセスパスが存在する
 * @retval			false	アクセスパスが存在しない
 */

bool NewtHasPath(newtRefArg r, newtRefArg p)
{
    return (NcGetPath(r, p) != kNewtRefUnbind);
}


/*------------------------------------------------------------------------*/
/** オブジェクトのアクセスパスの値を取得
 *
 * @param r			[in] オブジェクト
 * @param p			[in] アクセスパス
 * @param slotP		[out]スロット
 *
 * @return			値オブジェクト
 */

newtRef NewtGetPath(newtRefArg r, newtRefArg p, newtRefVar * slotP)
{
    newtRefVar	v;

    v = r;

    if (NcClassOf(p) == NSSYM0(pathExpr))
//    if (NewtRefEqual(NcClassOf(p), NSSYM0(pathExpr)))
    {
        newtRefVar	path;
        int32_t	len;
        int32_t	i;

        len = NewtArrayLength(p);

        if (slotP != NULL)
            len--;

        for (i = 0; i < len; i++)
        {
            path = NewtGetArraySlot(p, i);
            v = NewtSlotsGetPath(v, path);

            if (v == kNewtRefUnbind)
                break;
        }

        if (slotP != NULL)
            *slotP = NewtGetArraySlot(p, len);
    }
    else
    {
        if (slotP != NULL)
            *slotP = p;
        else
            v = NewtSlotsGetPath(r, p);
    }

    return v;
}


/*------------------------------------------------------------------------*/
/** バイナリオブジェクトの指定位置から値を取得する
 *
 * @param r			[in] バイナリオブジェクト
 * @param p			[in] 位置
 *
 * @return			値オブジェクト
 */

newtRef NewtGetBinarySlot(newtRefArg r, uint32_t p)
{
    uint32_t	len;

    len = NewtBinaryLength(r);

    if (p < len)
    {
        uint8_t *	data;
    
        data = NewtRefToBinary(r);
        return NewtMakeInteger(data[p]);
    }

    return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** バイナリオブジェクトの指定位置に値をセットする
 *
 * @param r			[in] バイナリオブジェクト
 * @param p			[in] 位置
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NewtSetBinarySlot(newtRefArg r, uint32_t p, newtRefArg v)
{
    uint32_t	len;

    if (NewtRefIsReadonly(r))
        return NewtThrow(kNErrObjectReadOnly, r);

    len = NewtBinaryLength(r);

    if (p < len)
    {
        uint8_t *	data;
        int32_t	n;

        if (! NewtRefIsInteger(v))
            return NewtThrow(kNErrNotAnInteger, v);

        n = NewtRefToInteger(v);
        data = NewtRefToBinary(r);
        data[p] = n;
    }
    else
    {
        NewtErrOutOfBounds(r, p);
    }

    return v;
}


/*------------------------------------------------------------------------*/
/** 文字列の指定位置から文字を取得
 *
 * @param r			[in] 文字オブジェクト
 * @param p			[in] 位置
 *
 * @return			文字オブジェクト
 */

newtRef NewtGetStringSlot(newtRefArg r, uint32_t p)
{
    uint32_t	len;

    len = NewtStringLength(r);

    if (p < len)
    {
        char *	str;
    
        str = NewtRefToString(r);
        return NewtMakeCharacter(str[p]);
    }

    return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** 文字列の指定位置に文字をセットする
 *
 * @param r			[in] 文字オブジェクト
 * @param p			[in] 位置
 * @param v			[in] 文字オブジェクト
 *
 * @return			文字オブジェクト
 */

newtRef NewtSetStringSlot(newtRefArg r, uint32_t p, newtRefArg v)
{
    uint32_t	slen;
    uint32_t	len;

    if (NewtRefIsReadonly(r))
        return NewtThrow(kNErrObjectReadOnly, r);

    slen = NewtStringLength(r);
	len = NewtBinaryLength(r);

    if (p + 1 < len)
    {
        char *	str;
        int		c;

        if (! NewtRefIsCharacter(v))
            return NewtThrow(kNErrNotACharacter, v);

        c = NewtRefToCharacter(v);
        str = NewtRefToString(r);
        str[p] = c;

		if (slen <= p)
		{	// 文字列が延びたので終端文字をセット
			str[p + 1] = '\0';
		}
    }
    else
    {
        NewtErrOutOfBounds(r, p);
    }

    return v;
}


/*------------------------------------------------------------------------*/
/** スロットオブジェクトの指定位置トから値を取得
 *
 * @param r			[in] オブジェクト
 * @param p			[in] 位置
 *
 * @return			値オブジェクト
 */

newtRef NewtSlotsGetSlot(newtRefArg r, uint32_t p)
{
    uint32_t	len;

    len = NewtSlotsLength(r);

    if (p < len)
    {
        newtRef *	slots;
    
        slots = NewtRefToSlots(r);
        return slots[p];
    }

    return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** スロットオブジェクトにの指定位置に値をセットする
 *
 * @param r			[in] オブジェクト
 * @param p			[in] 位置
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NewtSlotsSetSlot(newtRefArg r, uint32_t p, newtRefArg v)
{
    uint32_t	len;

    len = NewtSlotsLength(r);

    if (p < len)
    {
        newtRef *	slots;
    
        slots = NewtRefToSlots(r);
        NewtGCHint(r[p], -1);
        slots[p] = v;
    }
    else
    {
        NewtErrOutOfBounds(r, p);
    }

    return v;
}


/*------------------------------------------------------------------------*/
/** スロットオブジェクトに値を挿入する
 *
 * @param r			[in] オブジェクト
 * @param p			[in] 位置
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NewtSlotsInsertSlot(newtRefArg r, uint32_t p, newtRefArg v)
{
    newtRef *	slots;
    newtObjRef	obj;
    uint32_t	len;

    obj = NewtRefToPointer(r);
    len = NewtObjSlotsLength(obj);
    NewtObjSlotsSetLength(obj, len + 1, kNewtRefUnbind);

    slots = NewtRefToSlots(r);

    if (len < p)
        p = len;

    if (0 < len - p)
        memmove(slots + p + 1, slots + p, (len - p) * sizeof(newtRef));

    slots[p] = v;

    return v;
}


/*------------------------------------------------------------------------*/
/** 配列の指定位置から値を取得する
 *
 * @param r			[in] 配列オブジェクト
 * @param p			[in] 位置
 *
 * @return			値オブジェクト
 */

newtRef NewtGetArraySlot(newtRefArg r, uint32_t p)
{
    return NewtSlotsGetSlot(r, p);
}


/*------------------------------------------------------------------------*/
/** 配列の指定位置に値をセットする
 *
 * @param r			[in] 配列オブジェクト
 * @param p			[in] 位置
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NewtSetArraySlot(newtRefArg r, uint32_t p, newtRefArg v)
{
    return NewtSlotsSetSlot(r, p, v);
}


/*------------------------------------------------------------------------*/
/** 配列の指定位置に値を挿入する
 *
 * @param r			[in] 配列オブジェクト
 * @param p			[in] 位置
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NewtInsertArraySlot(newtRefArg r, uint32_t p, newtRefArg v)
{
    return NewtSlotsInsertSlot(r, p, v);
}


/*------------------------------------------------------------------------*/
/** フレームのスロットから値を取得する
 *
 * @param r			[in] フレーム
 * @param p			[in] スロットシンボル
 *
 * @return			値オブジェクト
 */

newtRef NewtGetFrameSlot(newtRefArg r, uint32_t p)
{
    return NewtSlotsGetSlot(r, p);
}


/*------------------------------------------------------------------------*/
/** フレームのスロットに値をセットする
 *
 * @param r			[in] フレーム
 * @param p			[in] スロットシンボル
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NewtSetFrameSlot(newtRefArg r, uint32_t p, newtRefArg v)
{
    return NewtSlotsSetSlot(r, p, v);
}


/**
 * Return the slot key for a given index.
 * This method can be used with/like NewtGetFrameSlot to iterate on the slots
 * (until we get optimized FOREACH/FOREACH_WITH_TAG/END_FOREACH macros).
 *
 * @param inFrame		frame to access the slot from
 * @param inIndex		index of the slot to return the key of
 * @return the key of the slot or unbind if there isn't that many slots.
 */

newtRef NewtGetFrameKey(newtRefArg inFrame, uint32_t inIndex)
{
	uint32_t start = 0;
	return NewtGetMapIndex(NewtFrameMap(inFrame), inIndex, &start);
}


/*------------------------------------------------------------------------*/
/** オブジェクトの指定された位置から値を取得
 *
 * @param r			[in] オブジェクト
 * @param p			[in] 位置
 *
 * @return			値オブジェクト
 */

newtRef NewtARef(newtRefArg r, uint32_t p)
{
    newtRefVar	v = kNewtRefNIL;

    switch (NewtGetRefType(r, true))
    {
        case kNewtArray:
            v = NewtGetArraySlot(r, p);
            break;

        case kNewtString:
            v = NewtGetStringSlot(r, p);
            break;

        case kNewtBinary:
            v = NewtGetBinarySlot(r, p);
            break;
    }

    return v;
}


/*------------------------------------------------------------------------*/
/** オブジェクトの指定された位置に値をセットする
 *
 * @param r			[in] オブジェクト
 * @param p			[in] 位置
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NewtSetARef(newtRefArg r, uint32_t p, newtRefArg v)
{
    newtRefVar	result = kNewtRefUnbind;

    switch (NewtGetRefType(r, true))
    {
        case kNewtArray:
            result = NewtSetArraySlot(r, p, v);
            break;

        case kNewtString:
            result = NewtSetStringSlot(r, p, v);
            break;

        case kNewtBinary:
            result = NewtSetBinarySlot(r, p, v);
            break;
    }

    return result;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 検索された変数の保存場所に値をセットする
 *
 * @param start		[in] 開始オブジェクト
 * @param name		[in] 変数名シンボル
 * @param value		[in] 値オブジェクト
 *
 * @retval			true	値がセットできた
 * @retval			false	値がセットできなかった
 */

bool NewtAssignment(newtRefArg start, newtRefArg name, newtRefArg value)
{
    newtRefVar	current;
    newtRefVar	left = start;

    while (NewtRefIsNotNIL(left))
    {
        current = left;

        while (NewtRefIsNotNIL(current))
        {
			current = NcResolveMagicPointer(current);

			if (NewtRefIsMagicPointer(current))
				return kNewtRefUnbind;

            if (NewtHasSlot(current, name))
            {
                NcSetSlot(left, name, value);
                return true;
            }
     
            current = NcGetSlot(current, NSSYM0(_proto));
        }

        left = NcGetSlot(left, NSSYM0(_parent));
    }

    return false;
}


/*------------------------------------------------------------------------*/
/** レキシカルスコープで検索された変数の保存場所に値をセットする
 *
 * @param start		[in] 開始オブジェクト
 * @param name		[in] 変数名シンボル
 * @param value		[in] 値オブジェクト
 *
 * @retval			true	値がセットできた
 * @retval			false	値がセットできなかった
 */

bool NewtLexicalAssignment(newtRefArg start, newtRefArg name, newtRefArg value)
{
    newtRefVar	current = start;

    while (NewtRefIsNotNIL(current))
    {
		current = NcResolveMagicPointer(current);

		if (NewtRefIsMagicPointer(current))
			return kNewtRefUnbind;

        if (NewtHasSlot(current, name))
        {
            NcSetSlot(current, name, value);
            return true;
        }

        current = NcGetSlot(current, NSSYM0(_nextArgFrame));
    }

    return false;
}


/*------------------------------------------------------------------------*/
/** レキシカルスコープで変数の存在を調べる
 *
 * @param start		[in] 開始オブジェクト
 * @param name		[in] 変数名シンボル
 *
 * @retval			true	変数がある
 * @retval			false	変数がない
 */

bool NewtHasLexical(newtRefArg start, newtRefArg name)
{
    newtRefVar	current = start;

    while (NewtRefIsNotNIL(current))
    {
		current = NcResolveMagicPointer(current);

		if (NewtRefIsMagicPointer(current))
			return false;

        if (NewtHasSlot(current, name))
            return true;

        current = NcGetSlot(current, NSSYM0(_nextArgFrame));
    }

    return false;
}


/*------------------------------------------------------------------------*/
/** プロト・ペアレント継承でフレーム内のスロットの有無を調べる
 *
 * @param r			[in] フレーム
 * @param name		[in] スロットシンボル
 *
 * @retval			true	スロットが存在する
 * @retval			false	スロットが存在しない
 */

bool NewtHasVariable(newtRefArg r, newtRefArg name)
{
    newtRefVar	current;
    newtRefVar	left = r;

    while (NewtRefIsNotNIL(left))
    {
        current = left;

        while (NewtRefIsNotNIL(current))
        {
			if (NewtRefIsMagicPointer(current))
				return false;

            if (NewtHasSlot(current, name))
                return true;
    
            current = NcGetSlot(current, NSSYM0(_proto));
        }

        left = NcGetSlot(left, NSSYM0(_parent));
    }

    return false;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** ネイティブ関数の関数オブジェクトから関数のポインタを取得する
 *
 * @param r			[in] 関数オブジェクト
 *
 * @return			関数のポインタ
 */

void * NewtRefToNativeFn(newtRefArg r)
{
    newtRefVar	fn;

    fn = NcGetSlot(r, NSSYM0(funcPtr));

    if (NewtRefIsInteger(fn))
        return (void *)NewtRefToInteger(fn);
    else
        return NULL;
}


/*------------------------------------------------------------------------*/
/** ネイティブ関数（rcvrなし）の関数オブジェクトを作成する
 *
 * @param funcPtr		[in] 関数のポインタ
 * @param numArgs		[in] 引数の数
 * @param indefinite	[in] 不定長フラグ
 * @param doc			[in] 説明文
 *
 * @return				関数オブジェクト
 */

newtRef NewtMakeNativeFn0(void * funcPtr, uint32_t numArgs, bool indefinite, char * doc)
{
    newtRefVar	fnv[] = {
                            NS_CLASS,			NSSYM0(_function.native0),
                            NSSYM0(funcPtr),	kNewtRefNIL,
                            NSSYM0(numArgs),	kNewtRefNIL,
                            NSSYM0(indefinite),	kNewtRefNIL,
                            NSSYM0(docString),	kNewtRefNIL,
                        };

    newtRefVar	fn;

    // function
    fn = NewtMakeFrame2(sizeof(fnv) / (sizeof(newtRefVar) * 2), fnv);

    NcSetSlot(fn, NSSYM0(funcPtr), NewtMakeAddress(funcPtr));
    NcSetSlot(fn, NSSYM0(numArgs), NewtMakeInteger(numArgs));
    NcSetSlot(fn, NSSYM0(indefinite), NewtMakeBoolean(indefinite));
    NcSetSlot(fn, NSSYM0(docString), NSSTRCONST(doc));

    return fn;
}


/*------------------------------------------------------------------------*/
/** ネイティブ関数（rcvrなし）のグローバル関数を登録する
 *
 * @param sym			[in] グローバル関数名
 * @param funcPtr		[in] 関数のポインタ
 * @param numArgs		[in] 引数の数
 * @param indefinite	[in] 不定長フラグ
 * @param doc			[in] 説明文
 *
 * @return				関数オブジェクト
 */

newtRef NewtDefGlobalFn0(newtRefArg sym, void * funcPtr, uint32_t numArgs, bool indefinite, char * doc)
{
    newtRefVar	fn;

    fn = NewtMakeNativeFn0(funcPtr, numArgs, indefinite, doc);
    return NcDefGlobalFn(sym, fn);
}


/*------------------------------------------------------------------------*/
/** ネイティブ関数（rcvrあり）の関数オブジェクトを作成する
 *
 * @param funcPtr		[in] 関数のポインタ
 * @param numArgs		[in] 引数の数
 * @param indefinite	[in] 不定長フラグ
 * @param doc			[in] 説明文
 *
 * @return				関数オブジェクト
 */

newtRef NewtMakeNativeFunc0(void * funcPtr, uint32_t numArgs, bool indefinite, char * doc)
{
    newtRefVar	fnv[] = {
                            NS_CLASS,			NSSYM0(_function.native),
                            NSSYM0(funcPtr),	kNewtRefNIL,
                            NSSYM0(numArgs),	kNewtRefNIL,
                            NSSYM0(indefinite),	kNewtRefNIL,
                            NSSYM0(docString),	kNewtRefNIL,
                        };

    newtRefVar	fn;

    // function
    fn = NewtMakeFrame2(sizeof(fnv) / (sizeof(newtRefVar) * 2), fnv);

    NcSetSlot(fn, NSSYM0(funcPtr), NewtMakeAddress(funcPtr));
    NcSetSlot(fn, NSSYM0(numArgs), NewtMakeInteger(numArgs));
    NcSetSlot(fn, NSSYM0(indefinite), NewtMakeBoolean(indefinite));
    NcSetSlot(fn, NSSYM0(docString), NSSTRCONST(doc));

    return fn;
}


/*------------------------------------------------------------------------*/
/** ネイティブ関数（rcvrあり）のグローバル関数を登録する
 *
 * @param sym			[in] グローバル関数名
 * @param funcPtr		[in] 関数のポインタ
 * @param numArgs		[in] 引数の数
 * @param indefinite	[in] 不定長フラグ
 * @param doc			[in] 説明文
 *
 * @return				関数オブジェクト
 */

newtRef NewtDefGlobalFunc0(newtRefArg sym, void * funcPtr, uint32_t numArgs, bool indefinite, char * doc)
{
    newtRefVar	fn;

    fn = NewtMakeNativeFunc0(funcPtr, numArgs, indefinite, doc);
    return NcDefGlobalFn(sym, fn);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 文字列の前半部が部分文字列と一致するかチェックする
 *
 * @param str		[in] 文字列
 * @param len		[in] 文字列の長さ
 * @param sub		[in] 部分文字列
 * @param sublen	[in] 部分文字列の長さ
 *
 * @retval			true	前半部が部分文字列と一致する
 * @retval			false	前半部が部分文字列と一致しない
 */

bool NewtStrNBeginsWith(char * str, uint32_t len, char * sub, uint32_t sublen)
{
	if (len < sublen)
		return false;
	else
		return (strncasecmp(str, sub, sublen) == 0);
}


/*------------------------------------------------------------------------*/
/** sub が supr のサブクラスをかチェックする
 *
 * @param sub		[in] サブクラス文字列
 * @param sublen	[in] サブスーパクラス文字列の長さ
 * @param supr		[in] スーパクラス文字列
 * @param suprlen	[in] スーパクラス文字列の長さ
 *
 * @retval			true	サブクラス
 * @retval			false	サブクラスでない
 */

bool NewtStrIsSubclass(char * sub, uint32_t sublen, char * supr, uint32_t suprlen)
{
    if (sublen == suprlen)
        return (strncasecmp(sub, supr, suprlen) == 0);

    if (sublen < suprlen)
        return false;

    if (sub[suprlen] != '.')
        return false;

    return NewtStrNBeginsWith(sub, sublen, supr, suprlen);
}


/*------------------------------------------------------------------------*/
/** sub が supr のサブクラスを含むかチェックする
 *
 * @param sub		[in] サブクラス文字列
 * @param sublen	[in] サブスーパクラス文字列の長さ
 * @param supr		[in] スーパクラス文字列
 * @param suprlen	[in] スーパクラス文字列の長さ
 *
 * @retval			true	サブクラスを含む
 * @retval			false	サブクラスを含まない
 */

bool NewtStrHasSubclass(char * sub, uint32_t sublen, char * supr, uint32_t suprlen)
{
    char *	last;
    char *	w;

    last = sub + sublen;

    do {
        w = strchr(sub, ';');
        if (w == NULL) break;

        if (NewtStrIsSubclass(sub, w - sub, supr, suprlen))
            return true;

        sub = w + 1;
        sublen = last - sub;
    } while (true);

    return NewtStrIsSubclass(sub, sublen, supr, suprlen);
}


/*------------------------------------------------------------------------*/
/** sub が supr のサブクラスを含むかチェックする
 *
 * @param sub		[in] サブクラス
 * @param supr		[in] スーパクラス
 *
 * @retval			true	サブクラスを含む
 * @retval			false	サブクラスを含まない
 */

bool NewtHasSubclass(newtRefArg sub, newtRefArg supr)
{
    newtSymDataRef	subSym;
    newtSymDataRef	suprSym;

    if (! NewtRefIsSymbol(sub)) return false;
    if (! NewtRefIsSymbol(supr)) return false;
    if (sub == supr) return true;

    subSym = NewtRefToSymbol(sub);
    suprSym = NewtRefToSymbol(supr);

    return NewtStrHasSubclass(subSym->name, NewtSymbolLength(sub),
                suprSym->name, NewtSymbolLength(supr));
}


/*------------------------------------------------------------------------*/
/** sub が supr のサブクラスかチェックする
 *
 * @param sub		[in] シンボルオブジェクト１
 * @param supr		[in] シンボルオブジェクト２
 *
 * @retval			true	サブクラス
 * @retval			false	サブクラスでない
 */

bool NewtIsSubclass(newtRefArg sub, newtRefArg supr)
{
    newtSymDataRef	subSym;
    newtSymDataRef	suprSym;

    if (! NewtRefIsSymbol(sub)) return false;
    if (! NewtRefIsSymbol(supr)) return false;
    if (sub == supr) return true;

    subSym = NewtRefToSymbol(sub);
    suprSym = NewtRefToSymbol(supr);

    return NewtStrIsSubclass(subSym->name, NewtSymbolLength(sub),
                suprSym->name, NewtSymbolLength(supr));
}


/*------------------------------------------------------------------------*/
/** obj が r のインスタンスかチェックする
 *
 * @param obj		[in] オブジェクト
 * @param r			[in] クラスシンボル
 *
 * @retval			true	インスタンス
 * @retval			false	インスタンスでない
 */

bool NewtIsInstance(newtRefArg obj, newtRefArg r)
{
    return NewtIsSubclass(NcClassOf(obj), r);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 文字列オブジェクトの最後に文字列を追加する
 *
 * @param r			[in] 文字列オブジェクト
 * @param s			[in] 追加する文字列
 *
 * @return			文字列オブジェクト
 */

newtRef NewtStrCat(newtRefArg r, char * s)
{
    if (NewtRefIsPointer(r))
		return NewtStrCat2(r, s, strlen(s));
	else
		return r;
}


/*------------------------------------------------------------------------*/
/** 文字列オブジェクトの最後に指定された長さの文字列を追加する
 *
 * @param r			[in] 文字列オブジェクト
 * @param s			[in] 追加する文字列
 * @param slen		[in] 追加する文字列の長さ
 *
 * @return			文字列オブジェクト
 */

newtRef NewtStrCat2(newtRefArg r, char * s, uint32_t slen)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(r);

    if (obj != NULL)
    {
        uint32_t	tgtlen;
        uint32_t	dstlen;

		tgtlen = NewtObjStringLength(obj);
		dstlen = tgtlen + slen;

		if (NewtObjSize(obj) <= dstlen)
			obj = NewtObjStringSetLength(obj, dstlen);

        if (obj != NULL)
        {
            char *	data;

            data = NewtObjToString(obj);
            memcpy(data + tgtlen, s, slen);
			data[dstlen] = '\0';
        }
    }

    return r;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 環境変数の取得
 *
 * @param s			[in] 文字列
 *
 * @return			文字列オブジェクト
 */

newtRef NewtGetEnv(const char * s)
{
	char *  v;

	v = getenv(s);

	if (v != NULL)
		return NSSTRCONST(v);
	else
		return kNewtRefUnbind;
}
