/*------------------------------------------------------------------------*/
/**
 * @file	NewtVM.h
 * @brief   VM
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTVM_H
#define	NEWTVM_H


/* ヘッダファイル */
#include <stdio.h>
#include "NewtType.h"
#include "NewtMem.h"


/* 定数 */

/// イテレータの要素位置
enum {
    kIterIndex		= 0,	///< 繰り返し中の位置
    kIterValue,				///< 値
    kIterObj,				///< オブジェクト
    kIterDeeply,			///< deeply フラグ
    kIterPos,				///< オブジェクトの位置
    kIterMax,				///< オブジェクトの長さ
    kIterMap,				///< frameオブジェクトのマップ

    //
    kIterALength			///< イテレータ配列の長さ
};


/// VM レジスタ
typedef struct {
    newtRefVar	func;	///< FUNC   実行中の関数オブジェクト
    uint32_t	pc;		///< PC     実行中の instructionオブジェクトのインデックス
    uint32_t	sp;		///< SP     スタックポインタ
    newtRefVar	locals;	///< LOCALS 実行中のローカルフレーム
    newtRefVar	rcvr;	///< RCVR   実行中のレシーバ（for メッセージ送信）
    newtRefVar	impl;	///< IMPL   実行中のインプリメンタ(for メッセージ送信)
} vm_reg_t;


/// 例外ハンドラ
typedef struct {
    uint32_t	callsp;		///< 呼出しスタックのスタックポインタ
    uint32_t	excppc;		///< 例外ハンドラを作成したときのプログラムカウンタ

    newtRefVar	sym;		///< シンボル
    uint32_t	pc;			///< プログラムカウンタ
} vm_excp_t;


/// VM 実行環境
typedef struct {
    // バイトコード
    uint8_t *	bc;			///< バイトコード
    uint32_t	bclen;		///< バイトコードの長さ

    // レジスタ
    vm_reg_t	reg;		///< レジスタ

    // スタック
    newtStack	stack;		///< スタック
    newtStack	callstack;	///< 関数呼出しスタック
    newtStack	excpstack;	///< 例外ハンドラ・スタック

    // 例外
    newtRefVar	currexcp;   ///< 現在の例外

	// VM 管理
	uint16_t	level;		///< VM呼出しレベル
} vm_env_t;


extern vm_env_t		vm_env; ///< VM 実行環境


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


newtRef		NVMSelf(void);
newtRef		NVMCurrentFunction(void);
newtRef		NVMCurrentImplementor(void);
bool		NVMHasVar(newtRefArg name);
void		NVMThrowData(newtRefArg name, newtRefArg data);
void		NVMThrow(newtRefArg name, newtRefArg data);
void		NVMRethrow(void);
newtRef		NVMCurrentException(void);
void		NVMClearException(void);

bool		NVMFuncCheckNumArgs(newtRefArg fn, int16_t numArgs);

void		NVMDumpInstName(FILE * f, uint8_t a, int16_t b);
void		NVMDumpCode(FILE * f, uint8_t * bc, uint32_t len);
void		NVMDumpBC(FILE * f, newtRefArg instructions);
void		NVMDumpFn(FILE * f, newtRefArg fn);
void		NVMDumpStackTop(FILE * f, char * s);
void		NVMDumpStacks(FILE * f);

void		NVMFnCall(newtRefArg fn, int16_t numArgs);
newtRef		NVMInterpret(newtRefArg fn, newtErr * errP);
newtErr		NVMInfo(const char * name);

newtRef		NVMCall(newtRefArg fn, int16_t numArgs, newtErr * errP);
newtRef		NVMInterpretFile(const char * path, newtErr * errP);
newtRef		NVMInterpretStr(const char * s, newtErr * errP);

newtRef		NVMMessageSendWithArgArray(newtRefArg inImpl, newtRefArg inRcvr,
				newtRefArg inFunction, newtRefArg inArgs);

#ifdef __cplusplus
}
#endif


#endif /* NEWTVM_H */
