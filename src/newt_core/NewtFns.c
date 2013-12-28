/*------------------------------------------------------------------------*/
/**
 * @file	NewtFns.c
 * @brief   組込み関数
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <string.h>

#include "NewtErrs.h"
#include "NewtFns.h"
#include "NewtEnv.h"
#include "NewtObj.h"
#include "NewtVM.h"
#include "NewtBC.h"
#include "NewtPrint.h"


/* 関数プロトタイプ */
static newtRef		NewtRefTypeToClass(uint16_t type);
static newtRef		NewtObjClassOf(newtRefArg r);
static newtRef		NewtObjSetClass(newtRefArg r, newtRefArg c);
static bool			NewtArgsIsNumber(newtRefArg r1, newtRefArg r2, bool * real);


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** プロト継承でシンボルを検索（フレームを見つける）
 *
 * @param start		[in] 開始オブジェクト
 * @param name		[in] シンボルオブジェクト
 *
 * @return			検索されたオブジェクトを持つフレーム
 */

newtRef NcProtoLookupFrame(newtRefArg start, newtRefArg name)
{
    newtRefVar	current = start;

    while (NewtRefIsNotNIL(current))
    {
		current = NcResolveMagicPointer(current);

		if (NewtRefIsMagicPointer(current))
			return kNewtRefUnbind;

        if (NewtHasSlot(current, name))
            return current;

        current = NcGetSlot(current, NSSYM0(_proto));
    }

    return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** プロト継承でシンボルを検索
 *
 * @param start		[in] 開始オブジェクト
 * @param name		[in] シンボルオブジェクト
 *
 * @return			検索されたオブジェクト
 */

newtRef NcProtoLookup(newtRefArg start, newtRefArg name)
{
    newtRefVar	current;

	current = NcProtoLookupFrame(start, name);

	if (current != kNewtRefUnbind)
		return NcGetSlot(current, name);
	else
		return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** レキシカルスコープでシンボルを検索
 *
 * @param start		[in] 開始オブジェクト
 * @param name		[in] シンボルオブジェクト
 *
 * @return			検索されたオブジェクト
 */

newtRef NcLexicalLookup(newtRefArg start, newtRef name)
{
    newtRefVar	current = start;

    while (NewtRefIsNotNIL(current))
    {
		current = NcResolveMagicPointer(current);

		if (NewtRefIsMagicPointer(current))
			return kNewtRefUnbind;

        if (NewtHasSlot(current, name))
            return NcGetSlot(current, name);

        current = NcGetSlot(current, NSSYM0(_nextArgFrame));
    }

    return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** プロト、ペアレント継承でシンボルを検索（フレームを見つける）
 *
 * @param start		[in] 開始オブジェクト
 * @param name		[in] シンボルオブジェクト
 *
 * @return			検索されたオブジェクトを持つフレーム
 */

newtRef NcFullLookupFrame(newtRefArg start, newtRefArg name)
{
    newtRefVar	current;
    newtRefVar	left = start;

	if (! NewtRefIsFrame(start))
		return kNewtRefUnbind;

    while (NewtRefIsNotNIL(left))
    {
        current = left;

        while (NewtRefIsNotNIL(current))
        {
			current = NcResolveMagicPointer(current);

			if (NewtRefIsMagicPointer(current))
				return kNewtRefUnbind;

            if (NewtHasSlot(current, name))
                return current;
    
            current = NcGetSlot(current, NSSYM0(_proto));
        }

        left = NcGetSlot(left, NSSYM0(_parent));
    }

    return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** プロト、ペアレント継承でシンボルを検索
 *
 * @param start		[in] 開始オブジェクト
 * @param name		[in] シンボルオブジェクト
 *
 * @return			検索されたオブジェクト
 */

newtRef NcFullLookup(newtRefArg start, newtRefArg name)
{
    newtRefVar	current;

	current = NcFullLookupFrame(start, name);

	if (current != kNewtRefUnbind)
		return NcGetSlot(current, name);
	else
		return kNewtRefUnbind;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** シンボルテーブルからシンボルを検索
 *
 * @param r			[in] シンボルテーブル
 * @param name		[in] シンボル名
 *
 * @return			検索されたシンボルオブジェクト
 */

newtRef NcLookupSymbol(newtRefArg r, newtRefArg name)
{
    return NewtLookupSymbolArray(r, name, 0);
}


/*------------------------------------------------------------------------*/
/** 例外を発生
 *
 * @param rcvr		[in] レシーバ
 * @param name		[in] 例外シンボル
 * @param data		[in] 例外データ
 *
 * @retval			NIL
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsThrow(newtRefArg rcvr, newtRefArg name, newtRefArg data)
{
    NVMThrow(name, data);
    return kNewtRefNIL;
}


/*------------------------------------------------------------------------*/
/** rethrow する
 *
 * @param rcvr		[in] レシーバ
 *
 * @retval			NIL
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsRethrow(newtRefArg rcvr)
{
    NVMRethrow();
    return kNewtRefNIL;
}


/*------------------------------------------------------------------------*/
/** オブジェクトをクローン複製する
 *
 * @param r			[in] オブジェクト
 *
 * @return			クローン複製されたオブジェクト
 */

newtRef NcClone(newtRefArg r)
{
    if (NewtRefIsPointer(r))
        return NewtObjClone(r);
    else
        return (newtRef)r;
}


/*------------------------------------------------------------------------*/
/** オブジェクトの深いクローン複製をする（マジックポインタは追跡しない）
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @return			クローン複製されたオブジェクト
 */

newtRef NsTotalClone(newtRefArg rcvr, newtRefArg r)
{
	newtRefVar	result;

	result = NcClone(r);

	if (NewtRefIsFrameOrArray(result))
	{
        newtRef *	slots;
        uint32_t	n;
        uint32_t	i;
    
        slots = NewtRefToSlots(result);
		n = NewtLength(result);
    
        for (i = 0; i < n; i++)
        {
            slots[i] = NsTotalClone(rcvr, slots[i]);
        }
	}

	return result;
}


/*------------------------------------------------------------------------*/
/** オブジェクトの長さを取得
 *
 * @param r			[in] オブジェクト
 *
 * @return			オブジェクトの長さ
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NcLength(newtRefArg r)
{
    return NewtMakeInteger(NewtLength(r));
}


/*------------------------------------------------------------------------*/
/** オブジェクトの（深い）長さを取得
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @return			オブジェクトの長さ
 *
 * @note			フレームの場合はプロト継承で長さを計算する
 *					スクリプトからの呼出し用
 */

newtRef NsDeeplyLength(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeInteger(NewtDeeplyLength(r));
}


/*------------------------------------------------------------------------*/
/** オブジェクトの（深い）長さを取得
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 * @param len		[in] 長さ
 *
 * @return			オブジェクトの長さ
 *
 * @note			フレームの場合はプロト継承で長さを計算する
 *					スクリプトからの呼出し用
 */

newtRef NsSetLength(newtRefArg rcvr, newtRefArg r, newtRefArg len)
{
	int32_t	n;

	if (NewtRefIsReadonly(r))
		return NewtThrow(kNErrObjectReadOnly, r);

    if (! NewtRefIsInteger(len))
        return NewtThrow(kNErrNotAnInteger, len);

	n = NewtRefToInteger(len);

    switch (NewtGetRefType(r, true))
    {
        case kNewtBinary:
        case kNewtString:
            NewtBinarySetLength(r, n);
            break;

        case kNewtArray:
            NewtSlotsSetLength(r, n, kNewtRefUnbind);
			break;

        case kNewtFrame:
			return NewtThrow(kNErrUnexpectedFrame, r);

		default:
			return NewtThrow(kNErrNotAnArrayOrString, r);
    }

    return r;
}


/*------------------------------------------------------------------------*/
/** フレーム内のスロットの有無を調べる
 *
 * @param rcvr		[in] レシーバ
 * @param frame		[in] フレーム
 * @param slot		[in] スロットシンボル
 *
 * @retval			TRUE	スロットが存在する
 * @retval			NIL		スロットが存在しない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsHasSlot(newtRefArg rcvr, newtRefArg frame, newtRefArg slot)
{
    return NewtMakeBoolean(NewtHasSlot(frame, slot));
}


/*------------------------------------------------------------------------*/
/** フレームからスロットの値を取得
 *
 * @param rcvr		[in] レシーバ
 * @param frame		[in] フレーム
 * @param slot		[in] スロットシンボル
 *
 * @return			スロットの値
 */

newtRef NsGetSlot(newtRefArg rcvr, newtRefArg frame, newtRefArg slot)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(frame);

    if (obj != NULL)
		return NcResolveMagicPointer(NewtObjGetSlot(obj, slot));
    else
        return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** フレームにスロットの値をセットする
 *
 * @param rcvr		[in] レシーバ
 * @param frame		[in] フレーム
 * @param slot		[in] スロットシンボル
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NsSetSlot(newtRefArg rcvr, newtRefArg frame, newtRefArg slot, newtRefArg v)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(frame);

    if (obj != NULL)
	{
		if (NewtRefIsReadonly(frame))
			return NewtThrow(kNErrObjectReadOnly, frame);

		return NewtObjSetSlot(obj, slot, v);
    }

   return kNewtRefNIL;
}


/*------------------------------------------------------------------------*/
/** フレームからスロットを削除する
 *
 * @param rcvr		[in] レシーバ
 * @param frame		[in] フレーム
 * @param slot		[in] スロットシンボル
 *
 * @return			フレーム
 */

newtRef NsRemoveSlot(newtRefArg rcvr, newtRefArg frame, newtRefArg slot)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(frame);
    NewtObjRemoveSlot(obj, slot);

    return frame;
}


/*------------------------------------------------------------------------*/
/** 配列に値をセットする
 *
 * @param r			[in] 配列オブジェクト
 * @param p			[in] 位置
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NcSetArraySlot(newtRefArg r, newtRefArg p, newtRefArg v)
{
    if (! NewtRefIsInteger(p))
        return NewtThrow(kNErrNotAnInteger, p);

    return NewtSetArraySlot(r, NewtRefToInteger(p), v);
}


/*------------------------------------------------------------------------*/
/** オブジェクト内のアクセスパスの有無を調べる
 *
 * @param r			[in] オブジェクト
 * @param p			[in] アクセスパス
 *
 * @retval			TRUE	アクセスパスが存在する
 * @retval			NIL		アクセスパスが存在しない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NcHasPath(newtRefArg r, newtRefArg p)
{
    return NewtMakeBoolean(NewtHasPath(r, p));
}


/*------------------------------------------------------------------------*/
/** オブジェクトのアクセスパスの値を取得
 *
 * @param r			[in] オブジェクト
 * @param p			[in] アクセスパス
 *
 * @return			値オブジェクト
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NcGetPath(newtRefArg r, newtRefArg p)
{
    return NewtGetPath(r, p, NULL);
}


/*------------------------------------------------------------------------*/
/** オブジェクトのアクセスパスに値をセットする
 *
 * @param r			[in] オブジェクト
 * @param p			[in] アクセスパス
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NcSetPath(newtRefArg r, newtRefArg p, newtRefArg v)
{
    newtRefVar	slot;
    newtRefVar	target;

    target = NewtGetPath(r, p, &slot);

    if (target == kNewtRefUnbind)
        NewtThrow(kNErrOutOfBounds, r);

    if (NewtRefIsArray(target))
        return NcSetArraySlot(target, slot, v);
    else
        return NcSetSlot(target, slot, v);
}


/*------------------------------------------------------------------------*/
/** オブジェクトの指定された位置から値を取得
 *
 * @param r			[in] オブジェクト
 * @param p			[in] 位置
 *
 * @return			値オブジェクト
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NcARef(newtRefArg r, newtRefArg p)
{
    if (! NewtRefIsInteger(p))
        return NewtThrow(kNErrNotAnInteger, p);

    return NewtARef(r, NewtRefToInteger(p));
}


/*------------------------------------------------------------------------*/
/** オブジェクトの指定された位置に値をセットする
 *
 * @param r			[in] オブジェクト
 * @param p			[in] 位置
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NcSetARef(newtRefArg r, newtRefArg p, newtRefArg v)
{
    if (! NewtRefIsInteger(p))
        return NewtThrow(kNErrNotAnInteger, p);

    return NewtSetARef(r, NewtRefToInteger(p), v);
}


/*------------------------------------------------------------------------*/
/** プロト・ペアレント継承でフレーム内のスロットの有無を調べる
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] フレーム
 * @param name		[in] スロットシンボル
 *
 * @retval			TRUE	スロットが存在する
 * @retval			NIL		スロットが存在しない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsHasVariable(newtRefArg rcvr, newtRefArg r, newtRefArg name)
{
    return NewtMakeBoolean(NewtHasVariable(r, name));
}


/*------------------------------------------------------------------------*/
/** プロト・ペアレント継承でフレームからスロットの値を取得
 *
 * @param rcvr		[in] レシーバ
 * @param frame		[in] フレーム
 * @param slot		[in] スロットシンボル
 *
 * @return			スロットの値
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsGetVariable(newtRefArg rcvr, newtRefArg frame, newtRefArg slot)
{
    return NcFullLookup(frame, slot);
}


/*------------------------------------------------------------------------*/
/** プロト・ペアレント継承でフレームのスロットに値をセット
 *
 * @param rcvr		[in] レシーバ
 * @param frame		[in] フレーム
 * @param slot		[in] スロットシンボル
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsSetVariable(newtRefArg rcvr, newtRefArg frame, newtRefArg slot, newtRefArg v)
{
	if (NewtAssignment(frame, slot, v))
		return v;
	else
		return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** 変数の有無を調べる
 *
 * @param rcvr		[in] レシーバ
 * @param name		[in] 変数名シンボル
 *
 * @retval			TRUE	スロットが存在する
 * @retval			NIL		スロットが存在しない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsHasVar(newtRefArg rcvr, newtRefArg name)
{
    return NewtMakeBoolean(NewtHasVar(name));
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** オブジェクトタイプを対応するクラスシンボルに変換する
 *
 * @param type		[in] オブジェクトタイプ
 *
 * @return			クラスシンボル
 */

newtRef NewtRefTypeToClass(uint16_t type)
{
    newtRefVar	klass = kNewtRefUnbind;

	switch (type)
	{
		case kNewtInt30:
		case kNewtInt32:
			klass = NS_INT;
			break;

		case kNewtCharacter:
			klass = NS_CHAR;
			break;

		case kNewtTrue:
			klass = NSSYM0(boolean);
			break;

		case kNewtSpecial:
		case kNewtNil:
		case kNewtUnbind:
			klass = NSSYM0(weird_immediate);
			break;

        case kNewtFrame:
            klass = NSSYM0(frame);
            break;

        case kNewtArray:
            klass = NSSYM0(array);
            break;

        case kNewtReal:
			klass = NSSYM0(real);
            break;

        case kNewtSymbol:
			klass = NSSYM0(symbol);
            break;

        case kNewtString:
            klass = NSSYM0(string);
            break;

        case kNewtBinary:
			klass = NSSYM0(binary);
            break;
	}

    return klass;
}


/*------------------------------------------------------------------------*/
/** オブジェクトのクラスシンボルを取得
 *
 * @param r			[in] オブジェクト
 *
 * @return			クラスシンボル
 */

newtRef NewtObjClassOf(newtRefArg r)
{
    newtObjRef	obj;
    newtRefVar	klass = kNewtRefNIL;

    obj = NewtRefToPointer(r);

    if (obj != NULL)
    {
        if (NewtObjIsFrame(obj))
        {
//            klass = NewtObjGetSlot(obj, NS_CLASS);
			klass = NcProtoLookup(r, NS_CLASS);

            if (NewtRefIsNIL(klass))
				klass = NSSYM0(frame);
        }
        else
        {
            klass = obj->as.klass;

			if (klass == kNewtSymbolClass)
				klass = NSSYM0(symbol);
            else if (NewtRefIsNIL(klass))
				klass = NewtRefTypeToClass(NewtGetObjectType(obj, true));
        }
    }

    return klass;
}


/*------------------------------------------------------------------------*/
/** オブジェクトのクラスシンボルをセットする
 *
 * @param r			[in] オブジェクト
 * @param c			[in] クラスシンボル
 *
 * @return			オブジェクト
 */

newtRef NewtObjSetClass(newtRefArg r, newtRefArg c)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(r);

    if (obj != NULL)
    {
        if (NewtObjIsFrame(obj))
		{
			if (NewtRefIsReadonly(r))
			{
				NewtThrow(kNErrObjectReadOnly, r);
				return r;
			}

			NewtObjSetSlot(obj, NS_CLASS, c);
        }
		else
		{
            obj->as.klass = c;
		}
    }

    return r;
}


/*------------------------------------------------------------------------*/
/** オブジェクトのプリミティブクラスを取得
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @return			プリミティブクラス
 */

newtRef NsPrimClassOf(newtRefArg rcvr, newtRefArg r)
{
    newtRefVar	klass;

    if (NewtRefIsPointer(r))
	{
		switch (NewtGetRefType(r, true))
		{
			case kNewtFrame:
				klass = NSSYM0(frame);
				break;

			case kNewtArray:
				klass = NSSYM0(array);
				break;

			default:
				klass = NSSYM0(binary);
				break;
		}
	}
	else
	{
		klass = NSSYM(immediate);
	}

	return klass;
}


/*------------------------------------------------------------------------*/
/** オブジェクトのクラスシンボルを取得
 *
 * @param r			[in] オブジェクト
 *
 * @return			クラスシンボル
 */

newtRef NcClassOf(newtRefArg r)
{
    if (NewtRefIsPointer(r))
        return NewtObjClassOf(r);
    else
		return NewtRefTypeToClass(NewtGetRefType(r, false));
}


/*------------------------------------------------------------------------*/
/** オブジェクトのクラスシンボルをセットする
 *
 * @param r			[in] オブジェクト
 * @param c			[in] クラスシンボル
 *
 * @retval			オブジェクト	クラスシンボルをセットできた場合
 * @retval			NIL			クラスシンボルをセットできなかった場合
 */

newtRef NcSetClass(newtRefArg r, newtRefArg c)
{
    if (NewtRefIsPointer(r))
		return NewtObjSetClass(r, c);
    else
        return kNewtRefNIL;;
}


/*------------------------------------------------------------------------*/
/** 参照の比較
 *
 * @param r1		[in] 参照１
 * @param r2		[in] 参照２
 *
 * @retval			TRUE	同値
 * @retval			NIL		同値でない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NcRefEqual(newtRefArg r1, newtRefArg r2)
{
    return NewtMakeBoolean(NewtRefEqual(r1, r2));
}


/*------------------------------------------------------------------------*/
/** オブジェクトの比較
 *
 * @param rcvr		[in] レシーバ
 * @param r1		[in] オブジェクト１
 * @param r2		[in] オブジェクト２
 *
 * @retval			TRUE	同値
 * @retval			NIL		同値でない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsObjectEqual(newtRefArg rcvr, newtRefArg r1, newtRefArg r2)
{
    return NewtMakeBoolean(NewtObjectEqual(r1, r2));
}


/*------------------------------------------------------------------------*/
/** シンボルを字句的に比較（大文字小文字は区別されない）
 *
 * @param rcvr		[in] レシーバ
 * @param r1		[in] シンボル１
 * @param r2		[in] シンボル２
 *
 * @retval			負の整数	r1 < r2
 * @retval			0		r1 = r2
 * @retval			正の整数	r1 > r2
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsSymbolCompareLex(newtRefArg rcvr, newtRefArg r1, newtRefArg r2)
{
	if (! NewtRefIsSymbol(r1))
        return NewtThrow(kNErrNotASymbol, r1);

	if (! NewtRefIsSymbol(r2))
        return NewtThrow(kNErrNotASymbol, r2);

    return NewtMakeInteger(NewtSymbolCompareLex(r1, r2));
}


/*------------------------------------------------------------------------*/
/** sub が supr のサブクラスを含むかチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param sub		[in] シンボルオブジェクト１
 * @param supr		[in] シンボルオブジェクト２
 *
 * @retval			TRUE	サブクラスを含む
 * @retval			NIL		サブクラスを含まない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsHasSubclass(newtRefArg rcvr, newtRefArg sub, newtRefArg supr)
{
    return NewtMakeBoolean(NewtHasSubclass(sub, supr));
}


/*------------------------------------------------------------------------*/
/** sub が supr のサブクラスかチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param sub		[in] シンボルオブジェクト１
 * @param supr		[in] シンボルオブジェクト２
 *
 * @retval			TRUE	サブクラスである
 * @retval			NIL		サブクラスでない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsIsSubclass(newtRefArg rcvr, newtRefArg sub, newtRefArg supr)
{
    return NewtMakeBoolean(NewtIsSubclass(sub, supr));
}


/*------------------------------------------------------------------------*/
/** obj が r のインスタンスかチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param obj		[in] オブジェクト
 * @param r			[in] クラスシンボル
 *
 * @retval			TRUE	インスタンスである
 * @retval			NIL		インスタンスでない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsIsInstance(newtRefArg rcvr, newtRefArg obj, newtRefArg r)
{
    return NewtMakeBoolean(NewtIsInstance(obj, r));
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** r が配列かチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @retval			TRUE	配列である
 * @retval			NIL		配列でない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsIsArray(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtRefIsArray(r));
}


/*------------------------------------------------------------------------*/
/** r がフレームかチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @retval			TRUE	フレームである
 * @retval			NIL		フレームでない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsIsFrame(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtRefIsFrame(r));
}


/*------------------------------------------------------------------------*/
/** r がバイナリかチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @retval			TRUE	バイナリである
 * @retval			NIL		バイナリでない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsIsBinary(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtRefIsBinary(r));
}


/*------------------------------------------------------------------------*/
/** r がシンボルかチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @retval			TRUE	シンボルである
 * @retval			NIL		シンボルでない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsIsSymbol(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtRefIsSymbol(r));
}


/*------------------------------------------------------------------------*/
/** r が文字列かチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @retval			TRUE	文字列である
 * @retval			NIL		文字列でない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsIsString(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtRefIsString(r));
}


/*------------------------------------------------------------------------*/
/** r が文字かチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @retval			TRUE	文字である
 * @retval			NIL		文字でない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsIsCharacter(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtRefIsCharacter(r));
}


/*------------------------------------------------------------------------*/
/** r が整数かチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @retval			TRUE	整数である
 * @retval			NIL		整数でない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsIsInteger(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtRefIsInteger(r));
}


/*------------------------------------------------------------------------*/
/** r が浮動小数点数かチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @retval			TRUE	浮動小数点数である
 * @retval			NIL		浮動小数点数でない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsIsReal(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtRefIsReal(r));
}


/*------------------------------------------------------------------------*/
/** r が数値データかチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @retval			TRUE	数値データである
 * @retval			NIL		数値データでない
 */

newtRef NsIsNumber(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtRefIsInteger(r) || NewtRefIsReal(r));
}


/*------------------------------------------------------------------------*/
/** r がイミディエイト（即値）かチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @retval			TRUE	イミディエイトである
 * @retval			NIL		イミディエイトでない
 */

newtRef NsIsImmediate(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtRefIsImmediate(r));
}


/*------------------------------------------------------------------------*/
/** r が関数オブジェクトかチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @retval			TRUE	関数オブジェクトである
 * @retval			NIL		関数オブジェクトでない
 */

newtRef NsIsFunction(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtRefIsFunction(r));
}


/*------------------------------------------------------------------------*/
/** r がリードオンリーかチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @retval			TRUE	リードオンリーである
 * @retval			NIL		リードオンリーでない
 */

newtRef NsIsReadonly(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtRefIsReadonly(r));
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 配列オブジェクトに値を追加する
 *
 * @param r			[in] 配列オブジェクト
 * @param v			[in] 値オブジェクト
 *
 * @return			値オブジェクト
 */

newtRef NcAddArraySlot(newtRefArg r, newtRefArg v)
{
    newtObjRef	obj;

    obj = NewtRefToPointer(r);

    if (obj != NULL)
		NewtObjAddArraySlot(obj, v);

    return v;
}


/*------------------------------------------------------------------------*/
/** 配列オブジェクトの要素を文字列に合成する
 *
 * @param r			[in] 配列オブジェクト
 *
 * @return			文字列オブジェクト
 */

newtRef NcStringer(newtRefArg r)
{
    newtRef *	slots;
    newtRefVar	str;
    uint32_t	len;
    uint32_t	i;

    if (! NewtRefIsArray(r))
        return NewtThrow(kNErrNotAnArray, r);

    str = NSSTR("");

    len = NewtArrayLength(r);
    slots = NewtRefToSlots(r);

    for (i = 0; i < len; i++)
    {
        NcStrCat(str, slots[i]);
    }

    return str;
}


/*------------------------------------------------------------------------*/
/** 文字列オブジェクトの最後にオブジェクトを文字列化して追加する
 *
 * @param rcvr		[in] レシーバ
 * @param str		[in] 文字列オブジェクト
 * @param v			[in] オブジェクト
 *
 * @return			文字列オブジェクト
 */

newtRef NsStrCat(newtRefArg rcvr, newtRefArg str, newtRefArg v)
{
	char	wk[32];
    char *	s = NULL;

    switch (NewtGetRefType(v, true))
    {
        case kNewtInt30:
        case kNewtInt32:
            {
                int	n;

                n = (int)NewtRefToInteger(v);
                sprintf(wk, "%d", n);
                s = wk;
            }
            break;

        case kNewtReal:
            {
                double	n;

                n = NewtRefToReal(v);
                sprintf(wk, "%f", n);
                s = wk;
            }
            break;

        case kNewtCharacter:
			{
				int		c;

				c = NewtRefToCharacter(v);
                sprintf(wk, "%c", c);
                s = wk;
			}
            break;

        case kNewtSymbol:
            {
                newtSymDataRef	sym;

                sym = NewtRefToSymbol(v);
                s = sym->name;
            }
            break;

        case kNewtString:
            s = NewtRefToString(v);
            break;
    }

    if (s != NULL)
        NewtStrCat(str, s);

    return str;
}


/*------------------------------------------------------------------------*/
/** 文字列オブジェクトからシンボルを作成する
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] 文字列オブジェクト
 *
 * @return			シンボルオブジェクト
 */

newtRef NsMakeSymbol(newtRefArg rcvr, newtRefArg r)
{
    char *	s;

    if (! NewtRefIsString(r))
        return NewtThrow(kNErrNotAString, r);

    s = NewtRefToString(r);

    return NewtMakeSymbol(s);
}


/*------------------------------------------------------------------------*/
/** フレームオブジェクトを作成する
 *
 * @param rcvr		[in] レシーバ
 *
 * @return			フレームオブジェクト
 */

newtRef NsMakeFrame(newtRefArg rcvr)
{
	return NewtMakeFrame(kNewtRefUnbind, 0);
}


newtRef NsMakeArray(newtRefArg rcvr, newtRefArg size, newtRefArg initialValue) {
  if (!NewtRefIsInteger(size)) {
    return NewtThrow(kNErrNotAnInteger, size);
  }
  int length = NewtRefToInteger(size);
  newtRef result = NewtMakeArray(kNewtRefUnbind, length);
  int i;
  for (i = 0; i<length; i++) {
    NewtSlotsSetSlot(result, i, initialValue);
  }
  return result;
}


/*------------------------------------------------------------------------*/
/** バイナリオブジェクトを作成する
 *
 * @param rcvr		[in] レシーバ
 * @param length	[in] 長さ
 * @param klass		[in] クラス
 *
 * @return			バイナリオブジェクト
 */

newtRef NsMakeBinary(newtRefArg rcvr, newtRefArg length, newtRefArg klass)
{
    if (! NewtRefIsInteger(length))
        return NewtThrow(kNErrNotAnInteger, length);

	return NewtMakeBinary(klass, NULL, NewtRefToInteger(length), false);
}


/*------------------------------------------------------------------------*/
/** Create a binary object from a string containing hexadecimal numbers.
 *
 * @param rcvr		[in] receiver
 * @param hex		[in] hexadecimal number pairs
 * @param klass		[in] class of new binary object
 *
 * @return			a new binary object
 */

newtRef NsMakeBinaryFromHex(newtRefArg rcvr, newtRefArg hex, newtRefArg klass)
{
    if (! NewtRefIsString(hex))
        return NewtThrow(kNErrNotAString, hex);

	return NewtMakeBinaryFromHex(klass, NewtRefToString(hex), false);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 整数のビットAND
 *
 * @param r1		[in] 整数オブジェクト１
 * @param r2		[in] 整数オブジェクト２
 *
 * @return			数値オブジェクト
 */

newtRef NcBAnd(newtRefArg r1, newtRefArg r2)
{
    if (! NewtRefIsInteger(r1))
        return NewtThrow(kNErrNotAnInteger, r1);

    if (! NewtRefIsInteger(r2))
        return NewtThrow(kNErrNotAnInteger, r2);

  return NewtMakeInteger((NewtRefToInteger(r1) & NewtRefToInteger(r2)));
}


/*------------------------------------------------------------------------*/
/** 整数のビットOR
 *
 * @param r1		[in] 整数オブジェクト１
 * @param r2		[in] 整数オブジェクト２
 *
 * @return			数値オブジェクト
 */

newtRef NcBOr(newtRefArg r1, newtRefArg r2)
{
    if (! NewtRefIsInteger(r1))
        return NewtThrow(kNErrNotAnInteger, r1);

    if (! NewtRefIsInteger(r2))
        return NewtThrow(kNErrNotAnInteger, r2);

  return NewtMakeInteger((NewtRefToInteger(r1) | NewtRefToInteger(r2)));
}


/*------------------------------------------------------------------------*/
/** 整数のビットNOT
 *
 * @param r		[in] 整数オブジェクト
 *
 * @return			数値オブジェクト
 */

newtRef NcBNot(newtRefArg r)
{
    if (! NewtRefIsInteger(r))
        return NewtThrow(kNErrNotAnInteger, r);

    return NewtMakeInteger(~ NewtRefToInteger(r));
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** ブール演算 AND
 *
 * @param rcvr		[in] レシーバ
 * @param r1		[in] オブジェクト１
 * @param r2		[in] オブジェクト２
 *
 * @retval			TRUE
 * @retval			NIL
 */

newtRef NsAnd(newtRefArg rcvr, newtRefArg r1, newtRefArg r2)
{
	bool	result;

	result = (NewtRefIsNotNIL(r1) && NewtRefIsNotNIL(r2));

    return NewtMakeBoolean(result);
}


/*------------------------------------------------------------------------*/
/** ブール演算 OR
 *
 * @param rcvr		[in] レシーバ
 * @param r1		[in] オブジェクト１
 * @param r2		[in] オブジェクト２
 *
 * @retval			TRUE
 * @retval			NIL
 */

newtRef NsOr(newtRefArg rcvr, newtRefArg r1, newtRefArg r2)
{
	bool	result;

	result = (NewtRefIsNotNIL(r1) || NewtRefIsNotNIL(r2));

    return NewtMakeBoolean(result);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 数値引数のチェック
 *
 * @param r1		[in] オブジェクト１
 * @param r2		[in] オブジェクト２
 * @param real		[out]浮動小数点を含む
 *
 * @retval			true	引数が数値
 * @retval			false   引数が数値でない
 */


bool NewtArgsIsNumber(newtRefArg r1, newtRefArg r2, bool * real)
{
    *real = false;

    if (NewtRefIsReal(r1))
    {
        *real = true;
    }
    else
    {
        if (! NewtRefIsInteger(r1))
            return false;
    }

    if (NewtRefIsReal(r2))
    {
        *real = true;
    }
    else
    {
        if (! NewtRefIsInteger(r2))
            return false;
    }

    return true;
}


/*------------------------------------------------------------------------*/
/** 加算(r1 + r2)
 *
 * @param r1		[in] 数値オブジェクト１
 * @param r2		[in] 数値オブジェクト２
 *
 * @return			数値オブジェクト
 */

newtRef NcAdd(newtRefArg r1, newtRefArg r2)
{
    bool	real;

    if (! NewtArgsIsNumber(r1, r2, &real))
		return NewtThrow0(kNErrNotANumber);

    if (real)
    {
        double real1;
        double real2;

        real1 = NewtRefToReal(r1);
        real2 = NewtRefToReal(r2);

        return NewtMakeReal(real1 + real2);
    }
    else
    {
        int32_t	int1;
        int32_t	int2;

        int1 = NewtRefToInteger(r1);
        int2 = NewtRefToInteger(r2);

        return NewtMakeInteger(int1 + int2);
    }
}


/*------------------------------------------------------------------------*/
/** 減算(r1 - r2)
 *
 * @param r1		[in] 数値オブジェクト１
 * @param r2		[in] 数値オブジェクト２
 *
 * @return			数値オブジェクト
 */

newtRef NcSubtract(newtRefArg r1, newtRefArg r2)
{
    bool	real;

    if (! NewtArgsIsNumber(r1, r2, &real))
		return NewtThrow0(kNErrNotANumber);

    if (real)
    {
        double real1;
        double real2;

        real1 = NewtRefToReal(r1);
        real2 = NewtRefToReal(r2);

        return NewtMakeReal(real1 - real2);
    }
    else
    {
        int32_t	int1;
        int32_t	int2;

        int1 = NewtRefToInteger(r1);
        int2 = NewtRefToInteger(r2);

        return NewtMakeInteger(int1 - int2);
    }
}


/*------------------------------------------------------------------------*/
/** 乗算(r1 x r2)
 *
 * @param r1		[in] 数値オブジェクト１
 * @param r2		[in] 数値オブジェクト２
 *
 * @return			数値オブジェクト
 */

newtRef NcMultiply(newtRefArg r1, newtRefArg r2)
{
    bool	real;

    if (! NewtArgsIsNumber(r1, r2, &real))
		return NewtThrow0(kNErrNotANumber);

    if (real)
    {
        double real1;
        double real2;

        real1 = NewtRefToReal(r1);
        real2 = NewtRefToReal(r2);

        return NewtMakeReal(real1 * real2);
    }
    else
    {
        int32_t	int1;
        int32_t	int2;

        int1 = NewtRefToInteger(r1);
        int2 = NewtRefToInteger(r2);

        return NewtMakeInteger(int1 * int2);
    }
}


/*------------------------------------------------------------------------*/
/** 割算(r1 / r2)
 *
 * @param r1		[in] 数値オブジェクト１
 * @param r2		[in] 数値オブジェクト２
 *
 * @return			数値オブジェクト
 */

newtRef NcDivide(newtRefArg r1, newtRefArg r2)
{
    bool	real;

    if (! NewtArgsIsNumber(r1, r2, &real))
		return NewtThrow0(kNErrNotANumber);

    if (real)
    {
        double real1;
        double real2;

        real1 = NewtRefToReal(r1);
        real2 = NewtRefToReal(r2);

        if (real2 == 0.0)
            return NewtThrow(kNErrDiv0, r2);

        return NewtMakeReal(real1 / real2);
    }
    else
    {
        int32_t	int1;
        int32_t	int2;

        int1 = NewtRefToInteger(r1);
        int2 = NewtRefToInteger(r2);

        if (int2 == 0)
            return NewtThrow(kNErrDiv0, r2);

        double result = (double)int1 / (double)int2;
        if (result == (int)result) {
            return NewtMakeInteger(result);
        }
        else {
            return NewtMakeReal(result);
        }
    }
}


/*------------------------------------------------------------------------*/
/** 整数の割算(r1 / r2)
 *
 * @param r1		[in] 整数オブジェクト１
 * @param r2		[in] 整数オブジェクト２
 *
 * @return			整数オブジェクト
 */

newtRef NcDiv(newtRefArg r1, newtRefArg r2)
{
    if (! NewtRefIsInteger(r1))
        return NewtThrow(kNErrNotAnInteger, r1);

    if (! NewtRefIsInteger(r2))
        return NewtThrow(kNErrNotAnInteger, r2);

    return NewtMakeInteger(NewtRefToInteger(r1) / NewtRefToInteger(r2));
}


/*------------------------------------------------------------------------*/
/** r1 を r2 で割ったの余りを計算
 *
 * @param rcvr		[in] レシーバ
 * @param r1		[in] 整数オブジェクト１
 * @param r2		[in] 整数オブジェクト２
 *
 * @return			整数オブジェクト
 */

newtRef NsMod(newtRefArg rcvr, newtRefArg r1, newtRefArg r2)
{
    if (! NewtRefIsInteger(r1))
        return NewtThrow(kNErrNotAnInteger, r1);

    if (! NewtRefIsInteger(r2))
        return NewtThrow(kNErrNotAnInteger, r2);

    return NewtMakeInteger(NewtRefToInteger(r1) % NewtRefToInteger(r2));
}


/*------------------------------------------------------------------------*/
/** ビットシフト(r1 << r2)
 *
 * @param rcvr		[in] レシーバ
 * @param r1		[in] 整数オブジェクト１
 * @param r2		[in] 整数オブジェクト２
 *
 * @return			整数オブジェクト
 */

newtRef NsShiftLeft(newtRefArg rcvr, newtRefArg r1, newtRefArg r2)
{
    if (! NewtRefIsInteger(r1))
        return NewtThrow(kNErrNotAnInteger, r1);

    if (! NewtRefIsInteger(r2))
        return NewtThrow(kNErrNotAnInteger, r2);

    return NewtMakeInteger(NewtRefToInteger(r1) << NewtRefToInteger(r2));
}


/*------------------------------------------------------------------------*/
/** ビットシフト(r1 >> r2)
 *
 * @param rcvr		[in] レシーバ
 * @param r1		[in] 整数オブジェクト１
 * @param r2		[in] 整数オブジェクト２
 *
 * @return			整数オブジェクト
 */

newtRef NsShiftRight(newtRefArg rcvr, newtRefArg r1, newtRefArg r2)
{
    if (! NewtRefIsInteger(r1))
        return NewtThrow(kNErrNotAnInteger, r1);

    if (! NewtRefIsInteger(r2))
        return NewtThrow(kNErrNotAnInteger, r2);

    return NewtMakeInteger(NewtRefToInteger(r1) >> NewtRefToInteger(r2));
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** オブジェクトの大小比較(r1 < r2)
 *
 * @param r1		[in] オブジェクト１
 * @param r2		[in] オブジェクト２
 *
 * @retval			TRUE
 * @retval			NIL
 *
 * @note			スクリプトの呼出し用
 */

newtRef NcLessThan(newtRefArg r1, newtRefArg r2)
{
    return NewtMakeBoolean(NewtObjectCompare(r1, r2) < 0);
}


/*------------------------------------------------------------------------*/
/** オブジェクトの大小比較(r1 > r2)
 *
 * @param r1		[in] オブジェクト１
 * @param r2		[in] オブジェクト２
 *
 * @retval			TRUE
 * @retval			NIL
 *
 * @note			スクリプトの呼出し用
 */

newtRef NcGreaterThan(newtRefArg r1, newtRefArg r2)
{
    return NewtMakeBoolean(NewtObjectCompare(r1, r2) > 0);
}


/*------------------------------------------------------------------------*/
/** オブジェクトの大小比較(r1 >= r2)
 *
 * @param r1		[in] オブジェクト１
 * @param r2		[in] オブジェクト２
 *
 * @retval			TRUE
 * @retval			NIL
 *
 * @note			スクリプトの呼出し用
 */

newtRef NcGreaterOrEqual(newtRefArg r1, newtRefArg r2)
{
    return NewtMakeBoolean(NewtObjectCompare(r1, r2) >= 0);
}


/*------------------------------------------------------------------------*/
/** オブジェクトの大小比較(r1 <= r2)
 *
 * @param r1		[in] オブジェクト１
 * @param r2		[in] オブジェクト２
 *
 * @retval			TRUE
 * @retval			NIL
 *
 * @note			スクリプトの呼出し用
 */

newtRef NcLessOrEqual(newtRefArg r1, newtRefArg r2)
{
    return NewtMakeBoolean(NewtObjectCompare(r1, r2) <= 0);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 正規表現オブジェクト（フレーム）の生成
 *
 * @param rcvr		[in] レシーバ
 *
 * @return			例外フレーム
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsCurrentException(newtRefArg rcvr)
{
	return NVMCurrentException();
}


#ifdef __NAMED_MAGIC_POINTER__
/*------------------------------------------------------------------------*/
/** 正規表現オブジェクト（フレーム）の生成
 *
 * @param rcvr		[in] レシーバ
 * @param pattern	[in] パターン文字列
 * @param opt		[in] オプション文字列
 *
 * @return			NIL
 */

newtRef NsMakeRegex(newtRefArg rcvr, newtRefArg pattern, newtRefArg opt)
{
    newtRefVar	v[] = {
                            NSSYM0(_proto),		NSNAMEDMP0(protoREGEX),
                            NSSYM0(pattern),	pattern,
                            NSSYM0(option),		opt,
                        };

	return NewtMakeFrame2(sizeof(v) / (sizeof(newtRefVar) * 2), v);
}

#endif /* __NAMED_MAGIC_POINTER__ */


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** Call a function with an array of parameters
 * 
 * @param rcvr		[in] レシーバ
 * @param func		[in] Function
 * @param params	[in] Parameters
 * 
 * @return			Return value
 */
newtRef 	NsApply(newtRefArg rcvr, newtRefArg func, newtRefArg params)
{
	if (! NewtRefIsFunction(func))
	{
		return NewtThrow(kNErrNotAFunction, func);
	}
	
	newtRef ary = NewtRefIsNIL(params) ? NewtMakeArray(kNewtRefUnbind, 0) : params;
	return NcCallWithArgArray(func, ary);
}

/*------------------------------------------------------------------------*/
/** Send a message to a method in a frame by name with an array of parameters
 * 
 * @param rcvr		[in] レシーバ
 * @param frame		[in] Frame
 * @param message	[in] Message
 * @param params	[in] Parameters
 * 
 * @return			Return value
 */
newtRef 	NsPerform(newtRefArg rcvr, newtRefArg frame, newtRefArg message, newtRefArg params)
{
	newtRef ary = NewtRefIsNIL(params) ? NewtMakeArray(kNewtRefUnbind, 0) : params;
	return NcSendWithArgArray(frame, message, false, ary);
}

/*------------------------------------------------------------------------*/
/** Send a message to a method in a frame by name with an array of parameters, if it is defined
 * 
 * @param rcvr		[in] レシーバ
 * @param frame		[in] Frame
 * @param message	[in] Message
 * @param params	[in] Parameters
 * 
 * @return			Return value
 */
newtRef 	NsPerformIfDefined(newtRefArg rcvr, newtRefArg frame, newtRefArg message, newtRefArg params)
{
	newtRef ary = NewtRefIsNIL(params) ? NewtMakeArray(kNewtRefUnbind, 0) : params;
	return NcSendWithArgArray(frame, message, true, ary);
}
/*------------------------------------------------------------------------*/
/** Send a message to a method in a frame by name with an array of parameters (proto inheritance only)
 * 
 * @param rcvr		[in] レシーバ
 * @param frame		[in] Frame
 * @param message	[in] Message
 * @param params	[in] Parameters
 * 
 * @return			Return value
 */
newtRef 	NsProtoPerform(newtRefArg rcvr, newtRefArg frame, newtRefArg message, newtRefArg params)
{
	newtRef ary = NewtRefIsNIL(params) ? NewtMakeArray(kNewtRefUnbind, 0) : params;
	return NcSendProtoWithArgArray(frame, message, false, ary);
}

/*------------------------------------------------------------------------*/
/** Send a message to a method in a frame by name with an array of parameters, if it is defined (proto inheritance only)
 * 
 * @param rcvr		[in] レシーバ
 * @param frame		[in] Frame
 * @param message	[in] Message
 * @param params	[in] Parameters
 * 
 * @return			Return value
 */
newtRef 	NsProtoPerformIfDefined(newtRefArg rcvr, newtRefArg frame, newtRefArg message, newtRefArg params)
{
	newtRef ary = NewtRefIsNIL(params) ? NewtMakeArray(kNewtRefUnbind, 0) : params;
	return NcSendProtoWithArgArray(frame, message, true, ary);
}

#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 標準出力にオブジェクトをプリント
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @return			NIL
 *
 * @note			グローバル関数用
 */

newtRef NsPrintObject(newtRefArg rcvr, newtRefArg r)
{
    NewtPrintObject(stdout, r);
    return kNewtRefNIL;
}


/*------------------------------------------------------------------------*/
/** 標準出力にオブジェクトをプリント
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @return			NIL
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsPrint(newtRefArg rcvr, newtRefArg r)
{
    NewtPrint(stdout, r);
    return kNewtRefNIL;
}


/*------------------------------------------------------------------------*/
/** 標準出力に関数オブジェクトをダンプ出力
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] 関数オブジェクト
 *
 * @return			NIL
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsDumpFn(newtRefArg rcvr, newtRefArg r)
{
    NVMDumpFn(stdout, r);
    return kNewtRefNIL;
}


/*------------------------------------------------------------------------*/
/** 標準出力にバイトコードをダンプ出力
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] バイトコード
 *
 * @return			NIL
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsDumpBC(newtRefArg rcvr, newtRefArg r)
{
    NVMDumpBC(stdout, r);
    return kNewtRefNIL;
}


/*------------------------------------------------------------------------*/
/** 標準出力にスタックをダンプ出力
 *
 * @param rcvr		[in] レシーバ
 *
 * @return			NIL
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsDumpStacks(newtRefArg rcvr)
{
    NVMDumpStacks(stdout);
    return kNewtRefNIL;
}


/*------------------------------------------------------------------------*/
/** 標準出力に関数情報を表示
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @return			NIL
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsInfo(newtRefArg rcvr, newtRefArg r)
{
    NewtInfo(r);
    return kNewtRefNIL;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 文字列オブジェクトをコンパイル
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] 文字列オブジェクト
 *
 * @return			関数オブジェクト
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsCompile(newtRefArg rcvr, newtRefArg r)
{
    if (! NewtRefIsString(r))
        return NewtThrow(kNErrNotAString, r);

    return NBCCompileStr(NewtRefToString(r), true);
}


/*------------------------------------------------------------------------*/
/** 環境変数の取得
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] 文字列オブジェクト
 *
 * @return			文字列オブジェクト
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsGetEnv(newtRefArg rcvr, newtRefArg r)
{
    if (! NewtRefIsString(r))
        return NewtThrow(kNErrNotAString, r);

    return NewtGetEnv(NewtRefToString(r));
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** オフセット位置から符号付きの1バイトを取り出す。 
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] バイナリオブジェクト
 * @param offset	[in] オフセット
 *
 * @return			符号付きの1バイト
 */

newtRef NsExtractByte(newtRefArg rcvr, newtRefArg r, newtRefArg offset)
{
    if (! NewtRefIsBinary(r))
        return NewtThrow(kNErrNotABinaryObject, r);

    if (! NewtRefIsInteger(offset))
        return NewtThrow(kNErrNotAnInteger, offset);

    return NewtGetBinarySlot(r, NewtRefToInteger(offset));
}

newtRef NsExtractWord(newtRefArg rcvr, newtRefArg r, newtRefArg offset)
{
  if (! NewtRefIsBinary(r))
    return NewtThrow(kNErrNotABinaryObject, r);
  
  if (! NewtRefIsInteger(offset))
    return NewtThrow(kNErrNotAnInteger, offset);

  uint32_t len = NewtBinaryLength(r);
  uint32_t p = NewtRefToInteger(offset);
  
  if (p < len && p + 1 < len)
  {
    uint8_t *	data = NewtRefToBinary(r);
    uint32_t value = (data[p] << 8) | (data[p+1]);
    return NewtMakeInteger(value);
  }
  
  return kNewtRefUnbind;

}

newtRef NsRef(newtRefArg rcvr, newtRefArg integer)
{
  if (! NewtRefIsInteger(integer))
    return NewtThrow(kNErrNotAnInteger, integer);

  newtRef result = (newtRef)NewtRefToInteger(integer);
  if (NewtRefIsPointer(result) == false) {
    //result = NSSYM0(weird_immediate);
  }
  return result;
}

newtRef NsRefOf(newtRefArg rcvr, newtRefArg object)
{
  return NewtMakeInteger(object);
}

newtRef NsNegate(newtRefArg rcvr, newtRefArg integer)
{
  if (! NewtRefIsInteger(integer))
    return NewtThrow(kNErrNotAnInteger, integer);
  
  return NewtMakeInt32(- (NewtRefToInteger(integer)));
}

newtRef NsSetContains(newtRefArg rcvr, newtRefArg array, newtRefArg item) {
  if (!NewtRefIsArray(array)) {
    return NewtThrow(kNErrNotAnArray, array);
  }
  
  newtRef *	slots;
  uint32_t	n;
  uint32_t	i;
  
  slots = NewtRefToSlots(array);
  n = NewtLength(array);
  
  for (i = 0; i < n; i++)
  {
    if (slots[i] == item) {
      return NewtMakeInteger(i);
    }
  }
  
  return kNewtRefNIL;
}
