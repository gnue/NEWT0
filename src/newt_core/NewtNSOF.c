/*------------------------------------------------------------------------*/
/**
 * @file	NewtNSOF.c
 * @brief   Newton Streamed Object Format
 *
 * @author  M.Nukui
 * @date	2005-04-01
 *
 * Copyright (C) 2005 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <string.h>

#include "NewtNSOF.h"
#include "NewtErrs.h"
#include "NewtObj.h"
#include "NewtEnv.h"
#include "NewtFns.h"
#include "NewtVM.h"


/* 関数プロトタイプ */
static bool			NewtRefIsByte(newtRefArg r);
static bool			NewtRefIsSmallRect(newtRefArg r);
static int32_t		NewtArraySearch(newtRefArg array, newtRefArg r);

static uint32_t		NewtWriteXlong(uint8_t * data, uint32_t offset, int32_t v);
static uint32_t		NewtWriteNIL(uint8_t * data, uint32_t offset);
static uint32_t		NewtWritePrecedent(uint8_t * data, uint32_t offset, int32_t pos);
static uint32_t		NewtWriteImmediate(uint8_t * data, uint32_t offset, newtRefArg r);
static uint32_t		NewtWriteCharacter(uint8_t * data, uint32_t offset, newtRefArg r);
static uint32_t		NewtWriteBinary(int verno, newtRefArg precedents, uint8_t * data, uint32_t offset, newtRefArg r);
static uint32_t		NewtWriteSymbol(uint8_t * data, uint32_t offset, newtRefArg r);
static uint32_t		NewtWriteArray(int verno, newtRefArg precedents, uint8_t * data, uint32_t offset, newtRefArg r);
static uint32_t		NewtWriteFrame(int verno, newtRefArg precedents, uint8_t * data, uint32_t offset, newtRefArg r);
static uint32_t		NewtWriteSmallRect(uint8_t * data, uint32_t offset, newtRefArg r);

static uint32_t		NewtWriteNSOF(int verno, newtRefArg precedents, uint8_t * data, uint32_t offset, newtRefArg r);


#pragma mark -
/*------------------------------------------------------------------------*/
/** オブジェクトが 0～255 の整数かチェックする
 *
 * @param r			[in] オブジェクト
 *
 * @retval			true	0～255 の整数
 * @retval			false   0～255 の整数でない
 */

bool NewtRefIsByte(newtRefArg r)
{
	if (NewtRefIsInteger(r))
	{
		int32_t		n;

		n = NewtRefToInteger(r);

		if (0 <= n && n <= 255)
			return true;
	}

	return false;
}


/*------------------------------------------------------------------------*/
/** フレームが NSOF(smallRect) の条件を満たすかチェックする
 *
 * @param r			[in] フレームオブジェクト
 *
 * @retval			true	条件を満たす
 * @retval			false	条件を満たさない
 */

bool NewtRefIsSmallRect(newtRefArg r)
{
	if (NewtFrameLength(r) == 4)
	{
		if (NewtRefIsByte(NcGetSlot(r, NSSYM(top))) &&
			NewtRefIsByte(NcGetSlot(r, NSSYM(left))) && 
			NewtRefIsByte(NcGetSlot(r, NSSYM(bottom))) && 
			NewtRefIsByte(NcGetSlot(r, NSSYM(right))))
		{
			return true;
		}
	}

	return false;
}


/*------------------------------------------------------------------------*/
/** 配列からオブジェクトを探す
 *
 * @param array		[in] 配列
 * @param r			[in] フレームオブジェクト
 *
 * @retval			0以上	見つかった位置
 * @retval			-1		見つからなかった
 */

int32_t NewtArraySearch(newtRefArg array, newtRefArg r)
{
    newtRef *	slots;
	uint32_t	len;
	uint32_t	i;

	len = NewtArrayLength(array);
    slots = NewtRefToSlots(array);

	for (i = 0; i < len; i++)
	{
		if (slots[i] == r)
			return i;
	}

	return -1;
}


#pragma mark -
/*------------------------------------------------------------------------*/
/** データを xlong 形式でバッファに書込む
 *
 * @param data		[out]バッファ
 * @param offset	[in] 書込み位置
 * @param v			[in] データ
 *
 * @return			書込まれる長さ
 *
 * @note			data が NULL の場合は長さのみ計算して返す
 */

uint32_t NewtWriteXlong(uint8_t * data, uint32_t offset, int32_t v)
{
	uint32_t	len = 0;

	if (0 <= v && v <= 254)
		len = 1;
	else
		len = 5;

	if (data)
	{
		if (len == 1)
		{
			data[offset++] = v;
		}
		else
		{
			data[offset++] = 0xff;
			data[offset++] = ((uint32_t)v >> 24) & 0xffff;
			data[offset++] = ((uint32_t)v >> 16) & 0xffff;
			data[offset++] = ((uint32_t)v >> 8) & 0xffff;
			data[offset++] = (uint32_t)v & 0xffff;
		}
	}

	return len;
}


/*------------------------------------------------------------------------*/
/** NILデータを NSOF でバッファに書込む
 *
 * @param data		[out]バッファ
 * @param offset	[in] 書込み位置
 *
 * @return			書込まれる長さ
 *
 * @note			data が NULL の場合は長さのみ計算して返す
 */
 
uint32_t NewtWriteNIL(uint8_t * data, uint32_t offset)
{
	if (data) data[offset] = kNSOFNIL;

	return 1;
}


/*------------------------------------------------------------------------*/
/** 出現済みデータを NSOF でバッファに書込む
 *
 * @param data		[out]バッファ
 * @param offset	[in] 書込み位置
 * @param pos		[in] 出現位置
 *
 * @return			書込まれる長さ
 *
 * @note			data が NULL の場合は長さのみ計算して返す
 */
 
uint32_t NewtWritePrecedent(uint8_t * data, uint32_t offset, int32_t pos)
{
	uint32_t	len = 0;

	if (data) data[offset + len] = kNSOFPrecedent;
	len++;

	len += NewtWriteXlong(data, offset + len, pos);

	return len;
}


/*------------------------------------------------------------------------*/
/** 即値データを NSOF でバッファに書込む
 *
 * @param data		[out]バッファ
 * @param offset	[in] 書込み位置
 * @param r			[in] 即値データ
 *
 * @return			書込まれる長さ
 *
 * @note			data が NULL の場合は長さのみ計算して返す
 */
 
uint32_t NewtWriteImmediate(uint8_t * data, uint32_t offset, newtRefArg r)
{
	uint32_t	len = 0;

	if (data) data[offset + len] = kNSOFImmediate;
	len++;

	len += NewtWriteXlong(data, offset + len, r);

	return len;
}


/*------------------------------------------------------------------------*/
/** 文字データを NSOF でバッファに書込む
 *
 * @param data		[out]バッファ
 * @param offset	[in] 書込み位置
 * @param r			[in] 文字データ
 *
 * @return			書込まれる長さ
 *
 * @note			data が NULL の場合は長さのみ計算して返す
 */

uint32_t NewtWriteCharacter(uint8_t * data, uint32_t offset, newtRefArg r)
{
	uint32_t	len = 1;
	int			type;
	int			c;

	c = NewtRefToCharacter(r);

	if (c < 0x100)
	{
		type = kNSOFCharacter;
		len += 1;
	}
	else
	{
		type = kNSOFUnicodeCharacter;
		len += 2;
	}

	if (data)
	{
		data[offset++] = type;

		if (type == kNSOFCharacter)
		{
			data[offset++] = c;
		}
		else
		{
			data[offset++] = (c >> 8) & 0xff;
			data[offset++] = c & 0xff;
		}
	}

	return len;
}


/*------------------------------------------------------------------------*/
/** バイナリデータを NSOF でバッファに書込む
 *
 * @param verno		[in] NSOF バージョン番号
 * @param precedents[i/o]出現済みテーブル
 * @param data		[out]バッファ
 * @param offset	[in] 書込み位置
 * @param r			[in] バイナリオブジェクト
 *
 * @return			書込まれる長さ
 *
 * @note			data が NULL の場合は長さのみ計算して返す
 */

uint32_t NewtWriteBinary(int verno, newtRefArg precedents, uint8_t * data, uint32_t offset, newtRefArg r)
{
	newtRefVar	klass;
	uint32_t	size;
	uint32_t	len = 0;
	int			type;

	klass = NcClassOf(r);

	if (klass == NSSYM0(string))
		type = kNSOFString;
	else
		type = kNSOFBinaryObject;

	size = NewtBinaryLength(r);

	if (data) data[offset + len] = type;
	len++;

	len += NewtWriteXlong(data, offset + len, size);

	if (type == kNSOFBinaryObject)
	{
		len += NewtWriteNSOF(verno, precedents, data, offset + len, klass);
	}

	if (data) memcpy(data + offset + len, NewtRefToBinary(r), size);
	len += size;

	return len;
}


/*------------------------------------------------------------------------*/
/** シンボルデータを NSOF でバッファに書込む
 *
 * @param data		[out]バッファ
 * @param offset	[in] 書込み位置
 * @param r			[in] シンボルオブジェクト
 *
 * @return			書込まれる長さ
 *
 * @note			data が NULL の場合は長さのみ計算して返す
 */

uint32_t NewtWriteSymbol(uint8_t * data, uint32_t offset, newtRefArg r)
{
	uint32_t	size;
	uint32_t	len = 0;

	size = NewtSymbolLength(r);

	if (data) data[offset + len] = kNSOFSymbol;
	len++;

	len += NewtWriteXlong(data, offset + len, size);

	if (data) memcpy(data + offset + len, NewtRefToSymbol(r)->name, size);
	len += size;

	return len;
}


/*------------------------------------------------------------------------*/
/** 配列データを NSOF でバッファに書込む
 *
 * @param verno		[in] NSOF バージョン番号
 * @param precedents[i/o]出現済みテーブル
 * @param data		[out]バッファ
 * @param offset	[in] 書込み位置
 * @param r			[in] 配列オブジェクト
 *
 * @return			書込まれる長さ
 *
 * @note			data が NULL の場合は長さのみ計算して返す
 */

uint32_t NewtWriteArray(int verno, newtRefArg precedents, uint8_t * data, uint32_t offset, newtRefArg r)
{
	newtRefVar	klass;
    newtRef *	slots;
	uint32_t	numSlots;
	uint32_t	len = 0;
	uint32_t	i;
	int			type;

	numSlots = NewtArrayLength(r);
	klass = NcClassOf(r);

	if (klass == NSSYM0(array))
		type = kNSOFPlainArray;
	else
		type = kNSOFArray;

	if (data) data[offset + len] = type;
	len++;

	len += NewtWriteXlong(data, offset + len, numSlots);

	if (type == kNSOFArray)
	{
		len += NewtWriteNSOF(verno, precedents, data, offset + len, klass);
	}

    slots = NewtRefToSlots(r);

	for (i = 0; i < numSlots; i++)
	{
		len += NewtWriteNSOF(verno, precedents, data, offset + len, slots[i]);
	}

	return len;
}


/*------------------------------------------------------------------------*/
/** フレームデータを NSOF でバッファに書込む
 *
 * @param verno		[in] NSOF バージョン番号
 * @param precedents[i/o]出現済みテーブル
 * @param data		[out]バッファ
 * @param offset	[in] 書込み位置
 * @param r			[in] フレームオブジェクト
 *
 * @return			書込まれる長さ
 *
 * @note			data が NULL の場合は長さのみ計算して返す
 */

uint32_t NewtWriteFrame(int verno, newtRefArg precedents, uint8_t * data, uint32_t offset, newtRefArg r)
{
    newtRefVar	map;
    newtRef *	slots;
	uint32_t	numSlots;
    uint32_t	index;
	uint32_t	len = 0;
	uint32_t	i;

	numSlots = NewtFrameLength(r);
    map = NewtFrameMap(r);

	if (data) data[offset + len] = kNSOFFrame;
	len++;

	len += NewtWriteXlong(data, offset + len, numSlots);

    slots = NewtRefToSlots(r);

	for (i = 0; i < numSlots; i++)
	{
		len += NewtWriteNSOF(verno, precedents, data, offset + len, NewtGetMapIndex(map, i, &index));
	}

	for (i = 0; i < numSlots; i++)
	{
		len += NewtWriteNSOF(verno, precedents, data, offset + len, slots[i]);
	}

	return len;
}


/*------------------------------------------------------------------------*/
/** フレームデータを NSOF(smallRect) でバッファに書込む
 *
 * @param data		[out]バッファ
 * @param offset	[in] 書込み位置
 * @param r			[in] フレームオブジェクト
 *
 * @return			書込まれる長さ
 *
 * @note			data が NULL の場合は長さのみ計算して返す
 */

uint32_t NewtWriteSmallRect(uint8_t * data, uint32_t offset, newtRefArg r)
{
	if (data)
	{
		data[offset++] = kNSOFSmallRect;
		data[offset++] = NewtRefToInteger(NcGetSlot(r, NSSYM(top)));
		data[offset++] = NewtRefToInteger(NcGetSlot(r, NSSYM(left)));
		data[offset++] = NewtRefToInteger(NcGetSlot(r, NSSYM(bottom)));
		data[offset++] = NewtRefToInteger(NcGetSlot(r, NSSYM(right)));
	}

	return 5;
}


/*------------------------------------------------------------------------*/
/** オブジェクトを NSOFバイナリオブジェクト に変換して書込む
 *
 * @param verno		[in] NSOF バージョン番号
 * @param precedents[i/o]出現済みテーブル
 * @param data		[out]バッファ
 * @param offset	[in] 書込み位置
 * @param r			[in] オブジェクト
 *
 * @return			書込まれる長さ
 */

uint32_t NewtWriteNSOF(int verno, newtRefArg precedents, uint8_t * data, uint32_t offset, newtRefArg r)
{
	uint32_t	len = 0;

	if (NewtRefIsMagicPointer(r))
	{
		if (verno == 2)
		{
			len = NewtWriteImmediate(data, offset, r);
		}
		else
		{
		}
	}
	else if (NewtRefIsImmediate(r))
	{
		if (r == kNewtRefNIL)
			len = NewtWriteNIL(data, offset);
		else if (NewtRefIsCharacter(r))
			len = NewtWriteCharacter(data, offset, r);
		else
			len = NewtWriteImmediate(data, offset, r);
	}
	else
	{
		int32_t	foundPrecedent;

		foundPrecedent = NewtArraySearch(precedents, r);

		if (foundPrecedent < 0)
		{
			NcAddArraySlot(precedents, r);

			switch (NewtGetRefType(r, true))
			{
				case kNewtArray:
					len = NewtWriteArray(verno, precedents, data, offset, r);
					break;

				case kNewtFrame:
					if (NewtRefIsSmallRect(r))
						len = NewtWriteSmallRect(data, offset, r);
					else
						len = NewtWriteFrame(verno, precedents, data, offset, r);
					break;

				case kNewtSymbol:
					len = NewtWriteSymbol(data, offset, r);
					break;

				default:
					len = NewtWriteBinary(verno, precedents, data, offset, r);
					break;
			}
		}
		else
		{
			len = NewtWritePrecedent(data, offset, foundPrecedent);
		}
	}

	return len;
}


#pragma mark -
/*------------------------------------------------------------------------*/
/** オブジェクトを NSOFバイナリオブジェクト に変換する
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 * @param ver		[in] バージョン
 *
 * @return			NSOFバイナリオブジェクト
 */

newtRef NsMakeNSOF(newtRefArg rcvr, newtRefArg r, newtRefArg ver)
{
	newtRefVar	precedents;
	newtRefVar	nsof;
	uint32_t	len = 1;
	int32_t		verno;

    if (! NewtRefIsInteger(ver))
        return NewtThrow(kNErrNotAnInteger, ver);

	verno = NewtRefToInteger(ver);
	precedents = NewtMakeArray(kNewtRefUnbind, 0);

	// 必要なサイズの計算
	len += NewtWriteNSOF(verno, precedents, NULL, len, r);

	// バイナリオブジェクトの作成
	nsof = NewtMakeBinary(NSSYM(NSOF), NULL, len, false);

	if (NewtRefIsNotNIL(nsof))
	{	// 実際の書込み
		uint32_t	offset = 0;
		uint8_t *	data;

		NewtSetLength(precedents, 0);
		data = NewtRefToBinary(nsof);

		data[offset++] = verno;
		NewtWriteNSOF(verno, precedents, data, offset, r);
	}

    return nsof;
}
