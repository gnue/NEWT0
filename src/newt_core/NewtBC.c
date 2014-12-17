/*------------------------------------------------------------------------*/
/**
 * @file	NewtBC.c
 * @brief   バイトコードの生成
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <stdio.h>
#include <stdlib.h>

#include "NewtCore.h"
#include "NewtBC.h"
#include "NewtVM.h"
#include "NewtIO.h"
#include "NewtMem.h"


/* 型宣言 */

/// バイトコード環境
typedef struct nbc_env_t	nbc_env_t;

/// バイトコード環境（構造体定義）
struct nbc_env_t {
    nbc_env_t *	parent;			///< 呼出し元環境

	newtStack   bytecode;		///< バイトコードバッファ
	newtStack   breakstack;		///< ブレークスタック
	newtStack   onexcpstack;	///< 例外スタック

    newtRefVar	func;			///< 関数オブジェクト
    newtRefVar	literals;		///< 関数オブジェクトのリテラルフレーム
    newtRefVar	argFrame;		///< 関数オブジェクトのフレーム
    newtRefVar	constant;		///< 定数フレーム
};


/// 関数命令テーブル構造体
typedef struct {
    char *		name;		///< 関数名
    int32_t		numArgs;	///< 引数の数
    int16_t		b;			///< バイトコード
    newtRefVar	sym;		///< シンボル
} freq_func_t;


/* 関数プロトタイプ */
#define	ENV_BC(env)				((uint8_t*)env->bytecode.stackp)				///< バイトコード
#define	ENV_CX(env)				(env->bytecode.sp)								///< コードインデックス（プログラムカウンタ）
#define	BC						ENV_BC(newt_bc_env)								///< 作成中のバイトコード
#define	CX						ENV_CX(newt_bc_env)								///< 作成中のコードインデックス（プログラムカウンタ）
#define	BREAKSTACK				((uint32_t*)newt_bc_env->breakstack.stackp)		///< ブレークスタック
#define BREAKSP					(newt_bc_env->breakstack.sp)					///< ブレークスタックのスタックポインタ
#define	ONEXCPSTACK				((uint32_t*)newt_bc_env->onexcpstack.stackp)	///< 例外スタック
#define ONEXCPSP				(newt_bc_env->onexcpstack.sp)					///< 例外スタックのスタックポインタ
#define	LITERALS				(newt_bc_env->literals)							///< 作成中関数オブジェクトのリテラル
#define	ARGFRAME				(newt_bc_env->argFrame)							///< 作成中関数オブジェクトの引数フレーム
#define	CONSTANT				(newt_bc_env->constant)							///< 定数フレーム

#define NBCAddLiteral(r)		NBCAddLiteralEnv(newt_bc_env, r)				///< リテラルリストにオブジェクトを追加
#define NBCGenCode(a, b)		NBCGenCodeEnv(newt_bc_env, a, b)				///< バイトコードを生成
#define NBCGenCodeL(a, r)		NBCGenCodeEnvL(newt_bc_env, a, r)				///< リテラルなオペデータのバイトコードを生成
#define NBCGenPushLiteral(r)	NBCGenPushLiteralEnv(newt_bc_env, r)			///< リテラルをプッシュするバイトコードを生成

#define NBCGenBC_op(stree, r)   NBCGenBC_stmt(stree, r, true)					///< 引数のバイトコードを生成する
#define NBCGenFreq(b)			NBCGenCode(kNBCFreqFunc, b)						///< 関数命令のバイトコードを生成する


#if 0
#pragma mark -
#pragma mark *** ローカル変数
#endif


/* ローカル変数 */

/// バイドコード環境
static nbc_env_t *	newt_bc_env;

/// 関数命令テーブル
static freq_func_t freq_func_tb[] =
    {
//		{"aref",			2,	kNBCAref,			0},
//		{"setAref",			3,	kNBCSetAref,		0},
        {"BAnd",			2,	kNBCBitAnd,			0},
        {"BOr",				2,	kNBCBitOr,			0},
        {"BNot",			1,	kNBCBitNot,			0},
        {"Length",			1,	kNBCLength,			0},
        {"Clone",			1,	kNBCClone,			0},
        {"SetClass",		2,	kNBCSetClass,		0},
        {"AddArraySlot",	2,	kNBCAddArraySlot,	0},
        {"Stringer",		1,	kNBCStringer,		0},
        {"ClassOf",			1,	kNBCClassOf,		0},

        // end
        {NULL,				0,	0,					0}
    };


#if 0
#pragma mark -
#endif

/* 関数プロトタイプ */
static int16_t			NBCAddLiteralEnv(nbc_env_t * env, newtRefArg r);
static void				NBCGenCodeEnv(nbc_env_t * env, uint8_t a, int16_t b);
static void				NBCGenCodeEnvL(nbc_env_t * env, uint8_t a, newtRefArg r);
static int16_t			NBCGenPushLiteralEnv(nbc_env_t * env, newtRefArg r);

static void				NBCGenPUSH(newtRefArg r);
static void				NBCGenGetVar(nps_syntax_node_t * stree, newtRefArg r);
static void				NBCGenCallFn(newtRefArg fn, int16_t numArgs);
static int16_t			NBCMakeFnArgFrame(newtRefArg argFrame, nps_syntax_node_t * stree, nps_node_t r, bool * indefiniteP);
static int16_t			NBCMakeFnArgs(newtRefArg fn, nps_syntax_node_t * stree, nps_node_t r);
static nbc_env_t *		NBCMakeFnEnv(nps_syntax_node_t * stree, nps_node_t args);
static uint32_t			NBCGenBranch(uint8_t a);
static void				NBCDefLocal(newtRefArg type, newtRefArg r, bool init);
static void				NBCBackPatch(uint32_t cx, int16_t b);
static void				NBCPushBreakStack(uint32_t cx);
static void				NBCBreakBackPatchs(uint32_t loop_head, uint32_t cx);
static void				NBCPushOnexcpStack(uint32_t cx);
static void				NBCOnexcpBackPatchs(uint32_t try_head, uint32_t cx);
static void				NBCGenOnexcpPC(int32_t pc);
static void				NBCGenOnexcpBranch(void);
static void				NBCOnexcpBackPatchL(uint32_t sp, int32_t pc);

static newtRef			NBCMakeFn(nbc_env_t * env);
static void				NBCInitFreqFuncTable(void);
static nbc_env_t *		NBCEnvNew(nbc_env_t * parent);
static void				NBCEnvFree(nbc_env_t * env);
static newtRef			NBCFnDone(nbc_env_t ** envP);

static void				NBCGenBC_stmt(nps_syntax_node_t * stree, nps_node_t r, bool ret);
static void				NBCGenConstant(nps_syntax_node_t * stree, nps_node_t r);
static void				NBCGenGlobalVar(nps_syntax_node_t * stree, nps_node_t r);
static void				NBCGenLocalVar(nps_syntax_node_t * stree, nps_node_t type, nps_node_t r);
static bool				NBCTypeValid(nps_node_t type);
static int16_t			NBCGenTryPre(nps_syntax_node_t * stree, nps_node_t r);
static int16_t			NBCGenTryPost(nps_syntax_node_t * stree, nps_node_t r, uint32_t * onexcpspP);
static void				NBCGenTry(nps_syntax_node_t * stree, nps_node_t expr, nps_node_t onexception_list);
static void				NBCGenIfThenElse(nps_syntax_node_t * stree, nps_node_t cond, nps_node_t thenelse, bool ret);
static void				NBCGenAnd(nps_syntax_node_t * stree, nps_node_t op1, nps_node_t op2);
static void				NBCGenOr(nps_syntax_node_t * stree, nps_node_t op1, nps_node_t op2);
static void				NBCGenLoop(nps_syntax_node_t * stree, nps_node_t expr);
static newtRef			NBCMakeTempSymbol(newtRefArg index, newtRefArg val, char * s);
static void				NBCGenFor(nps_syntax_node_t * stree, nps_node_t r, nps_node_t expr);
static void				NBCGenForeach(nps_syntax_node_t * stree, nps_node_t r, nps_node_t expr);
static void				NBCGenWhile(nps_syntax_node_t * stree, nps_node_t cond, nps_node_t expr);
static void				NBCGenRepeat(nps_syntax_node_t * stree, nps_node_t expr, nps_node_t cond);
static void				NBCGenBreak(nps_syntax_node_t * stree, nps_node_t expr);
static void				NBCGenStringer(nps_syntax_node_t * stree, nps_node_t op1, nps_node_t op2, char * dlmt);
static void				NBCGenAssign(nps_syntax_node_t * stree, nps_node_t lvalue, nps_node_t expr, bool ret);
static void				NBCGenExists(nps_syntax_node_t * stree, nps_node_t r);
static void				NBCGenReceiver(nps_syntax_node_t * stree, nps_node_t r);
static void				NBCGenMethodExists(nps_syntax_node_t * stree, nps_node_t receiver, nps_node_t name);
static void				NBCGenFn(nps_syntax_node_t * stree, nps_node_t args, nps_node_t expr);
static void				NBCGenGlobalFn(nps_syntax_node_t * stree, nps_node_t name, nps_node_t fn);
static void				NBCGenCall(nps_syntax_node_t * stree, nps_node_t name, nps_node_t args);
static void				NBCGenInvoke(nps_syntax_node_t * stree, nps_node_t fn, nps_node_t args);
static void				NBCGenFunc2(nps_syntax_node_t * stree, newtRefArg name, nps_node_t op1, nps_node_t op2);
static void				NBCGenSend(nps_syntax_node_t * stree, uint32_t code, nps_node_t receiver, nps_node_t r);
static void				NBCGenResend(nps_syntax_node_t * stree, uint32_t code, nps_node_t name, nps_node_t args);
static void				NBCGenMakeArray(nps_syntax_node_t * stree, nps_node_t klass, nps_node_t r);
static void				NBCGenMakeFrame(nps_syntax_node_t * stree, nps_node_t r);
static void				NVCGenNoResult(bool ret);
static void				NBCGenSyntaxCode(nps_syntax_node_t * stree, nps_syntax_node_t * node, bool ret);
static int16_t			NBCCountNumArgs(nps_syntax_node_t * stree, nps_node_t r);
static newtRef			NBCGenMakeFrameSlots_sub(nps_syntax_node_t * stree, nps_node_t r);
static newtRef			NBCGenMakeFrameSlots(nps_syntax_node_t * stree, nps_node_t r);
static void				NBCGenBC_sub(nps_syntax_node_t * stree, uint32_t n, bool ret);


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** リテラルリストにオブジェクトを追加する
 *
 * @param env		[in] バイトコード環境
 * @param r			[in] オブジェクト
 *
 * @return			追加された位置
 */

int16_t NBCAddLiteralEnv(nbc_env_t * env, newtRefArg r)
{
    int16_t	b;

    b = NewtArrayLength(env->literals);
    NcAddArraySlot(env->literals, r);

    return b;
}


/*------------------------------------------------------------------------*/
/** バイトコードを生成
 *
 * @param env		[in] バイトコード環境
 * @param a			[in] 命令
 * @param b			[in] オペデータ
 *
 * @return			なし
 */

void NBCGenCodeEnv(nbc_env_t * env, uint8_t a, int16_t b)
{
    uint8_t *	bc;
    uint32_t	cx;

	cx = ENV_CX(env);

	if (! NewtStackExpand(&env->bytecode, cx + 3))
		return;

	bc = ENV_BC(env);

    if (a == kNBCFieldMask)
        b = 1;

    if (a != kNBCFieldMask &&
        ((a & kNBCFieldMask) == a ||
        (b != kNBCFieldMask && (b & kNBCFieldMask) == b)))
    {
        bc[cx++] = a | b;
    }
    else
    {
        bc[cx++] = a | kNBCFieldMask;
        bc[cx++] = b >> 8;
        bc[cx++] = b & 0xff;
    }

	ENV_CX(env) = cx;
}


/*------------------------------------------------------------------------*/
/** リテラルなオペデータのバイトコードを生成
 *
 * @param env		[in] バイトコード環境
 * @param a			[in] 命令
 * @param r			[in] オブジェクト
 *
 * @return			なし
 */

void NBCGenCodeEnvL(nbc_env_t * env, uint8_t a, newtRefArg r)
{
    newtRefVar	obj;
    int16_t	b = 0;

    obj = NewtPackLiteral(r);

    // リテラルを検索
    b = NewtFindArrayIndex(env->literals, obj, 0);

    if (b == -1) // リテラルに追加
        b = NBCAddLiteralEnv(env, obj);

    NBCGenCodeEnv(env, a, b);
}


/*------------------------------------------------------------------------*/
/** リテラルをプッシュするバイトコードを生成
 *
 * @param env		[in] バイトコード環境
 * @param r			[in] オブジェクト
 *
 * @return			リテラルリストの位置
 */

int16_t NBCGenPushLiteralEnv(nbc_env_t * env, newtRefArg r)
{
    newtRefVar	obj;
    int16_t	b;

    obj = NewtPackLiteral(r);
    b = NBCAddLiteralEnv(env, obj);

    NBCGenCodeEnv(env, kNBCPush, b);

    return b;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** オブジェクトをプッシュするバイトコードを生成
 *
 * @param r			[in] オブジェクト
 *
 * @return			なし
 */

void NBCGenPUSH(newtRefArg r)
{
    switch (NewtGetRefType(r, false))
    {
        case kNewtInt30:
            {
                int32_t	n;

                n = NewtRefToInteger(r);

                if (-8192 <= n && n <= 8191)
                    NBCGenCode(kNBCPushConstant, r);
                else
                    NBCGenCodeL(kNBCPush, r);
            }
            break;

        case kNewtNil:
        case kNewtTrue:
        case kNewtUnbind:
            NBCGenCode(kNBCPushConstant, r);
            break;

        case kNewtCharacter:
        case kNewtSpecial:
        case kNewtMagicPointer:
            if ((r & 0xffff0000) == 0)
                NBCGenCode(kNBCPushConstant, r);
            else
                NBCGenCodeL(kNBCPush, r);
            break;

        case kNewtPointer:
        default:
            NBCGenCodeL(kNBCPush, r);
            break;
    }
}


/*------------------------------------------------------------------------*/
/** 変数を取得するバイトコードを生成
 *
 * @param stree		[in] 構文木
 * @param r			[in] 変数名オブジェクト
 *
 * @return			なし
 */

void NBCGenGetVar(nps_syntax_node_t * stree, newtRefArg r)
{
    if (NewtHasSlot(CONSTANT, r))
    {
        // 定数の場合
        newtRefVar	c;

        c = NcGetSlot(CONSTANT, r);

        if (! NPSRefIsSyntaxNode(c) && NewtRefIsLiteral(c))
            NBCGenPUSH(c);
        else
            NBCGenBC_op(stree, c);
    }
    else
    {
        int16_t	b;

        // ローカル変数を検索
        b = NewtFindSlotIndex(ARGFRAME, r);
    
        if (b != -1)
            NBCGenCode(kNBCGetVar, b);
        else
            NBCGenCodeL(kNBCFindVar, r);
    }
}


/*------------------------------------------------------------------------*/
/** 関数呼出しのバイトコードを生成
 *
 * @param fn		[in] 関数オブジェクト
 * @param numArgs	[in] 引数の数
 *
 * @return			なし
 */

void NBCGenCallFn(newtRefArg fn, int16_t numArgs)
{
    int	i;

    // freq-func の場合
    for (i = 0; freq_func_tb[i].name != NULL; i++)
    {
        if (NewtRefEqual(fn, freq_func_tb[i].sym))
        {
            if (numArgs != freq_func_tb[i].numArgs)
            {
                NBError(kNErrWrongNumberOfArgs);
                return;
            }

            NBCGenFreq(freq_func_tb[i].b);
            return;
        }
    }

    // call function
    NBCGenPUSH(fn);
    NBCGenCode(kNBCCall, numArgs);
}


/*------------------------------------------------------------------------*/
/** 関数の引数フレームを作成する
 *
 * @param argFrame		[in] 引数フレーム
 * @param stree			[in] 構文木
 * @param r				[in] 構文木ノード
 * @param indefiniteP	[out]不定長フラグ
 *
 * @return			引数の数
 */

int16_t NBCMakeFnArgFrame(newtRefArg argFrame, nps_syntax_node_t * stree, nps_node_t r, bool * indefiniteP)
{
    int16_t	numArgs = 1;

    if (r == kNewtRefUnbind)
        return 0;

	if (*indefiniteP == true)
	{
		NBError(kNErrSyntaxError);
        return 0;
	}

    if (NPSRefIsSyntaxNode(r))
    {
        nps_syntax_node_t * node;

        node = &stree[NPSRefToSyntaxNode(r)];

        switch (node->code)
        {
            case kNPSCommaList:
                numArgs = NBCMakeFnArgFrame(argFrame, stree, node->op1, indefiniteP)
                            + NBCMakeFnArgFrame(argFrame, stree, node->op2, indefiniteP);
                break;

            case kNPSArg:
                // type (node->op1) はとりあえず無視
                NcSetSlot(argFrame, node->op2, kNewtRefUnbind);
                break;

			case kNPSIndefinite:
                // 不定長
				NcSetSlot(argFrame, node->op1, kNewtRefUnbind);
				*indefiniteP = true;
                numArgs = 0;
				break;

            default:
                NBError(kNErrSyntaxError);
                break;
        }
    }
    else
    {
        NcSetSlot(argFrame, r, kNewtRefUnbind);
    }

    return numArgs;
}


/*------------------------------------------------------------------------*/
/** 関数オブジェクトの引数を作成する
 *
 * @param fn		[in] 関数オブジェクト
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 *
 * @return			引数の数
 */

int16_t NBCMakeFnArgs(newtRefArg fn, nps_syntax_node_t * stree, nps_node_t r)
{
    int16_t	numArgs = 0;

    if (r != kNewtRefUnbind)
    {
        newtRefVar	argFrame;
		bool		indefinite = false;

        argFrame = NcGetSlot(fn, NSSYM0(argFrame));
        numArgs = NBCMakeFnArgFrame(argFrame, stree, r, &indefinite);

        if (0 < numArgs)
            NcSetSlot(fn, NSSYM0(numArgs), NewtMakeInteger(numArgs));
 
		if (indefinite)
            NcSetSlot(fn, NSSYM0(indefinite), kNewtRefTRUE);
	}

    return numArgs;
}


/*------------------------------------------------------------------------*/
/** 関数のバイトコード環境を作成する
 *
 * @param stree		[in] 構文木
 * @param args		[in] 引数
 *
 * @return			バイトコード環境
 */

nbc_env_t * NBCMakeFnEnv(nps_syntax_node_t * stree, nps_node_t args)
{
    newt_bc_env = NBCEnvNew(newt_bc_env);

	if (newt_bc_env != NULL)
		NBCMakeFnArgs(newt_bc_env->func, stree, args);

    return newt_bc_env;
}


/*------------------------------------------------------------------------*/
/** 分岐命令のバイトコードを生成
 *
 * @param a			[in] 命令
 *
 * @return			バイトコードの位置
 */

uint32_t NBCGenBranch(uint8_t a)
{
    uint32_t	cx;

    cx = CX;
    NBCGenCode(a, 0xffff);	// 0xffff is dummy

    return cx;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** ローカル変数を定義するバイトコードを生成
 *
 * @param type		[in] 変数の型
 * @param r			[in] 変数名シンボル
 * @param init		[in] 初期化
 *
 * @return			なし
 *
 * @note			もし init が true ならば初期化するバイトコードを生成する
 *					現在のバージョンでは type は完全に無視される
 */

void NBCDefLocal(newtRefArg type, newtRefArg r, bool init)
{
    NcSetSlot(ARGFRAME, r, kNewtRefUnbind);

    if (init)
    {
        int16_t	b;
    
        // ローカル変数を検索
        b = NewtFindSlotIndex(ARGFRAME, r);

        if (b != -1)
            NBCGenCode(kNBCSetVar, b);
        else
            NewtFprintf(stderr, "Not inpriment");
    }
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** バイトコードをバックパッチする
 *
 * @param cx		[in] バックパッチする位置
 * @param b			[in] バックパッチするオペデータ
 *
 * @return			なし
 *
 *　@note				分岐命令やループ命令などすぐにオペデータが決定しない場合に使う
 */

void NBCBackPatch(uint32_t cx, int16_t b)
{
    BC[cx + 1] = b >> 8;
    BC[cx + 2] = b & 0xff;
}


/*------------------------------------------------------------------------*/
/** ブレーク命令の位置をスタックする
 *
 * @param cx		[in] ブレーク命令の位置
 *
 * @return			なし
 *
 *　@note				バックパッチのために覚えておく
 */

void NBCPushBreakStack(uint32_t cx)
{
    if (BREAKSTACK == NULL)
		NewtStackSetup(&newt_bc_env->breakstack, NEWT_POOL, sizeof(uint32_t), NEWT_NUM_BREAKSTACK);

	if (! NewtStackExpand(&newt_bc_env->breakstack, BREAKSP + 1))
		return;

    BREAKSTACK[BREAKSP] = cx;
    BREAKSP++;
}


/*------------------------------------------------------------------------*/
/** ループ内のブレーク命令をバックパッチする
 *
 * @param loop_head	[in] ループの開始位置
 * @param cx		[in] ループの終わり位置
 *
 * @return			なし
 */

void NBCBreakBackPatchs(uint32_t loop_head, uint32_t cx)
{
    uint32_t	branch;

    for (; 0 < BREAKSP; BREAKSP--)
    {
        branch = BREAKSTACK[BREAKSP - 1];

        if (branch < loop_head)
            break;

        NBCBackPatch(branch, cx);	// ブランチをバックパッチ
    }
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 例外処理命令の位置をスタックする
 *
 * @param cx		[in] 例外処理命令の位置
 *
 * @return			なし
 *
 *　@note				バックパッチのために覚えておく
 */

void NBCPushOnexcpStack(uint32_t cx)
{
    if (ONEXCPSTACK == NULL)
		NewtStackSetup(&newt_bc_env->onexcpstack, NEWT_POOL, sizeof(uint32_t), NEWT_NUM_ONEXCPSTACK);

	if (! NewtStackExpand(&newt_bc_env->onexcpstack, ONEXCPSP + 1))
		return;

    ONEXCPSTACK[ONEXCPSP] = cx;
    ONEXCPSP++;
}


/*------------------------------------------------------------------------*/
/** TRY文内のブレーク命令をバックパッチする
 *
 * @param try_head	[in] TRY文の開始位置
 * @param cx		[in] TRY文の終わり位置
 *
 * @return			なし
 */

void NBCOnexcpBackPatchs(uint32_t try_head, uint32_t cx)
{
    uint32_t	branch;

    for (; 0 < ONEXCPSP; ONEXCPSP--)
    {
        branch = ONEXCPSTACK[ONEXCPSP - 1];

        if (branch < try_head)
            break;

        NBCBackPatch(branch, cx);	// ブランチをバックパッチ
    }
}


/*------------------------------------------------------------------------*/
/** 例外処理命令のバイトコードを生成する
 *
 * @param pc		[in] 例外処理命令のプログラムカウンタ
 *
 * @return			なし
 */

void NBCGenOnexcpPC(int32_t pc)
{
    newtRefVar	r;
    int16_t	b;

    r = NewtMakeInteger(pc);
    b = NBCGenPushLiteral(r);

    NBCPushOnexcpStack(b);	// バックパッチのためにスタックにプッシュする
}


/*------------------------------------------------------------------------*/
/** 例外処理内のブレーク命令のバイトコードを生成する
 *
 * @return			なし
 */

void NBCGenOnexcpBranch(void)
{
    uint32_t	cx;
    
    cx = NBCGenBranch(kNBCBranch);	// ブランチ
    NBCPushOnexcpStack(cx);			// バックパッチのためにスタックにプッシュする
}


/*------------------------------------------------------------------------*/
/** バックパッチ時に例外処理シンボルをリテラルリストに登録する
 *
 * @param sp		[in] 例外処理命令スタックのスタックポインタ
 * @param pc		[in] 例外処理命令のプログラムカウンタ
 *
 * @return			なし
 */

void NBCOnexcpBackPatchL(uint32_t sp, int32_t pc)
{
    newtRefVar	r;

    if (ONEXCPSTACK == NULL || ONEXCPSP <= sp)
        return;

    r = NewtMakeInteger(pc);
    NewtSetArraySlot(LITERALS, ONEXCPSTACK[sp], r);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 関数オブジェクトを作成する
 *
 * @param env		[in] バイトコード環境
 *
 * @return			関数オブジェクト
 */

newtRef NBCMakeFn(nbc_env_t * env)
{
    newtRefVar	fnv[] = {
                            NS_CLASS,				NSSYM0(CodeBlock),
                            NSSYM0(instructions),	kNewtRefNIL,
                            NSSYM0(literals),		kNewtRefNIL,
                            NSSYM0(argFrame),		kNewtRefNIL,
                            NSSYM0(numArgs),		kNewtRefNIL
                        };

    newtRefVar	afv[] = {
                            NSSYM0(_nextArgFrame),	kNewtRefNIL,
                            NSSYM0(_parent),		kNewtRefNIL,
                            NSSYM0(_implementor),	kNewtRefNIL
                        };

    newtRefVar	fn;
    int32_t	numArgs = 0;

    // literals
    env->literals = NewtMakeArray(NSSYM0(literals), 0);

    // argFrame
    env->argFrame = NewtMakeFrame2(sizeof(afv) / (sizeof(newtRefVar) * 2), afv);

    // function
    fn = NewtMakeFrame2(sizeof(fnv) / (sizeof(newtRefVar) * 2), fnv);

//    NcSetClass(fn, NSSYM0(CodeBlock));
    NcSetSlot(fn, NSSYM0(instructions), kNewtRefNIL);
    NcSetSlot(fn, NSSYM0(literals), env->literals);
    NcSetSlot(fn, NSSYM0(argFrame), env->argFrame);
    NcSetSlot(fn, NSSYM0(numArgs), NewtMakeInteger(numArgs));

    env->func = fn;

    // constant
	if (env->parent)
	{	// 親があれば定数フレームを共有する
		env->constant = env->parent->constant;
	}
	else
	{	// 親がなければ新規に定数フレームを作成
		env->constant = NcMakeFrame();
	}

    return fn;
}


/*------------------------------------------------------------------------*/
/** 関数命令テーブルを初期化する
 *
 * @return			なし
 */

void NBCInitFreqFuncTable(void)
{
    int	i;

    for (i = 0; freq_func_tb[i].name != NULL; i++)
    {
        freq_func_tb[i].sym = NewtMakeSymbol(freq_func_tb[i].name);
    }
}


/*------------------------------------------------------------------------*/
/** バイトコード環境を作成する
 *
 * @param parent	[in] 呼出し元のバイトコード環境
 *
 * @return			バイトコード環境
 */

nbc_env_t * NBCEnvNew(nbc_env_t * parent)
{
    nbc_env_t *	env;

    env = (nbc_env_t *)NewtMemCalloc(NEWT_POOL, 1, sizeof(nbc_env_t));

	if (env != NULL)
	{
		env->parent = parent;
		NewtStackSetup(&env->bytecode, NEWT_POOL, sizeof(uint8_t), NEWT_NUM_BYTECODE);

		NBCMakeFn(env);
	}

    return env;
}


/*------------------------------------------------------------------------*/
/** バイトコード環境を解放する
 *
 * @param env		[in] バイトコード環境
 *
 * @return			なし
 */

void NBCEnvFree(nbc_env_t * env)
{
    if (env != NULL)
    {
		NewtStackFree(&env->bytecode);
		NewtStackFree(&env->breakstack);
		NewtStackFree(&env->onexcpstack);

        NewtMemFree(env);
    }
}


/*------------------------------------------------------------------------*/
/** 関数オブジェクトの生成を終了する
 *
 * @param envP		[in] バイトコード環境へのポインタ
 *
 * @return			関数オブジェクト
 */

newtRef NBCFnDone(nbc_env_t ** envP)
{
    nbc_env_t *	env = *envP;
    newtRefVar	fn = kNewtRefNIL;

    if (env != NULL)
    {
        newtRefVar	instr;
        newtRefVar	literals;

        NBCGenCodeEnv(env, kNBCReturn, 0);

        fn = env->func;
        instr = NewtMakeBinary(kNewtRefNIL, ENV_BC(env), ENV_CX(env), true);
        NcSetSlot(fn, NSSYM0(instructions), instr);
    
        literals = NcGetSlot(fn, NSSYM0(literals));

        if (NewtRefIsNotNIL(literals) && NcLength(literals) == 0)
            NcSetSlot(fn, NSSYM0(literals), kNewtRefNIL);

        *envP = env->parent;
        NBCEnvFree(env);
    }

    return fn;
}


/*------------------------------------------------------------------------*/
/** バイトコード生成のための初期化
 *
 * @return			なし
 */

void NBCInit(void)
{
    NBCInitFreqFuncTable();
    newt_bc_env = NBCEnvNew(NULL);
}


/*------------------------------------------------------------------------*/
/** バイトコード生成の後始末
 *
 * @return			なし
 */

void NBCCleanup(void)
{
    nbc_env_t *	env;

    while (newt_bc_env != NULL)
    {
        env = newt_bc_env;
        newt_bc_env = env->parent;
        NBCEnvFree(env);
    }
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 文のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 * @param ret		[in] 戻り値の有無
 *
 * @return			なし
 */

void NBCGenBC_stmt(nps_syntax_node_t * stree, nps_node_t r, bool ret)
{
    if (NPSRefIsSyntaxNode(r))
    {
        NBCGenBC_sub(stree, NPSRefToSyntaxNode(r), ret);
        return;
    }

    if (r != kNewtRefUnbind)
        NBCGenPUSH(r);
}


/*------------------------------------------------------------------------*/
/** 定数のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 *
 * @return			なし
 */

void NBCGenConstant(nps_syntax_node_t * stree, nps_node_t r)
{
    if (NPSRefIsSyntaxNode(r))
    {
        nps_syntax_node_t * node;

        node = &stree[NPSRefToSyntaxNode(r)];

        switch (node->code)
        {
            case kNPSCommaList:
                NBCGenConstant(stree, node->op1);
                NBCGenConstant(stree, node->op2);
                break;

            case kNPSAssign:
                // node->op2 がオブジェクトでない場合の処理を行うこと
                NcSetSlot(CONSTANT, node->op1, node->op2);
                break;
        }
    }
    else
    {
        NBError(kNErrSyntaxError);
    }
}


/*------------------------------------------------------------------------*/
/** グローバル変数のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 *
 * @return			なし
 */

void NBCGenGlobalVar(nps_syntax_node_t * stree, nps_node_t r)
{
    if (NPSRefIsSyntaxNode(r))
    {
        nps_syntax_node_t * node;

        node = &stree[NPSRefToSyntaxNode(r)];

        switch (node->code)
        {
            case kNPSCommaList:
                NBCGenGlobalVar(stree, node->op1);
                NBCGenGlobalVar(stree, node->op2);
                break;

            case kNPSAssign:
                NBCGenPUSH(node->op1);
                NBCGenBC_op(stree, node->op2);

                // defGlobalVar を呼出す
                NBCGenCallFn(NSSYM0(defGlobalVar), 2);
                break;
        }
    }
    else if (NewtRefIsSymbol(r))
    {
        NBCGenPUSH(r);
        NBCGenPUSH(kNewtRefUnbind);

        // defGlobalVar を呼出す
        NBCGenCallFn(NSSYM0(defGlobalVar), 2);
    }
    else
    {
        NBError(kNErrSyntaxError);
    }
}


/*------------------------------------------------------------------------*/
/** ローカル変数のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param type		[in] 型
 * @param r			[in] 構文木ノード
 *
 * @return			なし
 *
 * @note			現在のバージョンでは type は完全に無視される
 */

void NBCGenLocalVar(nps_syntax_node_t * stree, nps_node_t type, nps_node_t r)
{
    if (NPSRefIsSyntaxNode(r))
    {
        nps_syntax_node_t * node;

        node = &stree[NPSRefToSyntaxNode(r)];

        switch (node->code)
        {
            case kNPSCommaList:
                NBCGenLocalVar(stree, type, node->op1);
                NBCGenLocalVar(stree, type, node->op2);
                break;

            case kNPSAssign:
                NBCGenBC_op(stree, node->op2);
                NBCDefLocal(type, node->op1, true);
                break;
        }
    }
    else if (NewtRefIsSymbol(r))
    {
        NBCDefLocal(type, r, false);
    }
    else
    {
        NBError(kNErrSyntaxError);
    }
}


/*------------------------------------------------------------------------*/
/** 型が正しいかチェックする
 *
 * @param type		[in] 型
 *
 * @retval			true	正しい型
 * @retval			false	正しくない型
 */

bool NBCTypeValid(nps_node_t type)
{
    if (type == kNewtRefUnbind)
        return true;

    if (type == NS_INT)
        return true;

    if (type == NSSYM0(array))
        return true;

    NBError(kNErrSyntaxError);

    return false;
}


/*------------------------------------------------------------------------*/
/** TRY文の先頭のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 *
 * @return			例外処理の数
 */

int16_t NBCGenTryPre(nps_syntax_node_t * stree, nps_node_t r)
{
    int16_t	numExcps = 0;

    if (NPSRefIsSyntaxNode(r))
    {
        nps_syntax_node_t * node;

        node = &stree[NPSRefToSyntaxNode(r)];

        switch (node->code)
        {
            case kNPSOnexception:
                NBCGenPUSH(node->op1);	// シンボル
                NBCGenOnexcpPC(-1);	// PC（ダミー）

                numExcps = 1;
                break;

            case kNPSOnexceptionList:
                numExcps = NBCGenTryPre(stree, node->op1)
                            + NBCGenTryPre(stree, node->op2);
                break;
        }
    }

    return numExcps;
}


/*------------------------------------------------------------------------*/
/** TRY文の終わりのバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 * @param onexcpspP	[in] 例外処理命令の順番へのポインタ
 *
 * @return			例外処理の数
 */

int16_t NBCGenTryPost(nps_syntax_node_t * stree, nps_node_t r,
            uint32_t * onexcpspP)
{
    int16_t	numExcps = 0;

    if (NPSRefIsSyntaxNode(r))
    {
        nps_syntax_node_t * node;

        node = &stree[NPSRefToSyntaxNode(r)];

        switch (node->code)
        {
            case kNPSOnexception:
                // new-handler の引数をバックパッチ
                NBCOnexcpBackPatchL(*onexcpspP, CX);
                (*onexcpspP)++;

                // onexception のコード生成
                NBCGenBC_stmt(stree, node->op2, true);
                NBCGenOnexcpBranch();

                numExcps = 1;
                break;

            case kNPSOnexceptionList:
                numExcps = NBCGenTryPost(stree, node->op1, onexcpspP)
                            + NBCGenTryPost(stree, node->op2, onexcpspP);
                break;
        }
    }

    return numExcps;
}


/*------------------------------------------------------------------------*/
/** TRY文のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param expr		[in] 式の構文木ノード
 * @param onexception_list	[in] 例外処理の構文木ノード
 *
 * @return			なし
 */

void NBCGenTry(nps_syntax_node_t * stree, nps_node_t expr,
        nps_node_t onexception_list)
{
    uint32_t	onexcp_cx;
    uint32_t	branch_cx;
    uint32_t	onexcpsp;
    int16_t	numExcps = 0;

    onexcpsp = ONEXCPSP;
    numExcps = NBCGenTryPre(stree, onexception_list);
    NBCGenCode(kNBCNewHandlers, numExcps);

    // 実行文
    NBCGenBC_op(stree, expr);
    NBCGenCode(kNBCPopHandlers, 0);

    branch_cx = CX;
    branch_cx = NBCGenBranch(kNBCBranch);

    // onexception
    onexcp_cx = CX;
    NBCGenTryPost(stree, onexception_list, &onexcpsp);

    // onexception の終了
    NBCOnexcpBackPatchs(onexcp_cx, CX);	// onexception の終了をバックパッチ
    NBCGenCode(kNBCPopHandlers, 0);

    // ONEXCPSP を戻す
    ONEXCPSP = onexcpsp;

    // バックパッチ
    NBCBackPatch(branch_cx, CX);	// branch をバックパッチ
}


/*------------------------------------------------------------------------*/
/** IF文のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param cond		[in] 条件式の構文木ノード
 * @param thenelse	[in] THEN, ELSE の構文木ノード
 * @param ret		[in] 戻り値の有無
 *
 * @return			なし
 */

void NBCGenIfThenElse(nps_syntax_node_t * stree, nps_node_t cond,
        nps_node_t thenelse, bool ret)
{
    nps_node_t	ifthen;
    nps_node_t	ifelse = kNewtRefUnbind;
    uint32_t	cond_cx;

    NBCGenBC_op(stree, cond);
    cond_cx = NBCGenBranch(kNBCBranchIfFalse);

    if (NewtRefIsArray(thenelse))
    {
        ifthen = NewtGetArraySlot(thenelse, 0);
        ifelse = NewtGetArraySlot(thenelse, 1);
    }
    else
    {
        ifthen = thenelse;
    }

    // THEN 文
    NBCGenBC_stmt(stree, ifthen, ret);

    if (ifelse == kNewtRefUnbind)
    {
        NBCBackPatch(cond_cx, CX);				// 条件文をバックパッチ
    }
    else
    {
        uint32_t	then_done;

        then_done = NBCGenBranch(kNBCBranch);	// THEN 文の終了
        NBCBackPatch(cond_cx, CX);				// 条件文をバックパッチ

        // ELSE 文
        NBCGenBC_stmt(stree, ifelse, ret);

        NBCBackPatch(then_done, CX);			// THEN 文終了のブランチをバックパッチ
    }
}


/*------------------------------------------------------------------------*/
/** 論理AND のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param op1		[in] オペランド１の構文木ノード
 * @param op2		[in] オペランド２の構文木ノード
 *
 * @return			なし
 */

void NBCGenAnd(nps_syntax_node_t * stree, nps_node_t op1, nps_node_t op2)
{
    uint32_t	cx1;
    uint32_t	cx2;

	// オペランド１
    NBCGenBC_op(stree, op1);

	// NIL なら分岐
    cx1 = NBCGenBranch(kNBCBranchIfFalse);

    // オペランド２
    NBCGenBC_op(stree, op2);
	// 式の最後へ分岐
    cx2 = NBCGenBranch(kNBCBranch);

	// 戻り値をプッシュ
	NBCBackPatch(cx1, CX);		// 分岐をバックパッチ
    NBCGenPUSH(kNewtRefNIL);	// 戻り値は NIL

	// 式の最後
	NBCBackPatch(cx2, CX);		// 分岐をバックパッチ
}


/*------------------------------------------------------------------------*/
/** 論理OR のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param op1		[in] オペランド１の構文木ノード
 * @param op2		[in] オペランド２の構文木ノード
 *
 * @return			なし
 */

void NBCGenOr(nps_syntax_node_t * stree, nps_node_t op1, nps_node_t op2)
{
    uint32_t	cx1;
    uint32_t	cx2;

	// オペランド１
    NBCGenBC_op(stree, op1);

	// TRUE なら分岐
    cx1 = NBCGenBranch(kNBCBranchIfTrue);

    // オペランド２
    NBCGenBC_op(stree, op2);
	// 式の最後へ分岐
    cx2 = NBCGenBranch(kNBCBranch);

	// 戻り値をプッシュ
	NBCBackPatch(cx1, CX);		// 分岐をバックパッチ

	if (NPSRefIsSyntaxNode(op1))
		NBCGenPUSH(kNewtRefTRUE);
	else
		NBCGenPUSH(op1);

	// 式の最後
	NBCBackPatch(cx2, CX);		// 分岐をバックパッチ
}


/*------------------------------------------------------------------------*/
/** LOOP文のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param expr		[in] 実行文の構文木ノード
 *
 * @return			なし
 */

void NBCGenLoop(nps_syntax_node_t * stree, nps_node_t expr)
{
    uint32_t	loop_head;

    // loop の先頭
    loop_head = CX;

    // 実行文
    NBCGenBC_stmt(stree, expr, false);

    NBCGenCode(kNBCBranch, loop_head);	// loop の先頭へ

    // バックパッチ
    NBCBreakBackPatchs(loop_head, CX);	// break をバックパッチ
}


/*------------------------------------------------------------------------*/
/** イテレータで使用する一時的なシンボルを作成する
 *
 * @param index		[in] インデックス変数シンボル
 * @param val		[in] バリュー変数シンボル
 * @param s			[in] 文字列
 *
 * @return			シンボル
 */

newtRef NBCMakeTempSymbol(newtRefArg index, newtRefArg val, char * s)
{
    newtRefVar	str;

    str = NSSTR("");

    NcStrCat(str, index);
    NcStrCat(str, val);
    NewtStrCat(str, "|");
    NewtStrCat(str, s);

    return NcMakeSymbol(str);
}


/*------------------------------------------------------------------------*/
/** FOR文のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 * @param expr		[in] 実行文の構文木ノード
 *
 * @return			なし
 */

void NBCGenFor(nps_syntax_node_t * stree, nps_node_t r, nps_node_t expr)
{
    nps_node_t	index;
    nps_node_t	to;
    nps_node_t	by;
    nps_node_t	v;
    newtRefVar	_limit;
    newtRefVar	_incr;
    uint32_t	loop_head;
    uint32_t	branch_cx;

    index = NewtGetArraySlot(r, 0);
    v = NewtGetArraySlot(r, 1);
    to = NewtGetArraySlot(r, 2);
    by = NewtGetArraySlot(r, 3);
 
    if (by == kNewtRefUnbind)
        by = NSINT(1);

    // index を初期化
    NBCGenBC_op(stree, v);
    NBCDefLocal(NS_INT, index, true);

    // index|limit を初期化
    _limit = NBCMakeTempSymbol(index, kNewtRefUnbind, "limit");
    NBCGenBC_op(stree, to);
    NBCDefLocal(NS_INT, _limit, true);

    // index|limit を初期化
    _incr = NBCMakeTempSymbol(index, kNewtRefUnbind, "incr");
    NBCGenBC_op(stree, by);
    NBCDefLocal(NS_INT, _incr, true);

    // 条件文へブランチ
    NBCGenGetVar(stree, _incr);
    NBCGenGetVar(stree, index);

    branch_cx = NBCGenBranch(kNBCBranch);

    // loop の先頭
    loop_head = CX;

    // 実行文
    NBCGenBC_stmt(stree, expr, false);

    // 変数に by を増分
    {
        int16_t	b;

        b = NewtFindSlotIndex(ARGFRAME, index);

        NBCGenGetVar(stree, _incr);
        NBCGenCode(kNBCIncrVar, b);
    }

    // 条件文
    NBCBackPatch(branch_cx, CX);			// branch をバックパッチ
    NBCGenGetVar(stree, _limit);
    NBCGenCode(kNBCBranchIfLoopNotDone, loop_head);	// loop の先頭へ

    // 戻り値
    NBCGenPUSH(kNewtRefNIL);

    // バックパッチ
    NBCBreakBackPatchs(loop_head, CX);	// break をバックパッチ
}


/*------------------------------------------------------------------------*/
/** FOREACH文のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 * @param expr		[in] 実行文の構文木ノード
 *
 * @return			なし
 */

void NBCGenForeach(nps_syntax_node_t * stree, nps_node_t r, nps_node_t expr)
{
    nps_node_t	index;
    nps_node_t	val;
    nps_node_t	obj;
    nps_node_t	deeply;
    nps_node_t	op;
    newtRefVar	_iter;
    newtRefVar	_index = kNewtRefUnbind;
    newtRefVar	_result = kNewtRefUnbind;
    uint32_t	loop_head;
    uint32_t	branch_cx;
    bool	collect;

    index = NewtGetArraySlot(r, 0);
    val = NewtGetArraySlot(r, 1);
    obj = NewtGetArraySlot(r, 2);
    deeply = NewtGetArraySlot(r, 3);
    op = NewtGetArraySlot(r, 4);

    collect = (op == NSSYM0(collect));

    NBCDefLocal(kNewtRefUnbind, val, false);
    if (index != kNewtRefUnbind) NBCDefLocal(kNewtRefUnbind, index, false);

    _iter = NBCMakeTempSymbol(index, val, "iter");

    // new-iterator
    NBCGenBC_op(stree, obj);
    NBCGenPUSH(deeply);
    NBCGenFreq(kNBCNewIterator);
    NBCDefLocal(NSSYM0(array), _iter, true);

    if (collect)
    {
        int32_t	lenIndex;

        _index = NBCMakeTempSymbol(index, val, "index");
        _result = NBCMakeTempSymbol(index, val, "result");

        NBCDefLocal(NSSYM0(array), _index, false);
        NBCDefLocal(NSSYM0(array), _result, false);

        // 戻り値の array を作成
        if (NewtRefIsNIL(deeply))
            lenIndex = kIterMax;
        else
            lenIndex = kIterDeeply;

        // length
        NBCGenGetVar(stree, _iter);
        NBCGenPUSH(NewtMakeInteger(lenIndex));
        NBCGenFreq(kNBCAref);
        // make-array
        NBCGenPUSH(kNewtRefUnbind);
        NBCGenCode(kNBCMakeArray, -1);
        NBCDefLocal(NSSYM0(array), _result, true);

        // index の初期化
        NBCGenPUSH(NSINT(0));
        NBCDefLocal(NS_INT, _index, true);
    }

    // 条件文へブランチ
    branch_cx = NBCGenBranch(kNBCBranch);

    // loop の先頭
    loop_head = CX;

    // val のセット
    NBCGenGetVar(stree, _iter);
    NBCGenPUSH(NewtMakeInteger(kIterValue));
    NBCGenFreq(kNBCAref);
    NBCDefLocal(kNewtRefUnbind, val, true);

    // index のセット
    if (index != kNewtRefUnbind)
    {
        NBCGenGetVar(stree, _iter);
        NBCGenPUSH(NewtMakeInteger(kIterIndex));
        NBCGenFreq(kNBCAref);
        NBCDefLocal(kNewtRefUnbind, index, true);
    }

    // 実行文
    if (collect)
    {
        int16_t	b;

        NBCGenGetVar(stree, _result);
        NBCGenGetVar(stree, _index);
        NBCGenBC_stmt(stree, expr, true);
        NBCGenFreq(kNBCSetAref);

        NBCGenCode(kNBCPop, 0);

        //
        b = NewtFindSlotIndex(ARGFRAME, _index);
        NBCGenPUSH(NSINT(1));
        NBCGenCode(kNBCIncrVar, b);

        NBCGenCode(kNBCPop, 0);
        NBCGenCode(kNBCPop, 0);
    }
    else
    {
        NBCGenBC_stmt(stree, expr, false);
    }

    // iter-next
    NBCGenGetVar(stree, _iter);
    NBCGenCode(kNBCIterNext, 0);

    // 条件文
    NBCBackPatch(branch_cx, CX);		// branch をバックパッチ
    NBCGenGetVar(stree, _iter);
    NBCGenCode(kNBCIterDone, 0);		// iter-done
    NBCGenCode(kNBCBranchIfFalse, loop_head);	// loop の先頭へ

    // 戻り値
    if (collect)
        NBCGenGetVar(stree, _result);
    else
        NBCGenPUSH(kNewtRefNIL);

    // バックパッチ
    NBCBreakBackPatchs(loop_head, CX);	// break をバックパッチ

    // iterator の後始末
    if (collect)
    {
        NBCGenPUSH(kNewtRefNIL);
        NBCDefLocal(kNewtRefUnbind, _result, true);
    }

    NBCGenPUSH(kNewtRefNIL);
    NBCDefLocal(kNewtRefUnbind, _iter, true);
}


/*------------------------------------------------------------------------*/
/** WHILE文のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param cond		[in] 条件式の構文木ノード
 * @param expr		[in] 実行文の構文木ノード
 *
 * @return			なし
 */

void NBCGenWhile(nps_syntax_node_t * stree, nps_node_t cond, nps_node_t expr)
{
    uint32_t	loop_head;
    uint32_t	cond_cx;

    // loop の先頭
    loop_head = CX;

    // 条件文
    NBCGenBC_op(stree, cond);
    cond_cx = NBCGenBranch(kNBCBranchIfFalse);

    // 実行文
    NBCGenBC_stmt(stree, expr, false);

    NBCGenCode(kNBCBranch, loop_head);	// loop の先頭へ

    // バックパッチ
    NBCBackPatch(cond_cx, CX);		// 条件文をバックパッチ

    // 戻り値
    NBCGenPUSH(kNewtRefNIL);

    NBCBreakBackPatchs(loop_head, CX);	// break をバックパッチ
}


/*------------------------------------------------------------------------*/
/** REPEAT文のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param expr		[in] 実行文の構文木ノード
 * @param cond		[in] 条件式の構文木ノード
 *
 * @return			なし
 */

void NBCGenRepeat(nps_syntax_node_t * stree, nps_node_t expr, nps_node_t cond)
{
    uint32_t	loop_head;

    // loop の先頭
    loop_head = CX;

    // 実行文
    NBCGenBC_stmt(stree, expr, false);

    // 条件文
    NBCGenBC_op(stree, cond);
    NBCGenCode(kNBCBranchIfFalse, loop_head);	// loop の先頭へ

    // 戻り値
    NBCGenPUSH(kNewtRefNIL);

    // バックパッチ
    NBCBreakBackPatchs(loop_head, CX);		// break をバックパッチ
}


/*------------------------------------------------------------------------*/
/** BREAK文のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param expr		[in] 実行文の構文木ノード
 *
 * @return			なし
 */

void NBCGenBreak(nps_syntax_node_t * stree, nps_node_t expr)
{
    uint32_t	cx;

    // 戻り値
    if (expr == kNewtRefUnbind)
        NBCGenPUSH(kNewtRefNIL);
    else
        NBCGenBC_op(stree, expr);

    // ブランチ
    cx = NBCGenBranch(kNBCBranch);	// loop の終わりへ
    NBCPushBreakStack(cx);		// バックパッチのためにスタックにプッシュする
}


/*------------------------------------------------------------------------*/
/** 文字列の結合命令のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param op1		[in] 引数１
 * @param op2		[in] 引数２
 * @param dlmt		[in] 区切り文字列
 *
 * @return			なし
 */

void NBCGenStringer(nps_syntax_node_t * stree, nps_node_t op1, nps_node_t op2,
        char * dlmt)
{
    int16_t numArgs = 2;

    NBCGenBC_op(stree, op1);

    if (dlmt != NULL)
    {
        NBCGenPUSH(NSSTRCONST(dlmt));
        numArgs++;
    }

    NBCGenBC_op(stree, op2);
    NBCGenPUSH(kNewtRefUnbind);
    NBCGenCode(kNBCMakeArray, numArgs);

    NBCGenFreq(kNBCStringer);
}


/*------------------------------------------------------------------------*/
/** 代入式のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param lvalue	[in] 左辺
 * @param expr		[in] 式
 * @param ret		[in] 戻り値の有無
 *
 * @return			なし
 */

void NBCGenAssign(nps_syntax_node_t * stree,
        nps_node_t lvalue, nps_node_t expr, bool ret)
{
    if (NewtRefIsSymbol(lvalue))
    {
        if (NewtHasSlot(CONSTANT, lvalue))
        {
            // 定数の場合
            NBError(kNErrAssignToConstant);
            return;
        }

        NBCGenBC_op(stree, expr);
        NBCGenCodeL(kNBCFindAndSetVar, lvalue);

        if (ret)
            NBCGenCodeL(kNBCFindVar, lvalue);
    }
#ifdef __NAMED_MAGIC_POINTER__
	else if (NewtRefIsNamedMP(lvalue))
	{
		newtRefVar	sym;

		sym = NewtMPToSymbol(lvalue);
		NBCGenFunc2(stree, NSSYM0(defMagicPointer), sym, expr);
		NVCGenNoResult(ret);
	}
#endif /* __NAMED_MAGIC_POINTER__ */
	else if (NewtRefIsNumberedMP(lvalue))
	{
		int32_t	index;

		index = NewtMPToIndex(lvalue);
		NBCGenFunc2(stree, NSSYM0(defMagicPointer), NewtMakeInteger(index), expr);
		NVCGenNoResult(ret);
	}
    else if (NPSRefIsSyntaxNode(lvalue))
    {
        nps_syntax_node_t * node;

        node = &stree[NPSRefToSyntaxNode(lvalue)];

        switch (node->code)
        {
            case kNPSGetPath:
                NBCGenBC_op(stree, node->op1);
                NBCGenBC_op(stree, node->op2);
                NBCGenBC_op(stree, expr);
                NBCGenCode(kNBCSetPath, 1);
                NVCGenNoResult(ret);
                break;

            case kNPSAref:
                NBCGenBC_op(stree, node->op1);
                NBCGenBC_op(stree, node->op2);
                NBCGenBC_op(stree, expr);
                NBCGenFreq(kNBCSetAref);
                NVCGenNoResult(ret);
                break;

            default:
                NBError(kNErrSyntaxError);
                break;
        }
    }
    else
    {
        NBError(kNErrSyntaxError);
    }
}


/*------------------------------------------------------------------------*/
/** EXISTS式のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 *
 * @return			なし
 */

void NBCGenExists(nps_syntax_node_t * stree, nps_node_t r)
{
    if (NewtRefIsSymbol(r))
    {
        if (NewtFindArrayIndex(LITERALS, r, 0) != -1)
        {
            // ローカル変数が宣言されている場合
            NBCGenPUSH(kNewtRefTRUE);
        }
        else
        {
            NBCGenPUSH(r);
            NBCGenCallFn(NSSYM0(hasVar), 1);
        }
    }
    else if (NPSRefIsSyntaxNode(r))
    {
        nps_syntax_node_t * node;

        node = &stree[NPSRefToSyntaxNode(r)];

        switch (node->code)
        {
            case kNPSGetPath:
                NBCGenBC_op(stree, node->op1);
                NBCGenBC_op(stree, node->op2);
                NBCGenFreq(kNBCHasPath);
                break;

            default:
                NBError(kNErrSyntaxError);
                break;
        }
    }
    else
    {
        NBError(kNErrSyntaxError);
    }
}


/*------------------------------------------------------------------------*/
/** レシーバのバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 *
 * @return			なし
 */

void NBCGenReceiver(nps_syntax_node_t * stree, nps_node_t r)
{
    if (r == kNewtRefUnbind)
        NBCGenCode(kNBCPushSelf, 0);
    else
        NBCGenBC_op(stree, r);
}


/*------------------------------------------------------------------------*/
/** メソッドEXISTS式のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param receiver  [in] レシーバ
 * @param name		[in] メソッド名
 *
 * @return			なし
 */

void NBCGenMethodExists(nps_syntax_node_t * stree,
        nps_node_t receiver, nps_node_t name)
{
    // receiver の生成
    NBCGenReceiver(stree, receiver);

    // name
    NBCGenBC_op(stree, name);

    // hasVariable を呼出す
    NBCGenCallFn(NSSYM0(hasVariable), 2);
}


/*------------------------------------------------------------------------*/
/** 関数定義のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param args		[in] 引数
 * @param expr		[in] 式
 *
 * @return			なし
 */

void NBCGenFn(nps_syntax_node_t * stree, nps_node_t args, nps_node_t expr)
{
    nbc_env_t * env;
    newtRefVar	fn;

    env = NBCMakeFnEnv(stree, args);
    NBCGenBC_op(stree, expr);
    fn = NBCFnDone(&newt_bc_env);
    NBCGenPUSH(fn);
    NBCGenCode(kNBCSetLexScope, 0);
}


/*------------------------------------------------------------------------*/
/** グローバル関数定義のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param name		[in] グローバル関数名
 * @param fn		[in] 関数
 *
 * @return			なし
 */

void NBCGenGlobalFn(nps_syntax_node_t * stree, nps_node_t name, nps_node_t fn)
{
    NBCGenCodeL(kNBCPush, name);
    NBCGenBC_op(stree, fn);
    NBCGenPUSH(NSSYM0(defGlobalFn));
    NBCGenCode(kNBCCall, 2);
}


/*------------------------------------------------------------------------*/
/** 関数呼出しのバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param name		[in] グローバル関数名
 * @param args		[in] 引数
 *
 * @return			なし
 */

void NBCGenCall(nps_syntax_node_t * stree, nps_node_t name, nps_node_t args)
{
    int16_t numArgs;

    NBCGenBC_op(stree, args);
    numArgs = NBCCountNumArgs(stree, args);

    NBCGenCallFn(name, numArgs);
}


/*------------------------------------------------------------------------*/
/** 関数オブジェクト実行のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param fn		[in] 関数オブジェクト
 * @param args		[in] 引数
 *
 * @return			なし
 */

void NBCGenInvoke(nps_syntax_node_t * stree, nps_node_t fn, nps_node_t args)
{
    int16_t numArgs;

    NBCGenBC_op(stree, args);
    numArgs = NBCCountNumArgs(stree, args);

    NBCGenBC_op(stree, fn);
//    NBCGenCode(kNBCSetLexScope, 0);
    NBCGenCode(kNBCInvoke, numArgs);
}


/*------------------------------------------------------------------------*/
/** ２項関数の呼出しバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param name		[in] 関数名
 * @param op1		[in] 引数１
 * @param op2		[in] 引数２
 *
 * @return			なし
 */

void NBCGenFunc2(nps_syntax_node_t * stree,
        newtRefArg name, nps_node_t op1, nps_node_t op2)
{
    NBCGenBC_op(stree, op1);
    NBCGenBC_op(stree, op2);
    NBCGenPUSH(name);
    NBCGenCode(kNBCCall, 2);
}


/*------------------------------------------------------------------------*/
/** メソッド送信のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param code		[in] 送信タイプ
 * @param receiver	[in] レシーバ
 * @param r			[in] メソッド名＋引数
 *
 * @return			なし
 */

void NBCGenSend(nps_syntax_node_t * stree, uint32_t code,
        nps_node_t receiver, nps_node_t r)
{
    nps_syntax_node_t * node;
    int16_t numArgs;

    node = &stree[NPSRefToSyntaxNode(r)];

    // 引数の生成
    NBCGenBC_op(stree, node->op2);
    numArgs = NBCCountNumArgs(stree, node->op2);

	// There is a mistake in NewtonFormats. We push arguments in the Newton's
	// natural order.

    // receiver の生成
    NBCGenReceiver(stree, receiver);
    // message の生成
    NBCGenPUSH(node->op1);

    // メッセージ呼出しの生成
    NBCGenCode(code, numArgs);
}


/*------------------------------------------------------------------------*/
/** メソッド再送信のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param code		[in] 送信タイプ
 * @param name		[in] メソッド名
 * @param args		[in] 引数
 *
 * @return			なし
 */

void NBCGenResend(nps_syntax_node_t * stree, uint32_t code,
        nps_node_t name, nps_node_t args)
{
    int16_t numArgs;

    NBCGenBC_op(stree, args);
    numArgs = NBCCountNumArgs(stree, args);

    NBCGenPUSH(name);
    NBCGenCode(code, numArgs);
}


/*------------------------------------------------------------------------*/
/** 配列作成のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param klass		[in] クラス
 * @param r			[in] 初期化データ
 *
 * @return			なし
 */

void NBCGenMakeArray(nps_syntax_node_t * stree, nps_node_t klass, nps_node_t r)
{
    int16_t n;

    NBCGenBC_op(stree, r);
    n = NBCCountNumArgs(stree, r);

    NBCGenPUSH(klass);
    NBCGenCode(kNBCMakeArray, n);
}


/*------------------------------------------------------------------------*/
/** フレーム作成のバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param r			[in] 初期化データ
 *
 * @return			なし
 */

void NBCGenMakeFrame(nps_syntax_node_t * stree, nps_node_t r)
{
    newtRefVar	map;
    uint32_t	n;

    map = NBCGenMakeFrameSlots(stree, r);
    n = NewtMapLength(map);

    NBCGenPUSH(map);
    NBCGenCode(kNBCMakeFrame, n);
}


/*------------------------------------------------------------------------*/
/** 戻り値が不用の場合のバイトコードを生成する
 *
 * @param ret		[in] 戻り値の有無
 *
 * @return			なし
 */

void NVCGenNoResult(bool ret)
{
    if (! ret)
        NBCGenCode(kNBCPop, 0);
}


/*------------------------------------------------------------------------*/
/** 構文コードのバイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param node		[in] 構文木ノード
 * @param ret		[in] 戻り値の有無
 *
 * @return			なし
 */

void NBCGenSyntaxCode(nps_syntax_node_t * stree, nps_syntax_node_t * node, bool ret)
{
    switch (node->code)
    {
        case kNPSConstituentList:
            if (NewtRefIsNIL(node->op2))
            {
                NBCGenBC_stmt(stree, node->op1, ret);
            }
            else
            {
                NBCGenBC_stmt(stree, node->op1, false);
                NBCGenBC_stmt(stree, node->op2, ret);
            }
            break;

        case kNPSCommaList:
            NBCGenBC_op(stree, node->op1);
            NBCGenBC_op(stree, node->op2);
            break;

        case kNPSConstant:
            NBCGenConstant(stree, node->op1);
            break;

        case kNPSGlobal:
            NBCGenGlobalVar(stree, node->op1);
            NVCGenNoResult(ret);
            break;

        case kNPSLocal:
            if (! NBCTypeValid(node->op1))
                return;

            NBCGenLocalVar(stree, node->op1, node->op2);
            break;

        case kNPSFunc:
            NBCGenFn(stree, node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

        case kNPSGlobalFn:
            NBCGenGlobalFn(stree, node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

        case kNPSLvalue:
            if (NewtRefIsSymbol(node->op1))
                NBCGenGetVar(stree, node->op1);
            else
                NBCGenBC_op(stree, node->op1);

            NVCGenNoResult(ret);
            break;

        case kNPSAssign:
            NBCGenAssign(stree, node->op1, node->op2, ret);
            break;

        case kNPSExists:
            NBCGenExists(stree, node->op1);
            NVCGenNoResult(ret);
            break;

        case kNPSMethodExists:
            NBCGenMethodExists(stree, node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

        case kNPSTry:
            NBCGenTry(stree, node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

        case kNPSIf:
            NBCGenIfThenElse(stree, node->op1, node->op2, ret);
//            NVCGenNoResult(ret);
            break;

        case kNPSLoop:
            NBCGenLoop(stree, node->op1);
            NVCGenNoResult(ret);
            break;

        case kNPSFor:
            NBCGenFor(stree, node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

        case kNPSForeach:
            NBCGenForeach(stree, node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

        case kNPSWhile:
            NBCGenWhile(stree, node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

        case kNPSRepeat:
            NBCGenRepeat(stree, node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

        case kNPSBreak:
            NBCGenBreak(stree, node->op1);
            break;

        case kNPSAnd:
            NBCGenAnd(stree, node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

        case kNPSOr:
            NBCGenOr(stree, node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

        case kNPSMod:
            NBCGenFunc2(stree, NSSYM0(mod), node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

		case kNPSShiftLeft:
            NBCGenFunc2(stree, NSSYM0(shiftLeft), node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

		case kNPSShiftRight:
            NBCGenFunc2(stree, NSSYM0(shiftRight), node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

        case kNPSConcat:
            NBCGenStringer(stree, node->op1, node->op2, NULL);
            NVCGenNoResult(ret);
            break;

        case kNPSConcat2:
            NBCGenStringer(stree, node->op1, node->op2, " ");
            NVCGenNoResult(ret);
            break;

		case kNPSObjectEqual:
            NBCGenFunc2(stree, NSSYM0(objectEqual), node->op1, node->op2);
            NVCGenNoResult(ret);
            break;

		case kNPSMakeRegex:
            NBCGenFunc2(stree, NSSYM0(makeRegex), node->op1, node->op2);
            NVCGenNoResult(ret);
            break;
    }
}


/*------------------------------------------------------------------------*/
/** 引数の数をカウントする
 *
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 *
 * @return			なし
 */

int16_t NBCCountNumArgs(nps_syntax_node_t * stree, nps_node_t r)
{
    int16_t	numArgs = 1;

    if (r == kNewtRefUnbind)
        return 0;

    if (NPSRefIsSyntaxNode(r))
    {
        nps_syntax_node_t * node;

        node = &stree[NPSRefToSyntaxNode(r)];

        switch (node->code)
        {
            case kNPSCommaList:
                numArgs = NBCCountNumArgs(stree, node->op1)
                            + NBCCountNumArgs(stree, node->op2);
                break;
        }
    }

    return numArgs;
}


/*------------------------------------------------------------------------*/
/** フレームまたは配列の初期化バイトコードを生成する（再帰呼出し用）
 *
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 *
 * @return			なし
 */

newtRef NBCGenMakeFrameSlots_sub(nps_syntax_node_t * stree, nps_node_t r)
{
    newtRefVar	obj = kNewtRefUnbind;

    if (r == kNewtRefUnbind)
        return NewtMakeMap(kNewtRefNIL, 0, NULL);

    if (NPSRefIsSyntaxNode(r))
    {
        nps_syntax_node_t * node;

        node = &stree[NPSRefToSyntaxNode(r)];

        switch (node->code)
        {
            case kNPSSlot:
                obj = node->op1;
                NBCGenBC_op(stree, node->op2);
                break;

            case kNPSCommaList:
                {
                    newtRefVar	obj1;
                    newtRefVar	obj2;

                    obj1 = NBCGenMakeFrameSlots_sub(stree, node->op1);
                    obj2 = NBCGenMakeFrameSlots_sub(stree, node->op2);

                    if (NewtRefIsArray(obj1))
                    {
                        obj = obj1;
                    }
                    else
                    {
                        newtRefVar	initMap[1];
                
                        initMap[0] = obj1;
                        obj = NewtMakeMap(kNewtRefNIL, 1, initMap);
                    }

                    if (obj2 == NSSYM0(_proto))
                        NewtSetMapFlags(obj, kNewtMapProto);

                    NcAddArraySlot(obj, obj2);
                }
                break;
        }
    }

    return obj;
}


/*------------------------------------------------------------------------*/
/** フレームまたは配列の初期化バイトコードを生成する
 *
 * @param stree		[in] 構文木
 * @param r			[in] 構文木ノード
 *
 * @return			なし
 */

newtRef NBCGenMakeFrameSlots(nps_syntax_node_t * stree, nps_node_t r)
{
    newtRefVar	obj;

    obj = NBCGenMakeFrameSlots_sub(stree, r);

    if (NewtRefIsArray(obj))
    {
        obj = NewtPackLiteral(obj);
    }
    else
    {
		newtRefVar	initMap[1];

        initMap[0] = obj;
        obj = NewtMakeMap(kNewtRefNIL, 1, initMap);
    }

    return obj;
}


/*------------------------------------------------------------------------*/
/** バイトコードの生成（再帰呼出し用）
 *
 * @param stree		[in] 構文木
 * @param n			[in] 構文木の長さ
 * @param ret		[in] 戻り値の有無
 *
 * @return			なし
 */

void NBCGenBC_sub(nps_syntax_node_t * stree, uint32_t n, bool ret)
{
    nps_syntax_node_t *	node;
    bool	handled = true;

    node = stree + n;

    if (kNPSConstituentList <= node->code)
    {
        NBCGenSyntaxCode(stree, node, ret);
        return;
    }

    switch (node->code)
    {
        case kNPSCall:
            NBCGenCall(stree, node->op1, node->op2);
            break;

        case kNPSInvoke:
            NBCGenInvoke(stree, node->op1, node->op2);
            break;

        case kNPSSend:
            NBCGenSend(stree, kNBCSend, node->op1, node->op2);
			break;

        case kNPSSendIfDefined:
            NBCGenSend(stree, kNBCSendIfDefined, node->op1, node->op2);
            break;

        case kNPSResend:
            NBCGenResend(stree, kNBCResend, node->op1, node->op2);
            break;

        case kNPSResendIfDefined:
            NBCGenResend(stree, kNBCResendIfDefined, node->op1, node->op2);
            break;

        case kNPSMakeArray:
            NBCGenMakeArray(stree, node->op1, node->op2);
            break;

        case kNPSMakeFrame:
            NBCGenMakeFrame(stree, node->op1);
            break;

        case kNPSGetPath:
            NBCGenBC_op(stree, node->op1);
            NBCGenBC_op(stree, node->op2);
            NBCGenCode(node->code, 1);
            break;

        default:
            handled = false;
            break;
    }

    if (handled)
    {
        NVCGenNoResult(ret);
    }
    else
    {
        NBCGenBC_op(stree, node->op1);
        NBCGenBC_op(stree, node->op2);
    
        if (node->code <= kNPSPopHandlers)
            NBCGenCode(0, node->code);
        else if (kNPSAdd <= node->code)
            NBCGenFreq(node->code - kNPSAdd);
        else
            NBCGenCode(node->code, 0);
    }
}


/*------------------------------------------------------------------------*/
/** バイトコードの生成
 *
 * @param stree		[in] 構文木
 * @param size		[in] 構文木の長さ
 * @param ret		[in] 戻り値の有無
 *
 * @return			関数オブジェクト
 */

newtRef NBCGenBC(nps_syntax_node_t * stree, uint32_t size, bool ret)
{
    newtRefVar	fn;

    NBCGenBC_sub(stree, size - 1, ret);
    fn = NBCFnDone(&newt_bc_env);

    if (NewtRefIsNotNIL(fn))
    {
		fn = NewtPackLiteral(fn);

        if (NEWT_DUMPBC)
        {
            NewtFprintf(stderr, "*** byte code ***\n");
            NVMDumpFn(stderr, fn);
            NewtFprintf(stderr, "\n");
        }
    }

    return fn;
}


/*------------------------------------------------------------------------*/
/** ソースファイルをコンパイル
 *
 * @param s			[in] ソースファイルのパス
 * @param ret		[in] 戻り値の有無
 *
 * @return			関数オブジェクト
 */

newtRef NBCCompileFile(char * s, bool ret)
{
    nps_syntax_node_t *	stree = NULL;
    uint32_t	numStree = 0;
    newtRefVar	fn = kNewtRefUnbind;
    newtErr	err;

    err = NPSParseFile(s, &stree, &numStree);

    if (stree != NULL)
    {
        NBCInit();
        fn = NBCGenBC(stree, numStree, ret);
        NBCCleanup();
        NPSCleanup();
    }

    return fn;
}


/*------------------------------------------------------------------------*/
/** 文字列をコンパイル
 *
 * @param s			[in] スクリプト文字列
 * @param ret		[in] 戻り値の有無
 *
 * @return			関数オブジェクト
 */

newtRef NBCCompileStr(char * s, bool ret)
{
    nps_syntax_node_t *	stree;
    uint32_t	numStree;
    newtRefVar	fn = kNewtRefUnbind;
    newtErr	err;

    err = NPSParseStr(s, &stree, &numStree);

    if (stree != NULL)
    {
        NBCInit();
        fn = NBCGenBC(stree, numStree, ret);
        NBCCleanup();
        NPSCleanup();
    }
    
    if (err)
    {
    	return NewtThrow(err, kNewtRefNIL);
    }

    return fn;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** エラーメッセージの表示
 *
 * @param err		[in] エラーコード
 *
 * @return			なし
 */

void NBError(int32_t err)
{
	char *  msg;

	switch (err)
	{
		case kNErrWrongNumberOfArgs:
			msg = "Wrong number of args";
			break;

		case kNErrAssignToConstant:
			msg = "Assign to constant";
			break;

		case kNErrSyntaxError:
			msg = "Syntax error";
			break;

		default:
			msg = "Unknown error";
			break;
	}

    NewtFprintf(stderr, "%s", msg);
}

newtRef NBCConstantTable(void)
{
    return (CONSTANT);
}
