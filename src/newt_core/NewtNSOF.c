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


/* マクロ */
#define NSOFIsNOS(verno)	((verno == 1) || (verno == 2))	///< Newton OS　互換の NSOF


/* 型宣言 */

/// NSOFストリーム構造体
typedef struct {
	int32_t		verno;			///< NSOFバージョン番号
	uint8_t *	data;			///< データ
	uint32_t	len;			///< データの長さ
	uint32_t	offset;			///< 作業中の位置
	newtRefVar	precedents;		///< 出現済みオブジェクトのリスト
	newtErr		lastErr;		///< 最後のエラーコード
} nsof_stream_t;


/* 関数プロトタイプ */
static bool			NewtRefIsByte(newtRefArg r);
static bool			NewtRefIsSmallRect(newtRefArg r);
static int32_t		NewtArraySearch(newtRefArg array, newtRefArg r);

static newtErr		NSOFWriteByte(nsof_stream_t * nsof, uint8_t value);
static newtErr		NSOFWriteXlong(nsof_stream_t * nsof, int32_t value);
static uint8_t		NSOFReadByte(nsof_stream_t * nsof);
static int32_t		NSOFReadXlong(nsof_stream_t * nsof);

static newtErr		NSOFWritePrecedent(nsof_stream_t * nsof, int32_t pos);
static newtErr		NSOFWriteImmediate(nsof_stream_t * nsof, newtRefArg r);
static newtErr		NSOFWriteCharacter(nsof_stream_t * nsof, newtRefArg r);
static newtErr		NSOFWriteBinary(nsof_stream_t * nsof, newtRefArg r);
static newtErr		NSOFWriteSymbol(nsof_stream_t * nsof, newtRefArg r);
static newtErr		NSOFWriteNamedMP(nsof_stream_t * nsof, newtRefArg r);
static newtErr		NSOFWriteArray(nsof_stream_t * nsof, newtRefArg r);
static newtErr		NSOFWriteFrame(nsof_stream_t * nsof, newtRefArg r);
static newtErr		NSOFWriteSmallRect(nsof_stream_t * nsof, newtRefArg r);
static newtErr		NewtWriteNSOF(nsof_stream_t * nsof, newtRefArg r);

static newtRef		NSOFReadBinary(nsof_stream_t * nsof, int type);
static newtRef		NSOFReadArray(nsof_stream_t * nsof, int type);
static newtRef		NSOFReadFrame(nsof_stream_t * nsof);
static newtRef		NSOFReadSymbol(nsof_stream_t * nsof);
static newtRef		NSOFReadNamedMP(nsof_stream_t * nsof);
static newtRef		NSOFReadSmallRect(nsof_stream_t * nsof);
static newtRef		NSOFReadNSOF(nsof_stream_t * nsof);


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
/** 1byte を NSOF でバッファに書込む
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param value		[in] 1byte　データ
 *
 * @return			エラーコード
 *
 * @note			nsof->data が NULL の場合は nsof->offset のみ更新される
 */
 
newtErr NSOFWriteByte(nsof_stream_t * nsof, uint8_t value)
{
	if (nsof->data)
	{
		if (nsof->len <= nsof->offset)
		{	// バッファを越えた
			nsof->lastErr = kNErrOutOfRange;
			return nsof->lastErr;
		}

		nsof->data[nsof->offset] = value;
	}

	nsof->offset++;

	return kNErrNone;
}


/*------------------------------------------------------------------------*/
/** データを xlong 形式でバッファに書込む
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param value		[in] データ
 *
 * @return			エラーコード
 *
 * @note			nsof->data が NULL の場合は nsof->offset のみ更新される
 */

newtErr NSOFWriteXlong(nsof_stream_t * nsof, int32_t value)
{
	if (0 <= value && value <= 254)
	{
		NSOFWriteByte(nsof, value);
	}
	else
	{
		NSOFWriteByte(nsof, 0xff);
		NSOFWriteByte(nsof, ((uint32_t)value >> 24) & 0xffff);
		NSOFWriteByte(nsof, ((uint32_t)value >> 16) & 0xffff);
		NSOFWriteByte(nsof, ((uint32_t)value >> 8) & 0xffff);
		NSOFWriteByte(nsof, (uint32_t)value & 0xffff);
	}

	return nsof->lastErr;
}


/*------------------------------------------------------------------------*/
/** NSOFバッファ からデータを 1byte 読込む
 *
 * @param nsof		[i/o]NSOFバッファ
 *
 * @return			1byte データ
 */
 
uint8_t NSOFReadByte(nsof_stream_t * nsof)
{
	uint8_t		result;

	if (nsof->len <= nsof->offset)
	{	// バッファを越えた
		nsof->lastErr = kNErrNotABinaryObject;
		return 0;
	}

	result = nsof->data[nsof->offset];
	nsof->offset++;

	return result;
}


/*------------------------------------------------------------------------*/
/** NSOFバッファ からデータを xlong 形式で読込む
 *
 * @param nsof		[i/o]NSOFバッファ
 *
 * @return			データ
 */

int32_t NSOFReadXlong(nsof_stream_t * nsof)
{
	int32_t		value;

	value = NSOFReadByte(nsof);

	if (value == 0xff)
	{
		value  = NSOFReadByte(nsof) << 24;
		value |= NSOFReadByte(nsof) << 16;
		value |= NSOFReadByte(nsof) << 8;
		value |= NSOFReadByte(nsof);
	}

	return value;
}


#pragma mark -
/*------------------------------------------------------------------------*/
/** 出現済みデータを NSOF でバッファに書込む
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param pos		[in] 出現位置
 *
 * @return			エラーコード
 *
 * @note			nsof->data が NULL の場合は nsof->offset のみ更新される
 */
 
newtErr NSOFWritePrecedent(nsof_stream_t * nsof, int32_t pos)
{
	NSOFWriteByte(nsof, kNSOFPrecedent);
	NSOFWriteXlong(nsof, pos);

	return nsof->lastErr;
}


/*------------------------------------------------------------------------*/
/** 即値データを NSOF でバッファに書込む
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param r			[in] 即値データ
 *
 * @return			エラーコード
 *
 * @note			nsof->data が NULL の場合は nsof->offset のみ更新される
 */
 
newtErr NSOFWriteImmediate(nsof_stream_t * nsof, newtRefArg r)
{
	NSOFWriteByte(nsof, kNSOFImmediate);
	NSOFWriteXlong(nsof, r);

	return nsof->lastErr;
}


/*------------------------------------------------------------------------*/
/** 文字データを NSOF でバッファに書込む
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param r			[in] 文字データ
 *
 * @return			エラーコード
 *
 * @note			nsof->data が NULL の場合は nsof->offset のみ更新される
 */

newtErr NSOFWriteCharacter(nsof_stream_t * nsof, newtRefArg r)
{
	int		c;

	c = NewtRefToCharacter(r);

	if (c < 0x100)
	{
		NSOFWriteByte(nsof, kNSOFCharacter);
		NSOFWriteByte(nsof, c);
	}
	else
	{
		NSOFWriteByte(nsof, kNSOFUnicodeCharacter);
		NSOFWriteByte(nsof, (c >> 8) & 0xff);
		NSOFWriteByte(nsof, c & 0xff);
	}

	return nsof->lastErr;
}


/*------------------------------------------------------------------------*/
/** バイナリデータを NSOF でバッファに書込む
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param r			[in] バイナリオブジェクト
 *
 * @return			エラーコード
 *
 * @note			nsof->data が NULL の場合は nsof->offset のみ更新される
 */

newtErr NSOFWriteBinary(nsof_stream_t * nsof, newtRefArg r)
{
	newtRefVar	klass;
	uint32_t	size;
	int			type;

	klass = NcClassOf(r);

	if (klass == NSSYM0(string))
		type = kNSOFString;
	else
		type = kNSOFBinaryObject;

	size = NewtBinaryLength(r);

	NSOFWriteByte(nsof, type);
	NSOFWriteXlong(nsof, size);

	if (type == kNSOFBinaryObject)
	{
		NewtWriteNSOF(nsof, klass);
	}

	if (nsof->data) memcpy(nsof->data + nsof->offset, NewtRefToBinary(r), size);
	nsof->offset += size;

	return nsof->lastErr;
}


/*------------------------------------------------------------------------*/
/** シンボルデータを NSOF でバッファに書込む
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param r			[in] シンボルオブジェクト
 *
 * @return			エラーコード
 *
 * @note			nsof->data が NULL の場合は nsof->offset のみ更新される
 */

newtErr NSOFWriteSymbol(nsof_stream_t * nsof, newtRefArg r)
{
	uint32_t	size;

	size = NewtSymbolLength(r);

	NSOFWriteByte(nsof, kNSOFSymbol);
	NSOFWriteXlong(nsof, size);

	if (nsof->data) memcpy(nsof->data + nsof->offset, NewtRefToSymbol(r)->name, size);
	nsof->offset += size;

	return nsof->lastErr;
}


#ifdef __NAMED_MAGIC_POINTER__
/*------------------------------------------------------------------------*/
/** 名前付マジックポインタを NSOF でバッファに書込む
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param r			[in] 名前付マジックポインタ
 *
 * @return			エラーコード
 *
 * @note			nsof->data が NULL の場合は nsof->offset のみ更新される
 */

newtErr NSOFWriteNamedMP(nsof_stream_t * nsof, newtRefArg r)
{
	newtRefVar	sym;

	sym = NewtMPToSymbol(r);

	if (NSOFIsNOS(nsof->verno))
	{
		// とりあえずシンボルを書込む
		NSOFWriteSymbol(nsof, sym);
		nsof->lastErr = kNErrNSOFWrite;
	}
	else
	{
		uint32_t	size;

		size = NewtSymbolLength(sym);

		NSOFWriteByte(nsof, kNSOFNamedMagicPointer);
		NSOFWriteXlong(nsof, size);

		if (nsof->data) memcpy(nsof->data + nsof->offset, NewtRefToSymbol(sym)->name, size);
		nsof->offset += size;
	}

	return nsof->lastErr;
}
#endif /* __NAMED_MAGIC_POINTER__ */


/*------------------------------------------------------------------------*/
/** 配列データを NSOF でバッファに書込む
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param r			[in] 配列オブジェクト
 *
 * @return			エラーコード
 *
 * @note			nsof->data が NULL の場合は nsof->offset のみ更新される
 */

newtErr NSOFWriteArray(nsof_stream_t * nsof, newtRefArg r)
{
	newtRefVar	klass;
    newtRef *	slots;
	uint32_t	numSlots;
	uint32_t	i;
	int			type;

	numSlots = NewtArrayLength(r);
	klass = NcClassOf(r);

	if (klass == NSSYM0(array))
		type = kNSOFPlainArray;
	else
		type = kNSOFArray;

	NSOFWriteByte(nsof, type);

	NSOFWriteXlong(nsof, numSlots);

	if (type == kNSOFArray)
	{
		NewtWriteNSOF(nsof, klass);
		if (nsof->lastErr != kNErrNone) return nsof->lastErr;
	}

    slots = NewtRefToSlots(r);

	for (i = 0; i < numSlots; i++)
	{
		NewtWriteNSOF(nsof, slots[i]);
		if (nsof->lastErr != kNErrNone) return nsof->lastErr;
	}

	return nsof->lastErr;
}


/*------------------------------------------------------------------------*/
/** フレームデータを NSOF でバッファに書込む
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param r			[in] フレームオブジェクト
 *
 * @return			エラーコード
 *
 * @note			nsof->data が NULL の場合は nsof->offset のみ更新される
 */

newtErr NSOFWriteFrame(nsof_stream_t * nsof, newtRefArg r)
{
    newtRefVar	map;
    newtRef *	slots;
	uint32_t	numSlots;
    uint32_t	index;
	uint32_t	i;

	numSlots = NewtFrameLength(r);
    map = NewtFrameMap(r);

	NSOFWriteByte(nsof, kNSOFFrame);
	NSOFWriteXlong(nsof, numSlots);

    slots = NewtRefToSlots(r);

	for (i = 0; i < numSlots; i++)
	{
		index = 0;
		NewtWriteNSOF(nsof, NewtGetMapIndex(map, i, &index));
		if (nsof->lastErr != kNErrNone) return nsof->lastErr;
	}

	for (i = 0; i < numSlots; i++)
	{
		NewtWriteNSOF(nsof, slots[i]);
		if (nsof->lastErr != kNErrNone) return nsof->lastErr;
	}

	return nsof->lastErr;
}


/*------------------------------------------------------------------------*/
/** フレームデータを NSOF(smallRect) でバッファに書込む
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param r			[in] フレームオブジェクト
 *
 * @return			エラーコード
 *
 * @note			nsof->data が NULL の場合は nsof->offset のみ更新される
 */

newtErr NSOFWriteSmallRect(nsof_stream_t * nsof, newtRefArg r)
{
	NSOFWriteByte(nsof, kNSOFSmallRect);
	NSOFWriteByte(nsof, NewtRefToInteger(NcGetSlot(r, NSSYM(top))));
	NSOFWriteByte(nsof, NewtRefToInteger(NcGetSlot(r, NSSYM(left))));
	NSOFWriteByte(nsof, NewtRefToInteger(NcGetSlot(r, NSSYM(bottom))));
	NSOFWriteByte(nsof, NewtRefToInteger(NcGetSlot(r, NSSYM(right))));

	return nsof->lastErr;
}


/*------------------------------------------------------------------------*/
/** オブジェクトを NSOFバイナリオブジェクトに変換して書込む
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param r			[in] オブジェクト
 *
 * @return			エラーコード
 *
 * @note			nsof->data が NULL の場合は nsof->offset のみ更新される
 */

newtErr NewtWriteNSOF(nsof_stream_t * nsof, newtRefArg r)
{
	if (NewtRefIsImmediate(r))
	{
		if (r == kNewtRefNIL)
			NSOFWriteByte(nsof, kNSOFNIL);
		else if (NewtRefIsCharacter(r))
			NSOFWriteCharacter(nsof, r);
		else
			NSOFWriteImmediate(nsof, r);
	}
	else
	{
		int32_t	foundPrecedent;

		foundPrecedent = NewtArraySearch(nsof->precedents, r);

		if (foundPrecedent < 0)
		{
			NcAddArraySlot(nsof->precedents, r);

			switch (NewtGetRefType(r, true))
			{
				case kNewtArray:
					NSOFWriteArray(nsof, r);
					break;

				case kNewtFrame:
					if (NewtRefIsSmallRect(r))
						NSOFWriteSmallRect(nsof, r);
					else
						NSOFWriteFrame(nsof, r);
					break;

				case kNewtSymbol:
					NSOFWriteSymbol(nsof, r);
					break;

#ifdef __NAMED_MAGIC_POINTER__
				case kNewtMagicPointer:
					NSOFWriteNamedMP(nsof, r);
					break;
#endif /* __NAMED_MAGIC_POINTER__ */

				default:
					NSOFWriteBinary(nsof, r);
					break;
			}
		}
		else
		{
			NSOFWritePrecedent(nsof, foundPrecedent);
		}
	}

	return nsof->lastErr;
}


/*------------------------------------------------------------------------*/
/** オブジェクトを NSOFバイナリオブジェクトに変換する
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 * @param ver		[in] バージョン
 *
 * @return			NSOFバイナリオブジェクト
 */

newtRef NsMakeNSOF(newtRefArg rcvr, newtRefArg r, newtRefArg ver)
{
	nsof_stream_t	nsof;
	newtRefVar	result;

    if (! NewtRefIsInteger(ver))
        return NewtThrow(kNErrNotAnInteger, ver);

	memset(&nsof, 0, sizeof(nsof));
	nsof.verno = NewtRefToInteger(ver);
	nsof.precedents = NewtMakeArray(kNewtRefUnbind, 0);
	nsof.offset = 1;

	// 必要なサイズの計算
	NewtWriteNSOF(&nsof, r);

	if (nsof.lastErr != kNErrNone)
        return NewtThrow(nsof.lastErr, r);

	// バイナリオブジェクトの作成
	result = NewtMakeBinary(NSSYM(NSOF), NULL, nsof.offset, false);

	if (NewtRefIsNotNIL(result))
	{	// 実際の書込み
		NewtSetLength(nsof.precedents, 0);
		nsof.data = NewtRefToBinary(result);
		nsof.len = nsof.offset;
		nsof.offset = 0;

		NSOFWriteByte(&nsof, nsof.verno);
		NewtWriteNSOF(&nsof, r);
	}

    return result;
}


#pragma mark -
/*------------------------------------------------------------------------*/
/** NSOFバッファを読込んでバイナリオブジェクトに変換する
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param type		[in] NSOFのタイプ
 *
 * @return			バイナリオブジェクト
 */

newtRef NSOFReadBinary(nsof_stream_t * nsof, int type)
{
	newtRefVar	klass;
	newtRefVar	r;
	int32_t		xlen;

	xlen = NSOFReadXlong(nsof);

	if (type == kNSOFString)
	{
		klass = NSSYM0(string);
	}
	else
	{
		klass = NSOFReadNSOF(nsof);
		if (nsof->lastErr != kNErrNone) return kNewtRefUnbind;
	}

	r = NewtMakeBinary(klass, nsof->data + nsof->offset, xlen, false); 
	nsof->offset += xlen;

	return r;
}


/*------------------------------------------------------------------------*/
/** NSOFバッファを読込んで配列オブジェクトに変換する
 *
 * @param nsof		[i/o]NSOFバッファ
 * @param type		[in] NSOFのタイプ
 *
 * @return			配列オブジェクト
 */

newtRef NSOFReadArray(nsof_stream_t * nsof, int type)
{
	newtRefVar	klass = kNewtRefUnbind;
	newtRefVar	r;
	int32_t		xlen;

	xlen = NSOFReadXlong(nsof);

	if (type == kNSOFArray)
	{
		klass = NSOFReadNSOF(nsof);
		if (nsof->lastErr != kNErrNone) return kNewtRefUnbind;
	}

	r = NewtMakeArray(klass, xlen);
	NcAddArraySlot(nsof->precedents, r);

	if (NewtRefIsNotNIL(r))
	{
		newtRef *	slots;
		int32_t	i;

		slots = NewtRefToSlots(r);

		for (i = 0; i < xlen; i++)
		{
			slots[i] = NSOFReadNSOF(nsof);
			if (nsof->lastErr != kNErrNone) break;
		}
	}

	return r;
}


/*------------------------------------------------------------------------*/
/** NSOFバッファを読込んでフレームオブジェクトに変換する
 *
 * @param nsof		[i/o]NSOFバッファ
 *
 * @return			フレームオブジェクト
 */

newtRef NSOFReadFrame(nsof_stream_t * nsof)
{
	newtRefVar	map;
	newtRefVar	r;
	newtRef *	slots;
	int32_t		xlen;
	int32_t		i;

	xlen = NSOFReadXlong(nsof);

	if (xlen == 0)
		return NcMakeFrame();

	map = NewtMakeMap(kNewtRefNIL, xlen, NULL);
	r = NewtMakeFrame(map, xlen);
	NcAddArraySlot(nsof->precedents, r);

	slots = NewtRefToSlots(map);

	for (i = 1; i <= xlen; i++)
	{
		slots[i] = NSOFReadNSOF(nsof);
		if (nsof->lastErr != kNErrNone) return kNewtRefUnbind;
	}

	slots = NewtRefToSlots(r);

	for (i = 0; i < xlen; i++)
	{
		slots[i] = NSOFReadNSOF(nsof);
		if (nsof->lastErr != kNErrNone) break;
	}

	return r;
}


/*------------------------------------------------------------------------*/
/** NSOFバッファを読込んでシンボルオブジェクトに変換する
 *
 * @param nsof		[i/o]NSOFバッファ
 *
 * @return			シンボルオブジェクト
 */

newtRef NSOFReadSymbol(nsof_stream_t * nsof)
{
	newtRefVar	r = kNewtRefUnbind;
	int32_t		xlen;
	char *		name;

	xlen = NSOFReadXlong(nsof);

	name = malloc(xlen + 1);

	if (name)
	{
		memcpy(name, nsof->data + nsof->offset, xlen);
		name[xlen] = '\0';
		r = NewtMakeSymbol(name);
		free(name);
	}

	nsof->offset += xlen;

	return r;
}


#ifdef __NAMED_MAGIC_POINTER__
/*------------------------------------------------------------------------*/
/** NSOFバッファを読込んで名前付マジックポインタに変換する
 *
 * @param nsof		[i/o]NSOFバッファ
 *
 * @return			名前付マジックポインタ
 */

newtRef NSOFReadNamedMP(nsof_stream_t * nsof)
{
	newtRefVar	r;

	r = NSOFReadSymbol(nsof);

	if (NewtRefIsNotNIL(r))
	{
		if (NSOFIsNOS(nsof->verno))
		{
			nsof->lastErr = kNErrNSOFRead;
			// とりあえずシンボルのまま
		}
		else
		{
			r = NewtSymbolToMP(r);
		}
	}

	return r;
}
#endif /* __NAMED_MAGIC_POINTER__ */


/*------------------------------------------------------------------------*/
/** NSOFバッファを読込んでフレームオブジェクト(smallRect)に変換する
 *
 * @param nsof		[i/o]NSOFバッファ
 *
 * @return			フレームオブジェクト(smallRect)
 */

newtRef NSOFReadSmallRect(nsof_stream_t * nsof)
{
	newtRefVar	r;

	r = NcMakeFrame();
	// 将来は map を共有すること

	NcSetSlot(r, NSSYM(top), NewtMakeInteger(NSOFReadByte(nsof)));
	NcSetSlot(r, NSSYM(left), NewtMakeInteger(NSOFReadByte(nsof)));
	NcSetSlot(r, NSSYM(bottom), NewtMakeInteger(NSOFReadByte(nsof)));
	NcSetSlot(r, NSSYM(right), NewtMakeInteger(NSOFReadByte(nsof)));

	return r;
}


/*------------------------------------------------------------------------*/
/** NSOFバイナリオブジェクトを読込んでオブジェクトに変換する
 *
 * @param nsof		[i/o]NSOFバッファ
 *
 * @return			オブジェクト
 */

newtRef NSOFReadNSOF(nsof_stream_t * nsof)
{
	newtRefVar	r = kNewtRefUnbind;
	int32_t		xlen;
	int			type;

	type = NSOFReadByte(nsof);

	switch (type)
	{
		case kNSOFImmediate:
			r = (newtRef)NSOFReadXlong(nsof);
			break;

		case kNSOFCharacter:
			r = NewtMakeCharacter(NSOFReadByte(nsof));
			break;

		case kNSOFUnicodeCharacter:
			r = NewtMakeCharacter((uint32_t)NSOFReadByte(nsof) << 8 | NSOFReadByte(nsof));
			break;

		case kNSOFBinaryObject:
		case kNSOFString:
			r = NSOFReadBinary(nsof, type);
			NcAddArraySlot(nsof->precedents, r);
			break;

		case kNSOFArray:
		case kNSOFPlainArray:
			r = NSOFReadArray(nsof, type);
			break;

		case kNSOFFrame:
			r = NSOFReadFrame(nsof);
			break;

		case kNSOFSymbol:
			r = NSOFReadSymbol(nsof);
			NcAddArraySlot(nsof->precedents, r);
			break;

		case kNSOFPrecedent:
			xlen = NSOFReadXlong(nsof);
			r = NewtGetArraySlot(nsof->precedents, xlen);
			break;

		case kNSOFNIL:
			r = kNewtRefNIL;
			break;

		case kNSOFSmallRect:
			r = NSOFReadSmallRect(nsof);
			NcAddArraySlot(nsof->precedents, r);
			break;

#ifdef __NAMED_MAGIC_POINTER__
		case kNSOFNamedMagicPointer:
			r = NSOFReadNamedMP(nsof);
			NcAddArraySlot(nsof->precedents, r);
			break;
#endif /* __NAMED_MAGIC_POINTER__ */

		case kNSOFLargeBinary:
		default:
			// サポートされていません
			nsof->lastErr = kNErrNSOFRead;
			break;
	}

	return r;
}


/*------------------------------------------------------------------------*/
/** NSOFバイナリオブジェクトを読込む
 *
 * @param data		[in] NSOFデータ
 * @param size		[in] NSOFデータサイズ
 *
 * @return			オブジェクト
 */

newtRef NewtReadNSOF(uint8_t * data, size_t size)
{
	nsof_stream_t	nsof;

	memset(&nsof, 0, sizeof(nsof));

	nsof.data = data;
	nsof.len = size;
	nsof.precedents = NewtMakeArray(kNewtRefUnbind, 0);
	nsof.verno = NSOFReadByte(&nsof);

	return NSOFReadNSOF(&nsof);
}


/*------------------------------------------------------------------------*/
/** NSOFバイナリオブジェクトを読込む
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] NSOFバイナリオブジェクト
 *
 * @return			オブジェクト
 */

newtRef NsReadNSOF(newtRefArg rcvr, newtRefArg r)
{
	uint32_t	len;

    if (! NewtRefIsBinary(r))
        return NewtThrow(kNErrNotABinaryObject, r);

	len = NewtBinaryLength(r);

	if (len < 2)
        return NewtThrow(kNErrOutOfRange, r);

	return NewtReadNSOF(NewtRefToBinary(r), len);
}
