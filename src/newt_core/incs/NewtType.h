/*------------------------------------------------------------------------*/
/**
 * @file	NewtType.h
 * @brief   型定義
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTTYPE_H
#define	NEWTTYPE_H


/* ヘッダファイル */
#include "platform.h"

#if HAVE_STDINT_H
	#include <stdint.h>
#else
	#include <inttypes.h>
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "NewtConf.h"


/* マクロ */

// Newton Refs Constant
#define	kNewtRefNIL			0x0002		///< NIL
#define	kNewtRefTRUE		0x001A		///< TRUE
#define	kNewtSymbolClass	0x55552		///< シンボルクラス

#define	kNewtRefUnbind		0xFFF2		///< #UNDEF（独自機能）


/* 定数 */

/// オブジェクトタイプ（内部でのみ使用）
enum {
    kNewtUnknownType		= 0,	///< 不明なタイプ
    kNewtInt30,						///< 30bit整数（即値）
    kNewtPointer,					///< ポインタ参照
    kNewtCharacter,					///< 文字（即値）
    kNewtSpecial,					///< 特殊参照（即値）
    kNewtNil,						///< NIL（特殊参照／即値）
    kNewtTrue,						///< TRUE（特殊参照／即値）
    kNewtUnbind,					///< 未定義（特殊参照／即値）
    kNewtMagicPointer,				///< マジックポインタ（即値）

    //　ポインタ参照
    kNewtBinary,					///< バイナリオブジェクト
    kNewtArray,						///< 配列
    kNewtFrame,						///< フレーム

    //　バイナリオブジェクト
    kNewtInt32,						///< 32bit整数
    kNewtReal,						///< 浮動小数点
    kNewtSymbol,					///< シンボル
    kNewtString						///< 文字列
};


/// Newton Object Constant
enum {
    kNewtObjSlotted		= 0x01,		///< スロット
    kNewtObjFrame		= 0x02,		///< フレーム

    kNewtObjLiteral		= 0x40,		///< リテラル
    kNewtObjSweep		= 0x80		///< ゴミ掃除（GC用）
};


/// Newton Map Constant
enum {
    kNewtMapSorted		= 0x01,		///< スロット
    kNewtMapProto		= 0x04		///< プロト
};


/// Newton Streamed Object Format (NSOF)
enum {
    kNSOFImmediate			= 0,	///< 即値
    kNSOFCharacter			= 1,	///< ASCII文字
    kNSOFUnicodeCharacter	= 2,	///< UNICODE文字
    kNSOFBinaryObject		= 3,	///< バイナリオブジェクト
    kNSOFArray				= 4,	///< 配列
    kNSOFPlainArray			= 5,	///< プレイン配列
    kNSOFFrame				= 6,	///< フレーム
    kNSOFSymbol				= 7,	///< シンボル
    kNSOFString				= 8,	///< 文字列
    kNSOFPrecedent			= 9,	///< 出現済みデータ
    kNSOFNIL				= 10,   ///< NIL
    kNSOFSmallRect			= 11,   ///< 小さい矩形
    kNSOFLargeBinary		= 12,	///< 大きいバイナリ

    kNSOFNamedMagicPointer	= 0x10,	///< 名前付マジックポインタ（独自機能）
};


/// Newton Package Format: Part Flags
enum {
	kProtocolPart	= 0,		///< part contains protocol data
	kNOSPart		= 1,		///< part contains NOS data
	kRawPart		= 2,		///< raw part data, no defined type
	kAutoLoadFlag	= 0x0010,	///< protocols will be registered automatically
	kAutoRemoveFlag	= 0x0020,	///< protocols will be unregistered automatically
	kNotifyFlag		= 0x0080,	///< notify system handler of installation
	kAutoCopyFlag	= 0x0100	///< part must be moved into precious RAM before activation
};


/// Newton Package Format: Object Header Flags
enum {
	kObjSlotted	= 0x01,	///< object is an array
	kObjFrame	= 0x02,	///< object is a frame if both flags are set
};


/* 型宣言 */

// Ref(Integer, Pointer, Charcter, Spatial, Magic pointer)
typedef uint32_t		newtRef;		///< オブジェクト参照
typedef newtRef			newtRefVar;		///< オブジェクト参照変数
typedef const newtRef	newtRefArg;		///< オブジェクト参照引数


/// オブジェクト参照
typedef struct newtObj *	newtObjRef;

/// オブジェクトヘッダ
typedef struct {
    uint32_t	h;		///< 管理情報
    newtObjRef	nextp;	///< 次のオブジェクトへのポインタ
} newtObjHeader;


/// オブジェクト
typedef struct newtObj {
    newtObjHeader	header; ///< オブジェクトヘッダ

	/// as
    union {
        newtRef	klass;		///< クラス
        newtRef	map;		///< マップ
    } as;
} newtObj;


/// シンボルデータ
typedef struct {
    uint32_t	hash;		///< ハッシュ値
    char		name[1];	///< テキスト
} newtSymData;

/// シンボルデータへのポインタ
typedef newtSymData *	newtSymDataRef;


/// エラーコード
typedef int32_t		newtErr;


#endif /* NEWTTYPE_H */

