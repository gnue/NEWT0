/*------------------------------------------------------------------------*/
/**
 * @file	NewtBC.h
 * @brief   バイトコードの生成
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTBC_H
#define	NEWTBC_H

/* ヘッダファイル */
#include "NewtType.h"
#include "NewtParser.h"


/* マクロ */

// Instruction Code
#define kNBCFieldMask				0x07		///< バイトコードフィールドのマスク

#define	kNBCInstructionsLen			26			///< 命令セットテーブルの長さ
#define	kNBCSimpleInstructionsLen	8			///< シンプル命令セットテーブルの長さ
#define	kBCFuncsLen					25			///< 関数テーブルの長さ


/* 定数 */

enum {
    kNBCPop					= 000,	// 000 pop
    kNBCDup					= 001,	// 001 dup
    kNBCReturn				= 002,	// 002 return
    kNBCPushSelf			= 003,	// 003 push-self
    kNBCSetLexScope			= 004,	// 004 set-lex-scope
    kNBCIterNext			= 005,	// 005 iter-next
    kNBCIterDone			= 006,	// 006 iter-done
    kNBCPopHandlers			= 007	// 007 000 001 pop-handlers
};


enum {
    kNBCPush				= 0030,	// 03x push
    kNBCPushConstant		= 0040,	// 04x (B signed) push-constant
    kNBCCall				= 0050,	// 05x call
    kNBCInvoke				= 0060,	// 06x invoke
    kNBCSend				= 0070,	// 07x send
    kNBCSendIfDefined		= 0100,	// 10x send-if-defined
    kNBCResend				= 0110,	// 11x resend
    kNBCResendIfDefined		= 0120,	// 12x resend-if-defined
    kNBCBranch				= 0130,	// 13x branch
    kNBCBranchIfTrue		= 0140,	// 14x branch-if-true
    kNBCBranchIfFalse		= 0150,	// 15x branch-if-false
    kNBCFindVar				= 0160,	// 16x find-var
    kNBCGetVar				= 0170,	// 17x get-var
    kNBCMakeFrame			= 0200,	// 20x make-frame
    kNBCMakeArray			= 0210,	// 21x make-array
    kNBCGetPath				= 0220,	// 220/221 get-path
    kNBCSetPath				= 0230,	// 230/231 set-path
    kNBCSetVar				= 0240,	// 24x set-var
    kNBCFindAndSetVar		= 0250,	// 25x find-and-set-var
    kNBCIncrVar				= 0260,	// 26x incr-var
    kNBCBranchIfLoopNotDone	= 0270,	// 27x branch-if-loop-not-done
    kNBCFreqFunc			= 0300,	// 30x freq-func
    kNBCNewHandlers			= 0310	// 31x new-handlers
};

// Primitive functions
enum {
    kNBCAdd					= 0,	//  0 add				|+|
    kNBCSubtract			= 1,	//  1 subtract			|-|
    kNBCAref				= 2,	//  2 aref				aref
    kNBCSetAref				= 3,	//  3 set-aref			setAref
    kNBCEquals				= 4,	//  4 equals			|=|
    kNBCNot					= 5,	//  5 not				|not|
    kNBCNotEqual			= 6,	//  6 not-equals		|<>|
    kNBCMultiply			= 7,	//  7 multiply			|*|
    kNBCDivide				= 8,	//  8 divide			|/|
    kNBCDiv					= 9,	//  9 div				|div|
    kNBCLessThan			= 10,	// 10 less-than			|<|
    kNBCGreaterThan			= 11,	// 11 greater-than		|>|
    kNBCGreaterOrEqual		= 12,	// 12 greater-or-equal	|>=|
    kNBCLessOrEqual			= 13,	// 13 less-or-equal		|<=|
    kNBCBitAnd				= 14,	// 14 bit-and			BAnd
    kNBCBitOr				= 15,	// 15 bit-or			BOr
    kNBCBitNot				= 16,	// 16 bit-not			BNot
    kNBCNewIterator			= 17,	// 17 new-iterator		newIterator
    kNBCLength				= 18,	// 18 length			Length
    kNBCClone				= 19,	// 19 clone				Clone
    kNBCSetClass			= 20,	// 20 set-class			SetClass
    kNBCAddArraySlot		= 21,	// 21 add-array-slot	AddArraySlot
    kNBCStringer			= 22,	// 22 stringer			Stringer
    kNBCHasPath				= 23,	// 23 has-path			none
    kNBCClassOf				= 24	// 24 class-of			ClassOf
};


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


void		NBCInit(void);
void		NBCCleanup(void);
newtRef		NBCConstantTable(void);
newtRef		NBCGenBC(nps_syntax_node_t * stree, uint32_t size, bool ret);
newtRef		NBCCompileFile(char * s, bool ret);
newtRef		NBCCompileStr(char * s, bool ret);
void		NBError(int32_t err);


#ifdef __cplusplus
}
#endif


#endif /* NEWTBC_H */

