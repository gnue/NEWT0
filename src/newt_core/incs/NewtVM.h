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


/* マクロ */
#define NcCall0(fn)													NcCall(fn, 0)
#define NcCall1(fn, a1)												NcCall(fn, 1, a1)
#define NcCall2(fn, a1, a2)											NcCall(fn, 2, a1, a2)
#define NcCall3(fn, a1, a2, a3)										NcCall(fn, 3, a1, a2, a3)
#define NcCall4(fn, a1, a2, a3, a4)									NcCall(fn, 4, a1, a2, a3, a4)
#define NcCall5(fn, a1, a2, a3, a4, a5)								NcCall(fn, 5, a1, a2, a3, a4, a5)

#define NcCallGlobalFn0(sym)										NcCallGlobalFn(sym, 0)
#define NcCallGlobalFn1(sym, a1)									NcCallGlobalFn(sym, 1, a1)
#define NcCallGlobalFn2(sym, a1, a2)								NcCallGlobalFn(sym, 2, a1, a2)
#define NcCallGlobalFn3(sym, a1, a2, a3)							NcCallGlobalFn(sym, 3, a1, a2, a3)
#define NcCallGlobalFn4(sym, a1, a2, a3, a4)						NcCallGlobalFn(sym, 4, a1, a2, a3, a4)
#define NcCallGlobalFn5(sym, a1, a2, a3, a4, a5)					NcCallGlobalFn(sym, 5, a1, a2, a3, a4, a5)

#define NcSend0(receiver, sym)										NcSend(receiver, sym, false, 0)
#define NcSend1(receiver, sym, a1)									NcSend(receiver, sym, false, 1, a1)
#define NcSend2(receiver, sym, a1, a2)								NcSend(receiver, sym, false, 2, a1, a2)
#define NcSend3(receiver, sym, a1, a2, a3)							NcSend(receiver, sym, false, 3, a1, a2, a3)
#define NcSend4(receiver, sym, a1, a2, a3, a4)						NcSend(receiver, sym, false, 4, a1, a2, a3, a4)
#define NcSend5(receiver, sym, a1, a2, a3, a4, a5)					NcSend(receiver, sym, false, 5, a1, a2, a3, a4, a5)

#define NcSendIfDefined0(receiver, sym)								NcSend(receiver, sym, true, 0)
#define NcSendIfDefined1(receiver, sym, a1)							NcSend(receiver, sym, true, 1, a1)
#define NcSendIfDefined2(receiver, sym, a1, a2)						NcSend(receiver, sym, true, 2, a1, a2)
#define NcSendIfDefined3(receiver, sym, a1, a2, a3)					NcSend(receiver, sym, true, 3, a1, a2, a3)
#define NcSendIfDefined4(receiver, sym, a1, a2, a3, a4)				NcSend(receiver, sym, true, 4, a1, a2, a3, a4)
#define NcSendIfDefined5(receiver, sym, a1, a2, a3, a4, a5)			NcSend(receiver, sym, true, 5, a1, a2, a3, a4, a5)

#define NcSendProto0(receiver, sym)									NcSendProto(receiver, sym, false, 0)
#define NcSendProto1(receiver, sym, a1)								NcSendProto(receiver, sym, false, 1, a1)
#define NcSendProto2(receiver, sym, a1, a2)							NcSendProto(receiver, sym, false, 2, a1, a2)
#define NcSendProto3(receiver, sym, a1, a2, a3)						NcSendProto(receiver, sym, false, 3, a1, a2, a3)
#define NcSendProto4(receiver, sym, a1, a2, a3, a4)					NcSendProto(receiver, sym, false, 4, a1, a2, a3, a4)
#define NcSendProto5(receiver, sym, a1, a2, a3, a4, a5)				NcSendProto(receiver, sym, false, 5, a1, a2, a3, a4, a5)

#define NcSendProtoIfDefined0(receiver, sym)						NcSendProto(receiver, sym, true, 0)
#define NcSendProtoIfDefined1(receiver, sym, a1)					NcSendProto(receiver, sym, true, 1, a1)
#define NcSendProtoIfDefined2(receiver, sym, a1, a2)				NcSendProto(receiver, sym, true, 2, a1, a2)
#define NcSendProtoIfDefined3(receiver, sym, a1, a2, a3)			NcSendProto(receiver, sym, true, 3, a1, a2, a3)
#define NcSendProtoIfDefined4(receiver, sym, a1, a2, a3, a4)		NcSendProto(receiver, sym, true, 4, a1, a2, a3, a4)
#define NcSendProtoIfDefined5(receiver, sym, a1, a2, a3, a4, a5)	NcSendProto(receiver, sym, true, 5, a1, a2, a3, a4, a5)


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
typedef struct vm_env_t {
    // バイトコード
    uint8_t *	bc;				///< バイトコード
    uint32_t	bclen;			///< バイトコードの長さ

    // レジスタ
    vm_reg_t	reg;			///< レジスタ

    // スタック
    newtStack	stack;			///< スタック
    newtStack	callstack;		///< 関数呼出しスタック
    newtStack	excpstack;		///< 例外ハンドラ・スタック

    // 例外
    newtRefVar	currexcp;		///< 現在の例外

	// VM 管理
	uint16_t	level;			///< VM呼出しレベル
	struct vm_env_t *	next;	///< VM 実行環境のチェイン
} vm_env_t;


extern vm_env_t		vm_env; ///< VM 実行環境


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


void			NVMInit(void);
void			NVMClean(void);
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

newtRef		NcCall(newtRefArg fn, int argc, ...);
newtRef		NcCallWithArgArray(newtRefArg fn, newtRefArg args);
newtRef		NcCallGlobalFn(newtRefArg sym, int argc, ...);
newtRef		NcCallGlobalFnWithArgArray(newtRefArg sym, newtRefArg args);

newtRef		NcSend(newtRefArg receiver, newtRefArg sym, bool ignore, int argc, ...);
newtRef		NcSendWithArgArray(newtRefArg receiver, newtRefArg sym, bool ignore, newtRefArg args);
newtRef		NcSendProto(newtRefArg receiver, newtRefArg sym, bool ignore, int argc, ...);
newtRef		NcSendProtoWithArgArray(newtRefArg receiver, newtRefArg sym, bool ignore, newtRefArg args);


#ifdef __cplusplus
}
#endif


#endif /* NEWTVM_H */
