/*------------------------------------------------------------------------*/
/**
 * @file	NewtParser.h
 * @brief   構文木の生成
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTPARSE_H
#define	NEWTPARSE_H

/* ヘッダファイル */
#include <stdio.h>

#include "NewtType.h"
#include "NewtConf.h"


/* マクロ */

/*
#define kNPSSyntaxNodeMask			0x80000003											///< オブジェクト参照のマスク（構文木ノード用）

#define NPSRefIsSyntaxNode(r)		(((r) & kNPSSyntaxNodeMask) == kNPSSyntaxNodeMask)	///< オブジェクト参照が構文木ノードか？
#define NPSRefToSyntaxNode(r)		(((uint32_t)(r) & 0x7fffffff) >> 2)					///< オブジェクト参照を構文木ノードに変換
#define NPSMakeSyntaxNode(v)		(((v) << 2) | kNPSSyntaxNodeMask)					///< 構文木ノードのオブジェクト参照を作成
*/

#define kNPSSyntaxNodeMask			0x0000000e										///< オブジェクト参照のマスク（構文木ノード用）

#define NPSRefIsSyntaxNode(r)		(((r) & 0x0000000f) == kNPSSyntaxNodeMask)		///< オブジェクト参照が構文木ノードか？
#define NPSRefToSyntaxNode(r)		((uint32_t)(r) >> 4)							///< オブジェクト参照を構文木ノードに変換
#define NPSMakeSyntaxNode(v)		(((v) << 4) | kNPSSyntaxNodeMask)				///< 構文木ノードのオブジェクト参照を作成


/* 定数 */

/// オペレータコード
enum {
	kNPS_NOT				= 256,  ///< not
	kNPS_DIV,						///< div
	kNPS_MOD,						///< mod
	kNPS_SHIFT_LEFT,				///< <<
	kNPS_SHIFT_RIGHT,				///< >>
	kNPS_NOT_EQUAL,					///< <>
	kNPS_GREATER_EQUAL,				///< >=
	kNPS_LESS_EQUAL,				///< <=
	kNPS_OBJECT_EQUAL,				///< ==
	kNPS_CONCAT2,					///< &&
};

/// パーサ用疑似命令
enum {
    kNPSPop					= 000,	///< 000 pop
    kNPSDup					= 001,	///< 001 dup
    kNPSReturn				= 002,	///< 002 return
    kNPSPushSelf			= 003,	///< 003 push-self
    kNPSSetLexScope			= 004,	///< 004 set-lex-scope
    kNPSIterNext			= 005,	///< 005 iter-next
    kNPSIterDone			= 006,	///< 006 iter-done
    kNPSPopHandlers			= 007,	///< 007 000 001 pop-handlers

    kNPSPush				= 0030,	///< 03x push
    kNPSPushConstant		= 0040,	///< 04x (B signed) push-constant
    kNPSCall				= 0050,	///< 05x call
    kNPSInvoke				= 0060,	///< 06x invoke
    kNPSSend				= 0070,	///< 07x send
    kNPSSendIfDefined		= 0100,	///< 10x send-if-defined
    kNPSResend				= 0110,	///< 11x resend
    kNPSResendIfDefined		= 0120,	///< 12x resend-if-defined
    kNPSBranch				= 0130,	///< 13x branch
    kNPSBranchIfTrue		= 0140,	///< 14x branch-if-true
    kNPSBranchIfFalse		= 0150,	///< 15x branch-if-false
    kNPSFindVar				= 0160,	///< 16x find-var
    kNPSGetVar				= 0170,	///< 17x get-var
    kNPSMakeFrame			= 0200,	///< 20x make-frame
    kNPSMakeArray			= 0210,	///< 21x make-array
    kNPSGetPath				= 0220,	///< 220/221 get-path
    kNPSSetPath				= 0230,	///< 230/231 set-path
    kNPSSetVar				= 0240,	///< 24x set-var
    kNPSFindAndSetVar		= 0250,	///< 25x find-and-set-var
    kNPSIncrVar				= 0260,	///< 26x incr-var
    kNPSBranchIfLoopNotDone	= 0270,	///< 27x branch-if-loop-not-done
    kNPSFreqFunc			= 0300,	///< 30x freq-func
    kNPSNewHandlers			= 0310,	///< 31x new-handlers

    // 30x freq-func
    kNPSAdd					= 03000,///<  0 add					|+|
    kNPSSubtract,					///<  1 subtract			|-|
    kNPSAref,						///<  2 aref				aref
    kNPSSetAref,					///<  3 set-aref			setAref
    kNPSEquals,						///<  4 equals				|=|
    kNPSNot,						///<  5 not					|not|
    kNPSNotEqual,					///<  6 not-equals			|<>|
    kNPSMultiply,					///<  7 multiply			|*|
    kNPSDivide,						///<  8 divide				|/|
    kNPSDiv,						///<  9 div					|div|
    kNPSLessThan,					///< 10 less-than			|<|
    kNPSGreaterThan,				///< 11 greater-than		|>|
    kNPSGreaterOrEqual,				///< 12 greater-or-equal	|>=|
    kNPSLessOrEqual,				///< 13 less-or-equal		|<=|
    kNPSBitAnd,						///< 14 bit-and				BAnd
    kNPSBitOr,						///< 15 bit-or				BOr
    kNPSBitNot,						///< 16 bit-not				BNot
    kNPSNewIterator,				///< 17 new-iterator		newIterator
    kNPSLength,						///< 18 length				Length
    kNPSClone,						///< 19 clone				Clone
    kNPSSetClass,					///< 20 set-class			SetClass
    kNPSAddArraySlot,				///< 21 add-array-slot		AddArraySlot
    kNPSStringer,					///< 22 stringer			Stringer
    kNPSHasPath,					///< 23 has-path			none
    kNPSClassOf,					///< 24 class-of			ClassOf

    // 40x syntax
    kNPSConstituentList		= 04000,
    kNPSCommaList,
    kNPSConstant,
    kNPSGlobal,
    kNPSLocal,
    kNPSGlobalFn,
    kNPSFunc,
    kNPSArg,
	kNPSIndefinite,
    kNPSMessage,
    kNPSLvalue,
    kNPSAssign,
    kNPSExists,
    kNPSMethodExists,
    kNPSTry,
    kNPSOnexception,
    kNPSOnexceptionList,
    kNPSIf,
    kNPSLoop,
    kNPSFor,
    kNPSForeach,
    kNPSWhile,
    kNPSRepeat,
    kNPSBreak,
    kNPSSlot,
    kNPSConcat,								///< &
    kNPSConcat2,							///< &&
    kNPSAnd,								///< and
    kNPSOr,									///< or

    // function
    kNPSMod,								///< mod
	kNPSShiftLeft,							///< <<
	kNPSShiftRight,							///< >>

	// 独自拡張
	kNPSObjectEqual,						///< ==
	kNPSMakeRegex,							///< 正規表現オブジェクトの生成

    // Unknown
    KNPSUnknownCode		= 0xffffffff		///< 不明な命令
};


/// 構文木データの種類
enum {
    kNPSKindNone,		///< データなし
    kNPSKindLink,		///< リンク
    kNPSKindObject		///< オブジェクト
};


/* 型宣言 */

/// 構文木ノードへのポインタ
typedef newtRef		nps_node_t;


/// 構文木ノード
typedef struct {
    uint32_t	code;   ///< 命令コード
    nps_node_t	op1;	///< オペコード1
    nps_node_t	op2;	///< オペコード2
} nps_syntax_node_t;


/// パーサ環境
typedef struct {
	bool			first_time;			///< 字句解析の初回判別用フラグ

	uint16_t		numwarns;			///< 発生したワーニング数
	uint16_t		numerrs;			///< 発生したエラー数

	const char *	fname;				///< 入力中のファイル名
	uint32_t		lineno;				///< 字句解析の行番号
	uint32_t		tokenpos;			///< トークンの位置
	uint16_t		yyleng;				///< トークンの長さ
	char			linebuf[NEWT_LEX_LINEBUFFSIZE];  ///< 行バッファ
} nps_env_t;


/* グローバル変数 */
extern nps_env_t	nps_env;		///< パーサ環境


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


int			nps_yyinput(FILE * yyin, char * buff, int max_size);
void		nps_yyinit(void);
int			nps_yycleanup(void);


newtErr		NPSParse(const char * path, nps_syntax_node_t ** streeP, uint32_t * sizeP, bool is_file);

newtErr		NPSParseFile(const char * path,
                    nps_syntax_node_t ** streeP, uint32_t * sizeP);

newtErr		NPSParseStr(const char * s,
                    nps_syntax_node_t ** streeP, uint32_t * sizeP);

void		NPSCleanup(void);
void		NPSDumpSyntaxTree(FILE * f, nps_syntax_node_t * stree, uint32_t size);

//nps_node_t	NPSGetCX(void);

nps_node_t	NPSGenNode0(uint32_t code);
nps_node_t	NPSGenNode1(uint32_t code, nps_node_t op1);
nps_node_t	NPSGenNode2(uint32_t code, nps_node_t op1, nps_node_t op2);

nps_node_t	NPSGenOP1(uint32_t op, nps_node_t op1);
nps_node_t	NPSGenOP2(uint32_t op, nps_node_t op1, nps_node_t op2);

nps_node_t	NPSGenSend(nps_node_t receiver,
                    uint32_t op, nps_node_t msg, nps_node_t args);
nps_node_t	NPSGenResend(uint32_t op, nps_node_t msg, nps_node_t args);

nps_node_t	NPSGenIfThenElse(nps_node_t cond, nps_node_t ifthen, nps_node_t ifelse);
nps_node_t	NPSGenForLoop(nps_node_t index, nps_node_t v,
                    nps_node_t to, nps_node_t by, nps_node_t expr);
nps_node_t	NPSGenForeach(nps_node_t index, nps_node_t val, nps_node_t obj,
                    nps_node_t deeply, nps_node_t op, nps_node_t expr);
nps_node_t	NPSGenGlobalFn(nps_node_t name, nps_node_t args, nps_node_t expr);

newtRef		NPSMakePathExpr(newtRefArg sym1, newtRefArg sym2);
newtRef		NPSMakeArray(newtRefArg v);
newtRef		NPSAddArraySlot(newtRefArg r, newtRefArg v);
newtRef		NPSInsertArraySlot(newtRefArg r, uint32_t p, newtRefArg v);

newtRef		NPSMakeMap(newtRefArg v);
newtRef		NPSMakeFrame(newtRefArg slot, newtRefArg v);
newtRef		NPSSetSlot(newtRefArg r, newtRefArg slot, newtRefArg v);

newtRef		NPSMakeBinary(newtRefArg v);
newtRef		NPSAddARef(newtRefArg r, newtRefArg v);

void		NPSErrorStr(char c, char * s);
void		NPSError(int32_t err);


#ifdef __cplusplus
}
#endif


#endif /* NEWTPARSE_H */
