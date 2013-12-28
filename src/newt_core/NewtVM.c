/*------------------------------------------------------------------------*/
/**
 * @file	NewtVM.c
 * @brief   VM
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <stdlib.h>
#include <stdarg.h>

#include "NewtErrs.h"
#include "NewtVM.h"
#include "NewtBC.h"
#include "NewtGC.h"
#include "NewtMem.h"
#include "NewtEnv.h"
#include "NewtObj.h"
#include "NewtFns.h"
#include "NewtStr.h"
#include "NewtFile.h"
#include "NewtIO.h"
#include "NewtPrint.h"
#include "NewtNSOF.h"
#include "NewtPkg.h"


/* 型宣言 */
typedef void(*instruction_t)(int16_t b);			///< 命令セット
typedef void(*simple_instruction_t)(void);			///< シンプル命令
typedef newtRef(*nvm_func_t)();						///< ネイティブ関数


/* グローバル変数 */
vm_env_t	vm_env;


#if 0
#pragma mark -
#endif


/* マクロ */

#define START_LOCALARGS		3										///< ローカル引数の開始位置


#define BC					(vm_env.bc)								///< バイトコード
#define BCLEN				(vm_env.bclen)							///<　バイトコード長

#define CALLSTACK			((vm_reg_t *)vm_env.callstack.stackp)   ///< 呼出しスタック
#define CALLSP				(vm_env.callstack.sp)					///< 呼出しスタックのスタックポインタ
#define EXCPSTACK			((vm_excp_t *)vm_env.excpstack.stackp)  ///< 例外スタック
#define EXCPSP				(vm_env.excpstack.sp)					///< 例外スタックのスタックポインタ
#define CURREXCP			(vm_env.currexcp)						///< 現在の例外

#define	REG					(vm_env.reg)							///< レジスタ
#define STACK				((newtRef *)vm_env.stack.stackp)		///< スタック

#define	FUNC				((REG).func)							///< 実行中の関数
#define	PC					((REG).pc)								///< プログラムカウンタ
#define	SP					((REG).sp)								///< スタックポインタ
#define	LOCALS				((REG).locals)							///< ローカルフレーム
#define	RCVR				((REG).rcvr)							///< レシーバ
#define	IMPL				((REG).impl)							///< インプリメンタ


#if 0
#pragma mark -
#endif


/* 関数プロトタイプ */

static newtErr		NVMGetExceptionErrCode(newtRefArg r, bool dump);
static newtRef		NVMMakeExceptionFrame(newtRefArg name, newtRefArg data);
static void			NVMClearCurrException(void);

static void			NVMSetFn(newtRefArg fn);
static void			NVMNoStackFrameForReturn(void);

static void			reg_rewind(int32_t sp);
static void			reg_pop(void);
static void			reg_push(int32_t sp);
static void			reg_save(int32_t sp);

static newtRef		stk_pop0(void);
static newtRef		stk_pop(void);
static void			stk_pop_n(int32_t n, newtRef a[]);
static void			stk_remove(uint16_t n);
static newtRef		stk_top(void);
static void			stk_push(newtRefArg value);
static void			stk_push_varg(int argc, va_list ap);
static void			stk_push_array(newtRefVar argArray);

static bool			excp_push(newtRefArg sym, newtRefArg pc);
static void			excp_pop(void);
static vm_excp_t *  excp_top(void);
static void			excp_pop_handlers(void);

static newtRef		liter_get(int16_t n);

static newtRef		iter_new(newtRefArg r, newtRefArg deeply);
static void			iter_next(newtRefArg iter);
static bool			iter_done(newtRefArg iter);

static newtRef		NVMMakeArgsArray(uint16_t numArgs);
static void			NVMBindArgs(uint16_t numArgs);
static void			NVMThrowBC(newtErr err, newtRefArg value, int16_t pop, bool push);
static newtErr		NVMFuncCheck(newtRefArg fn, int16_t numArgs);
static void			NVMCallNativeFn(newtRefArg fn, int16_t numArgs);
static void			NVMCallNativeFunc(newtRefArg fn, newtRefArg rcvr, int16_t numArgs);
static void			NVMFuncCall(newtRefArg fn, int16_t numArgs);
static void			NVMMessageSend(newtRefArg impl, newtRefArg receiver, newtRefArg fn, int16_t numArgs);

static newtRef		vm_send(int16_t b, newtErr * errP);
static newtRef		vm_resend(int16_t b, newtErr * errP);

static void			si_pop(void);
static void			si_dup(void);
static void			si_return(void);
static void			si_pushself(void);
static void			si_set_lex_scope(void);
static void			si_iternext(void);
static void			si_iterdone(void);
static void			si_pop_handlers(void);

static void			fn_add(void);
static void			fn_subtract(void);
static void			fn_aref(void);
static void			fn_set_aref(void);
static void			fn_equals(void);
static void			fn_not(void);
static void			fn_not_equals(void);
static void			fn_multiply(void);
static void			fn_divide(void);
static void			fn_div(void);
static void			fn_less_than(void);
static void			fn_greater_than(void);
static void			fn_greater_or_equal(void);
static void			fn_less_or_equal(void);
static void			fn_bit_and(void);
static void			fn_bit_or(void);
static void			fn_bit_not(void);
static void			fn_new_iterator(void);
static void			fn_length(void);
static void			fn_clone(void);
static void			fn_set_class(void);
static void			fn_add_array_slot(void);
static void			fn_stringer(void);
static void			fn_has_path(void);
static void			fn_classof(void);

static void			is_dummy(int16_t b);
static void			is_simple_instructions(int16_t b);
static void			is_push(int16_t b);
static void			is_push_constant(int16_t b);
static void			is_call(int16_t b);
static void			is_invoke(int16_t b);
static void			is_send(int16_t b);
static void			is_send_if_defined(int16_t b);
static void			is_resend(int16_t b);
static void			is_resend_if_defined(int16_t b);
static void			is_branch(int16_t b);
static void			is_branch_if_true(int16_t b);
static void			is_branch_if_false(int16_t b);
static void			is_find_var(int16_t b);
static void			is_get_var(int16_t b);
static void			is_make_frame(int16_t b);
static void			is_make_array(int16_t b);
static void			is_get_path(int16_t b);
static void			is_set_path(int16_t b);
static void			is_set_var(int16_t b);
static void			is_find_and_set_var(int16_t b);
static void			is_incr_var(int16_t b);
static void			is_branch_if_loop_not_done(int16_t b);
static void			is_freq_func(int16_t b);
static void			is_new_handlers(int16_t b);

static void			NVMDumpInstResult(FILE * f);
static void			NVMDumpInstCode(FILE * f, uint8_t * bc, uint32_t pc, uint16_t len);

static void			vm_env_push(vm_env_t * next);
static void			vm_env_pop(void);

static void			NVMInitREG(void);
static void			NVMInitSTACK(void);
static void			NVMCleanSTACK(void);

static void			NVMInitGlobalFns0(void);
static void			NVMInitGlobalFns1(void);
static void			NVMInitExGlobalFns(void);
static void			NVMInitDebugGlobalFns(void);
static void			NVMInitGlobalFns(void);

static void			NVMInitGlobalVars(void);

static void			NVMLoop(uint32_t callsp);

static newtRef		NVMInterpret2(nps_syntax_node_t * stree, uint32_t numStree, newtErr * errP);

static newtRef		NVMFuncCallWithValist(newtRefArg fn, int argc, va_list ap);
static newtRef		NVMMessageSendWithValist(newtRefArg impl, newtRefArg receiver, newtRefArg fn, int argc, va_list ap);


/* ローカル変数 */

/// シンプル命令テーブル
static simple_instruction_t	simple_instructions[] =
            {
                si_pop,				// 000 pop
                si_dup,				// 001 dup
                si_return,			// 002 return
                si_pushself,		// 003 push-self
                si_set_lex_scope,	// 004 set-lex-scope
                si_iternext,		// 005 iter-next
                si_iterdone,		// 006 iter-done
                si_pop_handlers		// 007 000 001 pop-handlers
            };


/// 関数命令テーブル
static simple_instruction_t	fn_instructions[] =
            {
                fn_add,					//  0 add				|+|
                fn_subtract,			//  1 subtract			|-|
                fn_aref,				//  2 aref				aref
                fn_set_aref,	        //  3 set-aref			setAref
                fn_equals,				//  4 equals			|=|
                fn_not,					//  5 not				|not|
                fn_not_equals,	        //  6 not-equals		|<>|
                fn_multiply,	        //  7 multiply			|*|
                fn_divide,				//  8 divide			|/|
                fn_div,					//  9 div				|div|
                fn_less_than,	        // 10 less-than			|<|
                fn_greater_than,		// 11 greater-than		|>|
                fn_greater_or_equal,	// 12 greater-or-equal	|>=|
                fn_less_or_equal,		// 13 less-or-equal		|<=|
                fn_bit_and,				// 14 bit-and			BAnd
                fn_bit_or,				// 15 bit-or			BOr
                fn_bit_not,				// 16 bit-not			BNot
                fn_new_iterator,		// 17 new-iterator		newIterator
                fn_length,				// 18 length			Length
                fn_clone,				// 19 clone				Clone
                fn_set_class,			// 20 set-class			SetClass
                fn_add_array_slot,		// 21 add-array-slot	AddArraySlot
                fn_stringer,			// 22 stringer			Stringer
                fn_has_path,			// 23 has-path			none
                fn_classof				// 24 class-of			ClassOf
            };

/// 命令セットテーブル
static instruction_t	is_instructions[] =
            {
                is_simple_instructions,		// 00x simple instructions
                is_dummy,					// 01x
                is_dummy,					// 02x
                is_push,					// 03x push
                is_push_constant,			// 04x (B signed) push-constant
                is_call,					// 05x call
                is_invoke,					// 06x invoke
                is_send,					// 07x send
                is_send_if_defined,			// 10x send-if-defined
                is_resend,					// 11x resend
                is_resend_if_defined,		// 12x resend-if-defined
                is_branch,					// 13x branch
                is_branch_if_true,			// 14x branch-if-true
                is_branch_if_false,			// 15x branch-if-false
                is_find_var,				// 16x find-var
                is_get_var,					// 17x get-var
                is_make_frame,				// 20x make-frame
                is_make_array,				// 21x make-array
                is_get_path,				// 220/221 get-path
                is_set_path,				// 230/231 set-path
                is_set_var,					// 24x set-var
                is_find_and_set_var,		// 25x find-and-set-var
                is_incr_var,				// 26x incr-var
                is_branch_if_loop_not_done,	// 27x branch-if-loop-not-done
                is_freq_func,				// 30x freq-func
                is_new_handlers				// 31x new-handlers
            };

/// シンプル命令名テーブル
static char *	simple_instruction_names[] =
            {
                "pop",				// 000 pop
                "dup",				// 001 dup
                "return",			// 002 return
                "push-self",		// 003 push-self
                "set-lex-scope",	// 004 set-lex-scope
                "iter-next",		// 005 iter-next
                "iter-done",		// 006 iter-done
                "pop-handlers"		// 007 000 001 pop-handlers
            };

/// 関数命令名テーブル
static char *	fn_instruction_names[] =
            {
                "add",				//  0 add		|+|
                "subtract",			//  1 subtract		|-|
                "aref",				//  2 aref		aref
                "set-aref",			//  3 set-aref		setAref
                "equals",			//  4 equals		|=|
                "not",				//  5 not		|not|
                "not-equals",		//  6 not-equals	|<>|
                "multiply",			//  7 multiply		|*|
                "divide",			//  8 divide		|/|
                "div",				//  9 div		|div|
                "less-than",		// 10 less-than		|<|
                "greater-than",		// 11 greater-than	|>|
                "greateror-equal",	// 12 greater-or-equal	|>=|
                "lessor-equal",		// 13 less-or-equal	|<=|
                "bit-and",			// 14 bit-and		BAnd
                "bit-or",			// 15 bit-or		BOr
                "bit-not",			// 16 bit-not		BNot
                "new-iterator",		// 17 new-iterator	newIterator
                "length",			// 18 length		Length
                "clone",			// 19 clone		Clone
                "set-class",		// 20 set-class		SetClass
                "add-array-slot",	// 21 add-array-slot	AddArraySlot
                "stringer",			// 22 stringer		Stringer
                "has-path",			// 23 has-path		none
                "class-of"			// 24 class-of		ClassOf
            };

/// 命令セット名テーブル
static char *	vm_instruction_names[] =
            {
                "simple-instructions",		// 00x simple instructions
                NULL,						// 01x
                NULL,						// 02x
                "push",						// 03x push
                "push-constant",			// 04x (B signed) push-constant
                "call",						// 05x call
                "invoke",					// 06x invoke
                "send",						// 07x send
                "send-if-defined",			// 10x send-if-defined
                "resend",					// 11x resend
                "resend-if-defined",		// 12x resend-if-defined
                "branch",					// 13x branch
                "branch-if-true",			// 14x branch-if-true
                "branch-if-false",			// 15x branch-if-false
                "find-var",					// 16x find-var
                "get-var",					// 17x get-var
                "make-frame",				// 20x make-frame
                "make-array",				// 21x make-array
                "get-path",					// 220/221 get-path
                "set-path",					// 230/231 set-path
                "set-var",					// 24x set-var
                "find-and-set-var",			// 25x find-and-set-var
                "incr-var",					// 26x incr-var
                "branch-if-loop-not-done",	// 27x branch-if-loop-not-done
                "freq-func",				// 30x freq-func
                "new-handlers"				// 31x new-handlers
            };


#if 0
#pragma mark -
#endif


/*------------------------------------------------------------------------*/
/** self を取得
 *
 * @return			self
 */

newtRef NVMSelf(void)
{
	return RCVR;
}


/*------------------------------------------------------------------------*/
/** 現在の関数オブジェクトを取得する
 *
 * @return		現在の関数オブジェクト
 */
newtRef NVMCurrentFunction(void)
{
    return FUNC;
}


/*------------------------------------------------------------------------*/
/** 現在のインプリメンタを取得する 
 *
 * @return		現在のインプリメンタ
 */
newtRef NVMCurrentImplementor(void)
{
    return IMPL;
}


/*------------------------------------------------------------------------*/
/** 変数の存在チェック
 *
 * @param name		[in] 変数シンボル
 *
 * @retval			true		変数が存在する
 * @retval			false		変数が存在しない
 */

bool NVMHasVar(newtRefArg name)
{
    if (NewtHasLexical(LOCALS, name))
        return true;

    if (NewtHasVariable(RCVR, name))
        return true;

    if (NewtHasGlobalVar(name))
        return true;

    return false;
}


/*------------------------------------------------------------------------*/
/** 例外フレームからエラーコードを取得する
 *
 * @param r		[in] 変数シンボル
 * @param dump	[in] ダンプフラグ
 *
 * @return		エラーコード
 */

newtErr NVMGetExceptionErrCode(newtRefArg r, bool dump)
{
    newtRefVar	err;

    if (NewtRefIsNIL(r))
        return kNErrNone;

    if (dump)
        NewtPrintObject(stderr, r);

    err = NcGetSlot(r, NSSYM0(error));

    if (NewtRefIsNotNIL(err))
        return NewtRefToInteger(err);

    return kNErrBadExceptionName;
}


/*------------------------------------------------------------------------*/
/** 例外フレームを作成する
 *
 * @param name	[in] シンボル
 * @param data	[in] データ
 *
 * @return		例外フレーム
 */

newtRef NVMMakeExceptionFrame(newtRefArg name, newtRefArg data)
{
    newtRefVar	r;

    r = NcMakeFrame();
    NcSetSlot(r, NSSYM0(name), name);

    if (NewtHasSubclass(name, NSSYM0(type.ref)))
        NcSetSlot(r, NSSYM0(data), data);
    else if (NewtHasSubclass(name, NSSYM0(ext.ex.msg)))
        NcSetSlot(r, NSSYM0(message), data);
    else
        NcSetSlot(r, NSSYM0(error), data);

    return r;
}


/*------------------------------------------------------------------------*/
/** 例外を発生させる
 *
 * @param name	[in] シンボル
 * @param data	[in] 例外フレーム
 *
 * @return		なし
 */

void NVMThrowData(newtRefArg name, newtRefArg data)
{
    vm_excp_t *	excp;
    uint32_t	i;

	// 例外処理中ならクリアする
	NVMClearCurrException();

    CURREXCP = data;

    for (i = EXCPSP; 0 < i; i--)
    {
        excp = &EXCPSTACK[i - 1];

        if (NewtHasSubclass(name, excp->sym))
        {
            reg_rewind(excp->callsp);
            PC = excp->pc;
            return;
        }
    }

    NVMNoStackFrameForReturn();
}


/*------------------------------------------------------------------------*/
/** 例外を発生させる
 *
 * @param name	[in] シンボル
 * @param data	[in] データ
 *
 * @return		なし
 */

void NVMThrow(newtRefArg name, newtRefArg data)
{
    newtRefVar	r;

    r = NVMMakeExceptionFrame(name, data);
    NVMThrowData(name, r);
}


/*------------------------------------------------------------------------*/
/** rethrow する
 *
 * @return		なし
 */

void NVMRethrow(void)
{
    if (NewtRefIsNotNIL(CURREXCP))
    {
        newtRefVar	currexcp;
        newtRefVar	name;

		currexcp = CURREXCP;
        name = NcGetSlot(currexcp, NSSYM0(name));

//        excp_pop_handlers();
        NVMThrowData(name, currexcp);
    }
}


/*------------------------------------------------------------------------*/
/** 現在の例外を取得する
 *
 * @return		例外フレーム
 */

newtRef NVMCurrentException(void)
{
    return CURREXCP;
}


/*------------------------------------------------------------------------*/
/** 例外が発生していたら例外スタックをクリアする
 *
 * @return		なし
 */

void NVMClearCurrException(void)
{
    if (NewtRefIsNotNIL(CURREXCP))
	{
		excp_pop_handlers();
		CURREXCP = kNewtRefUnbind;
	}
}


/*------------------------------------------------------------------------*/
/** 現在の例外をクリアする
 *
 * @return		なし
 *
 * @note		ネイティブ関数で例外処理を行うために使用
 */

void NVMClearException(void)
{
	CURREXCP = kNewtRefUnbind;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 関数オブジェクトを現在の実行関数にする
 *
 * @param fn	[in] 関数オブジェクト
 *
 * @return		なし
 */

void NVMSetFn(newtRefArg fn)
{
    FUNC = fn;

    if (NewtRefIsNIL(FUNC) || ! NewtRefIsCodeBlock(FUNC))
    {
        BC = NULL;
        BCLEN = 0;
    }
    else
    {
        newtRefVar	instr;

        instr = NcGetSlot(FUNC, NSSYM0(instructions));
        BC = NewtRefToBinary(instr);
        BCLEN = NewtLength(instr);
    }
}


/*------------------------------------------------------------------------*/
/** 戻る関数スタックがない場合の処理
 *
 * @return		なし
 */

void NVMNoStackFrameForReturn(void)
{
    BC = NULL;
    BCLEN = 0;
    CALLSP = 0;
}


#if 0
#pragma mark *** 呼出しスタック
#endif
/*------------------------------------------------------------------------*/
/** レジスタの巻き戻し
 *
 * @param sp	[in] 呼出しスタックのスタックポインタ
 *
 * @return		なし
 */

void reg_rewind(int32_t sp)
{
    if (sp == CALLSP)
        return;

    if (sp < CALLSP)
    {
        CALLSP = sp;
        REG = CALLSTACK[CALLSP];
        NVMSetFn(FUNC);
    }
    else
    {
        NVMNoStackFrameForReturn();
    }
}


/*------------------------------------------------------------------------*/
/** レジスタのポップ
 *
 * @return			なし
 */

void reg_pop(void)
{
	reg_rewind(CALLSP - 1);
}


/*------------------------------------------------------------------------*/
/** レジスタのプッシュ
 *
 * @param sp		[in] スタックポインタ
 *
 * @return			なし
 */

void reg_push(int32_t sp)
{
    if (! NewtStackExpand(&vm_env.callstack, CALLSP + 1))
        return;

    CALLSTACK[CALLSP] = REG;
    CALLSTACK[CALLSP].sp = sp;
    CALLSP++;
}


/*------------------------------------------------------------------------*/
/** レジスタの保存
 *
 * @param sp		[in] スタックポインタ
 *
 * @return			なし
 */

void reg_save(int32_t sp)
{
//    PC++;
    reg_push(sp);
}


#if 0
#pragma mark *** スタック
#endif
/*------------------------------------------------------------------------*/
/** スタックのポップ
 *
 * @return			オブジェクト
 */

newtRef stk_pop0(void)
{
    newtRefVar	x = kNewtRefUnbind;

    if (0 < SP)
    {
        SP--;
        x = STACK[SP];
    }

    return x;
}


/*------------------------------------------------------------------------*/
/** スタックのポップ（マジックポインタ参照を解決）
 *
 * @return			オブジェクト
 */

newtRef stk_pop(void)
{
	return NcResolveMagicPointer(stk_pop0());
}


/*------------------------------------------------------------------------*/
/** スタックを n個ポップ
 *
 * @param n			[in] ポップする数
 * @param a			[out]ポップしたオブジェクトを格納する配列
 *
 * @return			なし
 */

void stk_pop_n(int32_t n, newtRef a[])
{
    for (n--; 0 <= n; n--)
    {
        a[n] = stk_pop();
    }
}


/*------------------------------------------------------------------------*/
/** スタックを n個削除
 *
 * @return			なし
 */

void stk_remove(uint16_t n)
{
    if (n < SP)
        SP -= n;
    else
        SP = 0;
}


/*------------------------------------------------------------------------*/
/** スタックの先頭データを取出す
 *
 * @return			オブジェクト
 */

newtRef stk_top(void)
{
    if (0 < SP)
        return STACK[SP - 1];
    else
        return kNewtRefNIL;
}


/*------------------------------------------------------------------------*/
/** スタックにオブジェクトをプッシュ
 *
 * @param value		[in] オブジェクト
 *
 * @return			なし
 */

void stk_push(newtRefArg value)
{
    if (! NewtStackExpand(&vm_env.stack, SP + 1))
        return;

    STACK[SP] = value;
    SP++;
}


/*------------------------------------------------------------------------*/
/** 可変引数をスタックにプッシュする
 *
 * @param argc		[in] 引数の数
 * @param ap		[in] 可変引数
 *
 * @return			なし
 */

void stk_push_varg(int argc, va_list ap)
{
	int		i;

	for (i = 0; i < argc; i++)
	{
		stk_push(va_arg(ap, newtRefArg));
	}
}


/*------------------------------------------------------------------------*/
/** 配列の要素をスタックにプッシュする
 *
 * @param argArray	[in] 引数配列
 *
 * @return			なし
 */

void stk_push_array(newtRefVar argArray)
{
	int		numArgs;
	int		i;

	numArgs = NewtArrayLength(argArray);

	for (i = 0; i < numArgs; i++)
	{
		stk_push(NewtGetArraySlot(argArray, i));
	}
}


#if 0
#pragma mark *** 例外ハンドラスタック
#endif
/*------------------------------------------------------------------------*/
/** 例外スタックにプッシュ
 *
 * @param sym		[in] 例外シンボル
 * @param pc		[in] プログラムカウンタ
 *
 * @retval			true	スタックされた
 * @retval			false	スタックされなかった
 */

bool excp_push(newtRefArg sym, newtRefArg pc)
{
    vm_excp_t *	excp;

    if (! NewtRefIsSymbol(sym))
        return false;

    if (! NewtRefIsInteger(pc))
        return false;

    if (! NewtStackExpand(&vm_env.excpstack, EXCPSP + 1))
        return false;

    excp = &EXCPSTACK[EXCPSP];

    excp->callsp = CALLSP;
    excp->excppc = PC;
    excp->sym = sym;
    excp->pc = NewtRefToInteger(pc);

    EXCPSP++;

    return true;
}

/*------------------------------------------------------------------------*/
/** 例外スタックをポップ
 *
 * @return			なし
 */

void excp_pop(void)
{
    if (0 < EXCPSP)
        EXCPSP--;
}

/*------------------------------------------------------------------------*/
/** 例外スタックの先頭を取得
 *
 * @return			例外構造体へのポインタ
 */

vm_excp_t * excp_top(void)
{
    if (0 < EXCPSP)
        return &EXCPSTACK[EXCPSP - 1];
    else
        return NULL;
}


/*------------------------------------------------------------------------*/
/** 例外ハンドラをポップする
 *
 * @return			なし
 */

void excp_pop_handlers(void)
{
    vm_excp_t *	excp;

    excp = excp_top();

    if (excp != NULL)
    {
        uint32_t	excppc;

        excppc = excp->excppc;

        while (excp != NULL)
        {
            if (excp->excppc != excppc)
                break;

            excp_pop();
            excp = excp_top();
        }
    }
}


#if 0
#pragma mark *** Literals
#endif
/*------------------------------------------------------------------------*/
/** リテラルを取出す
 *
 * @param n			[in] リテラルリストの位置
 *
 * @return			リテラルオブジェクト
 */

newtRef liter_get(int16_t n)
{
    newtRefVar	literals;

    literals = NcGetSlot(FUNC, NSSYM0(literals));

    if (NewtRefIsNotNIL(literals))
        return NewtGetArraySlot(literals, n);
    else
        return kNewtRefNIL;
}


#if 0
#pragma mark *** Iterator
#endif
/*------------------------------------------------------------------------*/
/** イテレータオブジェクトを作成する
 *
 * @param r			[in] オブジェクト
 * @param deeply	[in] deeply フラグ
 *
 * @return			イテレータオブジェクト
 */

newtRef iter_new(newtRefArg r, newtRefArg deeply)
{
    newtRefVar	iter;

    iter = NewtMakeArray(NSSYM0(forEachState), kIterALength);

    NewtSetArraySlot(iter, kIterObj, r);

    if (NewtRefIsNIL(deeply))
        NewtSetArraySlot(iter, kIterDeeply, deeply);
    else
        NewtSetArraySlot(iter, kIterDeeply, NcDeeplyLength(r));

    NewtSetArraySlot(iter, kIterPos, NSINT(-1));
    NewtSetArraySlot(iter, kIterMax, NcLength(r));

    if (NewtRefIsFrame(r))
        NewtSetArraySlot(iter, kIterMap, NewtFrameMap(r));
    else
        NewtSetArraySlot(iter, kIterMap, kNewtRefNIL);

    iter_next(iter);

    return iter;
}


/*------------------------------------------------------------------------*/
/** イテレータを次に進める
 *
 * @param iter		[in] イテレータオブジェクト
 *
 * @return			なし
 */

void iter_next(newtRefArg iter)
{
    newtRefVar	deeply;
    newtRefVar	obj;
    newtRefVar	index = kNewtRefUnbind;
    newtRefVar	value = kNewtRefUnbind;
    newtRefVar	map;
    int32_t	pos;
    int32_t	len;

    obj = NewtGetArraySlot(iter, kIterObj);
    deeply = NewtGetArraySlot(iter, kIterDeeply);
    pos = NewtRefToInteger(NewtGetArraySlot(iter, kIterPos));
    len = NewtRefToInteger(NewtGetArraySlot(iter, kIterMax));
    map = NewtGetArraySlot(iter, kIterMap);

    if (NewtRefIsNIL(deeply) || NewtRefIsNIL(map))
    {
        pos++;

        if (pos < len)
        {
            if (NewtRefIsNIL(map))
            {
                index = NewtMakeInteger(pos);
                value = NewtARef(obj, pos);
            }
            else
            {
                index = NewtGetArraySlot(map, pos + 1);
                value = NewtGetFrameSlot(obj, pos);
            }
        }
    }
    else
    {
        while (true)
        {
            pos++;

            if (len <= pos)
            {
                obj = NcGetSlot(obj, NSSYM0(_proto));

                if (NewtRefIsNIL(obj))
                {
                    index = kNewtRefUnbind;
                    break;
                }

                map = NewtFrameMap(obj);
                len = NewtLength(obj);

                NewtSetArraySlot(iter, kIterObj, obj);
                NewtSetArraySlot(iter, kIterMap, map);
                NewtSetArraySlot(iter, kIterMax, NewtMakeInteger(len));

                pos = -1;
                continue;
            }

            index = NewtGetArraySlot(map, pos + 1);

            if (index != NSSYM0(_proto))
            {
                value = NewtGetFrameSlot(obj, pos);
                break;
            }
        }
    }

    NewtSetArraySlot(iter, kIterIndex, index);
    NewtSetArraySlot(iter, kIterPos, NewtMakeInteger(pos));
    NewtSetArraySlot(iter, kIterValue, value);
}


/*------------------------------------------------------------------------*/
/** イテレータの終了をチェックする
 *
 * @param iter		[in] イテレータオブジェクト
 *
 * @retval			true	終了
 * @retval			false	終了していない
 */

bool iter_done(newtRefArg iter)
{
    int32_t	pos;
    int32_t	len;

    pos = NewtRefToInteger(NewtGetArraySlot(iter, kIterPos));
    len = NewtRefToInteger(NewtGetArraySlot(iter, kIterMax));

    return (len <= pos);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 引数をスタックから取出して配列にする
 *
 * @param numArgs	[in] 引数の数
 *
 * @return			配列
 */

newtRef NVMMakeArgsArray(uint16_t numArgs)
{
	newtRefVar	args;
	int16_t		i;

	args = NewtMakeArray(kNewtRefUnbind, numArgs);

	for (i = numArgs - 1; 0 <= i; i--)
	{
		NewtSetArraySlot(args, i, stk_pop());
	}

	return args;
}


/*------------------------------------------------------------------------*/
/** 引数をスタックから取出してローカルフレームに束縛する
 *
 * @param numArgs	[in] 引数の数
 *
 * @return			なし
 */

void NVMBindArgs(uint16_t numArgs)
{
    newtRefVar	indefinite;
	int32_t		minArgs;
	int16_t		i;
    newtRefVar	v;

	minArgs = NewtRefToInteger(NcGetSlot(FUNC, NSSYM0(numArgs)));
    indefinite = NcGetSlot(FUNC, NSSYM0(indefinite));

	if (NewtRefIsNotNIL(indefinite))
	{
		newtRefVar	args;

		args = NVMMakeArgsArray(numArgs - minArgs);
		NewtSetFrameSlot(LOCALS, START_LOCALARGS + minArgs, args);
	}

    for (i = START_LOCALARGS + minArgs - 1; START_LOCALARGS <= i; i--)
    {
        v = stk_pop();
        NewtSetFrameSlot(LOCALS, i, v);
    }
}


/*------------------------------------------------------------------------*/
/** 例外を発生する
 *
 * @param err		[in] エラー番号
 * @param value		[in] 値オブジェクト
 * @param pop		[in] ポップする数
 * @param push		[in] プッシュの有無
 *
 * @return			なし
 */

void NVMThrowBC(newtErr err, newtRefArg value, int16_t pop, bool push)
{
    stk_remove(pop);

    if (push)
        stk_push(kNewtRefUnbind);

	if (NewtRefIsSymbol(value))
		NewtThrowSymbol(err, value);
	else
		NewtThrow(err, value);
}


/*------------------------------------------------------------------------*/
/** 関数の引数の数をチェックする
 *
 * @param fn		[in] 関数オブジェクト
 * @param numArgs	[in] 引数の数
 *
 * @retval			true	正常
 * @retval			false	不正
 */

bool NVMFuncCheckNumArgs(newtRefArg fn, int16_t numArgs)
{
    newtRefVar	indefinite;
    int32_t		minArgs;

    minArgs = NewtRefToInteger(NcGetSlot(fn, NSSYM0(numArgs)));
    indefinite = NcGetSlot(fn, NSSYM0(indefinite));

	if (NewtRefIsNIL(indefinite))
		return (minArgs == numArgs);
	else
		return (minArgs <= numArgs);
}


/*------------------------------------------------------------------------*/
/** 関数オブジェクトをチェックする
 *
 * @param fn		[in] 関数オブジェクト
 * @param numArgs	[in] 引数の数
 *
 * @return			エラーコード
 */

newtErr NVMFuncCheck(newtRefArg fn, int16_t numArgs)
{
    // 1. 関数オブジェクトでなければ例外を発生

    if (! NewtRefIsFunction(fn))
        return kNErrInvalidFunc;

    // 2. 引数の数が一致さなければ WrongNumberOfArgs 例外を発生

    if (! NVMFuncCheckNumArgs(fn, numArgs))
        return kNErrWrongNumberOfArgs;

    return kNErrNone;
}


/*------------------------------------------------------------------------*/
/** ネイティブ関数（rcvrなし）の呼出し
 *
 * @param fn		[in] 関数オブジェクト
 * @param numArgs	[in] 引数の数
 *
 * @return			なし
 */

void NVMCallNativeFn(newtRefArg fn, int16_t numArgs)
{
    newtRefVar	r = kNewtRefUnbind;
    newtRefVar	indefinite;
    nvm_func_t	funcPtr;
	int32_t		minArgs;

    funcPtr = (nvm_func_t)NewtRefToAddress(NcGetSlot(fn, NSSYM0(funcPtr)));

	if (funcPtr == NULL)
		return;

	minArgs = NewtRefToInteger(NcGetSlot(fn, NSSYM0(numArgs)));
    indefinite = NcGetSlot(fn, NSSYM0(indefinite));

	if (NewtRefIsNIL(indefinite))
	{
		switch (minArgs)
		{
			case 0:
				r = (*funcPtr)();
				break;

			case 1:
				{
					newtRefVar	a;

					a = stk_pop();
					r = (*funcPtr)(a);
				}
				break;

			case 2:
				{
					newtRefVar	a[2];

					stk_pop_n(2, a);
					r = (*funcPtr)(a[0], a[1]);
				}
				break;

			case 3:
				{
					newtRefVar	a[3];

					stk_pop_n(3, a);
					r = (*funcPtr)(a[0], a[1], a[2]);
				}
				break;

			case 4:
				{
					newtRefVar	a[4];

					stk_pop_n(4, a);
					r = (*funcPtr)(a[0], a[1], a[2], a[3]);
				}
				break;

			case 5:
				{
					newtRefVar	a[5];

					stk_pop_n(5, a);
					r = (*funcPtr)(a[0], a[1], a[2], a[3], a[4]);
				}
				break;

			case 6:
				{
					newtRefVar	a[6];

					stk_pop_n(6, a);
					r = (*funcPtr)(a[0], a[1], a[2], a[3], a[4], a[5]);
				}
				break;

			case 7:
				{
					newtRefVar	a[7];

					stk_pop_n(7, a);
					r = (*funcPtr)(a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
				}
				break;


			case 8:
				{
					newtRefVar	a[8];

					stk_pop_n(8, a);
					r = (*funcPtr)(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
				}
				break;

			case 9:
				{
					newtRefVar	a[9];

					stk_pop_n(9, a);
					r = (*funcPtr)(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]);
				}
				break;

			default:
				stk_remove(minArgs);
				break;
		}
	}
	else
	{
		newtRefVar	args;

		args = NVMMakeArgsArray(numArgs - minArgs);

		switch (minArgs)
		{
			case 0:
				r = (*funcPtr)(args);
				break;

			case 1:
				{
					newtRefVar	a;

					a = stk_pop();
					r = (*funcPtr)(a, args);
				}
				break;

			case 2:
				{
					newtRefVar	a[2];

					stk_pop_n(2, a);
					r = (*funcPtr)(a[0], a[1], args);
				}
				break;

			case 3:
				{
					newtRefVar	a[3];

					stk_pop_n(3, a);
					r = (*funcPtr)(a[0], a[1], a[2], args);
				}
				break;

			case 4:
				{
					newtRefVar	a[4];

					stk_pop_n(4, a);
					r = (*funcPtr)(a[0], a[1], a[2], a[3], args);
				}
				break;

			case 5:
				{
					newtRefVar	a[5];

					stk_pop_n(5, a);
					r = (*funcPtr)(a[0], a[1], a[2], a[3], a[4], args);
				}
				break;

			case 6:
				{
					newtRefVar	a[6];

					stk_pop_n(6, a);
					r = (*funcPtr)(a[0], a[1], a[2], a[3], a[4], a[5], args);
				}
				break;

			case 7:
				{
					newtRefVar	a[7];

					stk_pop_n(7, a);
					r = (*funcPtr)(a[0], a[1], a[2], a[3], a[4], a[5], a[6], args);
				}
				break;


			case 8:
				{
					newtRefVar	a[8];

					stk_pop_n(8, a);
					r = (*funcPtr)(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], args);
				}
				break;

			case 9:
				{
					newtRefVar	a[9];

					stk_pop_n(9, a);
					r = (*funcPtr)(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], args);
				}
				break;

			default:
				stk_remove(minArgs);
				break;
		}
	}

    stk_push(r);
}


/*------------------------------------------------------------------------*/
/** ネイティブ関数（rcvrあり）の呼出し
 *
 * @param fn		[in] 関数オブジェクト
 * @param rcvr		[in] レシーバ
 * @param numArgs	[in] 引数の数
 *
 * @return			なし
 */

void NVMCallNativeFunc(newtRefArg fn, newtRefArg rcvr, int16_t numArgs)
{
    newtRefVar	r = kNewtRefUnbind;
    newtRefVar	indefinite;
    nvm_func_t	funcPtr;
	int32_t		minArgs;

    funcPtr = (nvm_func_t)NewtRefToAddress(NcGetSlot(fn, NSSYM0(funcPtr)));

	if (funcPtr == NULL)
		return;

	minArgs = NewtRefToInteger(NcGetSlot(fn, NSSYM0(numArgs)));
    indefinite = NcGetSlot(fn, NSSYM0(indefinite));

	if (NewtRefIsNIL(indefinite))
	{
		switch (minArgs)
		{
			case 0:
				r = (*funcPtr)(rcvr);
				break;

			case 1:
				{
					newtRefVar	a;

					a = stk_pop();
					r = (*funcPtr)(rcvr, a);
				}
				break;

			case 2:
				{
					newtRefVar	a[2];

					stk_pop_n(2, a);
					r = (*funcPtr)(rcvr, a[0], a[1]);
				}
				break;

			case 3:
				{
					newtRefVar	a[3];

					stk_pop_n(3, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2]);
				}
				break;

			case 4:
				{
					newtRefVar	a[4];

					stk_pop_n(4, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], a[3]);
				}
				break;

			case 5:
				{
					newtRefVar	a[5];

					stk_pop_n(5, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], a[3], a[4]);
				}
				break;

			case 6:
				{
					newtRefVar	a[6];

					stk_pop_n(6, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], a[3], a[4], a[5]);
				}
				break;

			case 7:
				{
					newtRefVar	a[7];

					stk_pop_n(7, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
				}
				break;


			case 8:
				{
					newtRefVar	a[8];

					stk_pop_n(8, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
				}
				break;

			case 9:
				{
					newtRefVar	a[9];

					stk_pop_n(9, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]);
				}
				break;

			default:
				stk_remove(minArgs);
				break;
		}
	}
	else
	{
		newtRefVar	args;

		args = NVMMakeArgsArray(numArgs - minArgs);

		switch (minArgs)
		{
			case 0:
				r = (*funcPtr)(rcvr, args);
				break;

			case 1:
				{
					newtRefVar	a;

					a = stk_pop();
					r = (*funcPtr)(rcvr, a, args);
				}
				break;

			case 2:
				{
					newtRefVar	a[2];

					stk_pop_n(2, a);
					r = (*funcPtr)(rcvr, a[0], a[1], args);
				}
				break;

			case 3:
				{
					newtRefVar	a[3];

					stk_pop_n(3, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], args);
				}
				break;

			case 4:
				{
					newtRefVar	a[4];

					stk_pop_n(4, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], a[3], args);
				}
				break;

			case 5:
				{
					newtRefVar	a[5];

					stk_pop_n(5, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], a[3], a[4], args);
				}
				break;

			case 6:
				{
					newtRefVar	a[6];

					stk_pop_n(6, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], a[3], a[4], a[5], args);
				}
				break;

			case 7:
				{
					newtRefVar	a[7];

					stk_pop_n(7, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], a[3], a[4], a[5], a[6], args);
				}
				break;


			case 8:
				{
					newtRefVar	a[8];

					stk_pop_n(8, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], args);
				}
				break;

			case 9:
				{
					newtRefVar	a[9];

					stk_pop_n(9, a);
					r = (*funcPtr)(rcvr, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], args);
				}
				break;

			default:
				stk_remove(minArgs);
				break;
		}
	}

    stk_push(r);
}


/*------------------------------------------------------------------------*/
/** 関数オブジェクトの呼出し
 *
 * @param fn		[in] 関数オブジェクト
 * @param numArgs	[in] 引数の数
 *
 * @return			なし
 */

void NVMFuncCall(newtRefArg fn, int16_t numArgs)
{
    newtErr	err;
	int		type;

    // 1. 関数オブジェクトでなければ例外を発生
    // 2. 引数の数が一致さなければ WrongNumberOfArgs 例外を発生

    err = NVMFuncCheck(fn, numArgs);

    if (err != kNErrNone)
    {
        NVMThrowBC(err, fn, numArgs, true);
        return;
    }

	type = NewtRefFunctionType(fn);

    if (type == kNewtNativeFn || type == kNewtNativeFunc)
    {	// ネイティブ関数の呼出し
    	// Save CALLSP to know if an exception occurred.
    	uint32_t saveCALLSP;
		reg_save(SP - numArgs + 1);
		FUNC = fn;
		saveCALLSP = CALLSP;

		switch (type)
		{
			case kNewtNativeFn:
				// rcvrなし(old style)
				NVMCallNativeFn(fn, numArgs);
				break;

			case kNewtNativeFunc:
				// rcvrあり(new style)
				NVMCallNativeFunc(fn, kNewtRefUnbind, numArgs);
				break;
		}

        if (saveCALLSP == CALLSP)
        {
			reg_pop();
		}
        return;
    }

    reg_save(SP - numArgs); // 3. VM レジスタを保存（PC の更新は...）
    NVMSetFn(fn);			// 4. FUNC に新しい関数オブジェクトをセット
    PC = 0;					// 5. PC に 0 をセット

    // 6. ローカルフレーム（FUNC.argFrame）をクローンして LOCALS にセット
    LOCALS = NcClone(NcGetSlot(FUNC, NSSYM0(argFrame)));

    // 7. LOCALS の引数スロットにスタックにある引数をセットする
    //    引数は LOCALS の４番スロットから開始し左から右へ挿入される
    NVMBindArgs(numArgs);

    // 8. LOCALS の _parent スロットを RCVR にセット
    RCVR = NcGetSlot(LOCALS, NSSYM0(_parent));

    // 9. LOCALS の _implementor スロットを IMPL にセット
    IMPL = NcGetSlot(LOCALS, NSSYM0(_implementor));

    // 10. 実行をリジュームする
}


/*------------------------------------------------------------------------*/
/** メソッドの送信
 *
 * @param impl		[in] インプリメンタ
 * @param receiver	[in] レシーバ
 * @param fn		[in] 関数オブジェクト
 * @param numArgs	[in] 引数の数
 *
 * @return			なし
 */

void NVMMessageSend(newtRefArg impl, newtRefArg receiver, newtRefArg fn, int16_t numArgs)
{
    newtErr	err;
	int		type;

    // 1. メソッドが関数オブジェクトでなければ例外を発生
    // 2. 引数の数が一致さなければ WrongNumberOfArgs 例外を発生

    err = NVMFuncCheck(fn, numArgs);

    if (err != kNErrNone)
    {
        NVMThrowBC(err, fn, numArgs, true);
        return;
    }

	type = NewtRefFunctionType(fn);

    if (type == kNewtNativeFn || type == kNewtNativeFunc)
    {	// ネイティブ関数の呼出し
    	// Save CALLSP to know if an exception occurred.
    	uint32_t saveCALLSP;
		reg_save(SP - numArgs + 1);
		FUNC = fn;
		RCVR = receiver;
		IMPL = impl;
		saveCALLSP = CALLSP;

		switch (type)
		{
			case kNewtNativeFn:
				// rcvrなし(old style)
				NVMCallNativeFn(fn, numArgs);
				break;

			case kNewtNativeFunc:
				// rcvrあり(new style)
				NVMCallNativeFunc(fn, receiver, numArgs);
				break;
		}

		if (saveCALLSP == CALLSP)
		{
			reg_pop();
		}
        return;
    }

    reg_save(SP - numArgs); // 3. VM レジスタを保存（PC の更新は...）
    NVMSetFn(fn);			// 4. FUNC にメソッドをセット
    PC = 0;					// 5. PC に 0 をセット
    RCVR = receiver;		// 6. RCVR に receiver をセット
    IMPL = impl;			// 7. IMPL IMPL implementor をセット

    // 8. ローカルフレーム（FUNC.argFrame）をクローンして LOCALS にセット
    LOCALS = NcClone(NcGetSlot(FUNC, NSSYM0(argFrame)));

    // 9. RCVR を LOCALS._parent にセット
    NcSetSlot(LOCALS, NSSYM0(_parent), RCVR);

    // 10. IMPL を LOCALS._implementor にセット
    NcSetSlot(LOCALS, NSSYM0(_implementor), IMPL);

    // 11. LOCALS の引数スロットにスタックにある引数をセットする
    //     引数は LOCALS の４番スロットから開始し左から右へ挿入される
    NVMBindArgs(numArgs);

    // 12. 実行をリジュームする
}


/*------------------------------------------------------------------------*/
/** メソッドの送信
 *
 * @param b			[in] オペコード
 * @param errP		[out]エラー番号
 *
 * @return			メソッド名
 */

newtRef	vm_send(int16_t b, newtErr * errP)
{
	// NewtonFormats say:
    // arg1 arg2 ... argN name receiver -- result
    // But in the Newton, this is:
    // arg1 arg2 ... argN receiver name -- result

    newtRefVar	receiver;
    newtRefVar	impl;
    newtRefVar	name;
    newtRefVar	fn;
    newtErr	err = kNErrNone;

	if (errP != NULL)
		*errP = kNErrNone;

    name = stk_pop();
    receiver = stk_pop();

	if (! NewtRefIsSymbol(name))
	{
		NVMThrowBC(kNErrNotASymbol, name, b, true);
		return name;
	}

	if (! NewtRefIsFrame(receiver) && ! NewtRefIsNIL(receiver))
	{
		NVMThrowBC(kNErrNotAFrame, receiver, b, true);
		return name;
	}

    impl = NcFullLookupFrame(receiver, name);

    if (impl != kNewtRefUnbind)
	{
		fn = NcGetSlot(impl, name);
        NVMMessageSend(impl, receiver, fn, b);
    }
	else
	{
        err = kNErrUndefinedMethod;
	}

	if (errP != NULL)
		*errP = err;

    return name;
}


/*------------------------------------------------------------------------*/
/** メソッドの再送信
 *
 * @param b			[in] オペコード
 * @param errP		[out]エラー番号
 *
 * @return			メソッド名
 */

newtRef vm_resend(int16_t b, newtErr * errP)
{
    newtRefVar	name;
    newtErr	err = kNErrNone;

	if (errP != NULL)
		*errP = kNErrNone;

    // arg1 arg2 ... argN name -- result

	name = stk_pop();

	if (! NewtRefIsSymbol(name))
	{
		NVMThrowBC(kNErrNotASymbol, name, b, true);
		return name;
	}

    if (NewtHasSlot(IMPL, NSSYM0(_proto)))
	{
		newtRefVar	impl;
		newtRefVar	fn;

		impl = NcGetSlot(IMPL, NSSYM0(_proto));
		impl = NcProtoLookupFrame(impl, name);

		if (impl != kNewtRefUnbind)
		{
			fn = NcGetSlot(impl, name);
			NVMMessageSend(impl, RCVR, fn, b);
		}
		else
		{
			err= kNErrUndefinedMethod;
		}
	}
	else
	{
		err = kNErrUndefinedMethod;
	}

	if (errP != NULL)
		*errP = err;

    return name;
}


#if 0
#pragma mark -
#pragma mark *** Simple instructions
#endif
/*------------------------------------------------------------------------*/
/** スタックのポップ
 *
 * @return			なし
 */

void si_pop(void)
{
    stk_pop0();
}


/*------------------------------------------------------------------------*/
/** スタックの先頭を複製してプッシュする
 *
 * @return			なし
 */

void si_dup(void)
{
    stk_push(stk_top());
}


/*------------------------------------------------------------------------*/
/** 関数のリターン
 *
 * @return			なし
 */

void si_return(void)
{
    newtRefVar	r;

    r = stk_top();
    reg_pop();
    stk_push(r);
}


/*------------------------------------------------------------------------*/
/** self をスタックにプッシュする
 *
 * @return			なし
 */

void si_pushself(void)
{
    stk_push(RCVR);
}


/*------------------------------------------------------------------------*/
/** レキシカルスコープをセットする
 *
 * @return			なし
 */

void si_set_lex_scope(void)
{
    newtRefVar	fn;
    newtRefVar	af;

    fn = NcClone(stk_pop());
    af = NcClone(NcGetSlot(fn, NSSYM0(argFrame)));
    NcSetSlot(af, NSSYM0(_nextArgFrame), LOCALS);
    NcSetSlot(af, NSSYM0(_parent), RCVR);
    NcSetSlot(af, NSSYM0(_implementor), IMPL);
    NcSetSlot(fn, NSSYM0(argFrame), af);
    stk_push(fn);
}


/*------------------------------------------------------------------------*/
/** イテレータを次に進める
 *
 * @return			なし
 */

void si_iternext(void)
{
    iter_next(stk_pop());
}


/*------------------------------------------------------------------------*/
/** イテレータが終了をチェック
 *
 * @return			なし
 */

void si_iterdone(void)
{
    newtRefVar	iter;

    iter = stk_pop();
    stk_push(NewtMakeBoolean(iter_done(iter)));
}


/*------------------------------------------------------------------------*/
/** 例外ハンドラをポップする
 *
 * @return			なし
 */

void si_pop_handlers(void)
{
    excp_pop_handlers();
    CURREXCP = kNewtRefUnbind;
}


#if 0
#pragma mark -
#pragma mark *** Primitive functions
#endif
/*------------------------------------------------------------------------*/
/** 加算
 *
 * @return			なし
 */

void fn_add(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcAdd(a1, a2));
}


/*------------------------------------------------------------------------*/
/** 減算
 *
 * @return			なし
 */

void fn_subtract(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcSubtract(a1, a2));
}


/*------------------------------------------------------------------------*/
/** オブジェクトの指定された位置から値を取得
 *
 * @return			なし
 */

void fn_aref(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcARef(a1, a2));
}


/*------------------------------------------------------------------------*/
/** オブジェクトの指定された位置に値をセットする
 *
 * @return			なし
 */

void fn_set_aref(void)
{
    newtRefVar	a1;
    newtRefVar	a2;
    newtRefVar	a3;

    a3 = stk_pop0();
    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcSetARef(a1, a2, a3));
}


/*------------------------------------------------------------------------*/
/** オブジェクトの同値をチェック
 *
 * @return			なし
 */

void fn_equals(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcRefEqual(a1, a2));
}


/*------------------------------------------------------------------------*/
/** ブール値の否定
 *
 * @return			なし
 */

void fn_not(void)
{
    newtRefVar	x;
    newtRefVar	r;

    x = stk_pop();
    r = NewtMakeBoolean(NewtRefIsNIL(x));

    stk_push(r);
}


/*------------------------------------------------------------------------*/
/** オブジェクトの違いをチェック
 *
 * @return			なし
 */

void fn_not_equals(void)
{
    newtRefVar	a1;
    newtRefVar	a2;
    newtRefVar	r;

    a2 = stk_pop();
    a1 = stk_pop();

    r = NewtMakeBoolean(! NewtRefEqual(a1, a2));

    stk_push(r);
}


/*------------------------------------------------------------------------*/
/** 乗算
 *
 * @return			なし
 */

void fn_multiply(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcMultiply(a1, a2));
}


/*------------------------------------------------------------------------*/
/** 割算
 *
 * @return			なし
 */

void fn_divide(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcDivide(a1, a2));
}


/*------------------------------------------------------------------------*/
/** 整数の割算
 *
 * @return			なし
 */

void fn_div(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcDiv(a1, a2));
}


/*------------------------------------------------------------------------*/
/** オブジェクトの比較(<)
 *
 * @return			なし
 */

void fn_less_than(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcLessThan(a1, a2));
}


/*------------------------------------------------------------------------*/
/** オブジェクトの比較(>)
 *
 * @return			なし
 */

void fn_greater_than(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcGreaterThan(a1, a2));
}


/*------------------------------------------------------------------------*/
/** オブジェクトの比較(>=)
 *
 * @return			なし
 */

void fn_greater_or_equal(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcGreaterOrEqual(a1, a2));
}


/*------------------------------------------------------------------------*/
/** オブジェクトの比較(<=)
 *
 * @return			なし
 */

void fn_less_or_equal(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcLessOrEqual(a1, a2));
}


/*------------------------------------------------------------------------*/
/** ビット演算 AND
 *
 * @return			なし
 */

void fn_bit_and(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcBAnd(a1, a2));
}


/*------------------------------------------------------------------------*/
/** ビット演算 OR
 *
 * @return			なし
 */

void fn_bit_or(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcBOr(a1, a2));
}


/*------------------------------------------------------------------------*/
/** ビット演算 NOT
 *
 * @return			なし
 */

void fn_bit_not(void)
{
    newtRefVar	x;

    x = stk_pop();
    stk_push(NcBNot(x));
}


/*------------------------------------------------------------------------*/
/** イテレータの作成
 *
 * @return			なし
 */

void fn_new_iterator(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    if (! NewtRefIsFrameOrArray(a1) && ! NewtRefIsBinary(a1))
    {
        NewtThrow(kNErrNotAFrameOrArray, a1);
        return;
    }

    stk_push(iter_new(a1, a2));
}


/*------------------------------------------------------------------------*/
/** オブジェクトの長さを取得
 *
 * @return			なし
 */

void fn_length(void)
{
    newtRefVar	x;

    x = stk_pop();
    stk_push(NcLength(x));
}


/*------------------------------------------------------------------------*/
/** オブジェクトをクローン複製する
 *
 * @return			なし
 */

void fn_clone(void)
{
    newtRefVar	x;

    x = stk_pop();
    stk_push(NcClone(x));
}


/*------------------------------------------------------------------------*/
/** オブジェクトにクラスをセットする
 *
 * @return			なし
 */

void fn_set_class(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcSetClass(a1, a2));
}


/*------------------------------------------------------------------------*/
/** 配列オブジェクトにオブジェクトを追加する
 *
 * @return			なし
 */

void fn_add_array_slot(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop0();
    a1 = stk_pop();

    stk_push(NcAddArraySlot(a1, a2));
}


/*------------------------------------------------------------------------*/
/** 配列オブジェクトの要素を文字列に合成する
 *
 * @return			なし
 */

void fn_stringer(void)
{
    newtRefVar	x;

    x = stk_pop();
    stk_push(NcStringer(x));
}


/*------------------------------------------------------------------------*/
/** オブジェクト内のアクセスパスの有無を調べる
 *
 * @return			なし
 */

void fn_has_path(void)
{
    newtRefVar	a1;
    newtRefVar	a2;

    a2 = stk_pop();
    a1 = stk_pop();

    stk_push(NcHasPath(a1, a2));
}


/*------------------------------------------------------------------------*/
/** オブジェクトのクラスシンボルを取得
 *
 * @return			なし
 */

void fn_classof(void)
{
    newtRefVar	x;

    x = stk_pop();
    stk_push(NcClassOf(x));
}


#if 0
#pragma mark -
#pragma mark *** Instructions
#endif
/*------------------------------------------------------------------------*/
/** 命令セットテーブル登録用のダミー
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_dummy(int16_t b)
{
    NewtThrow0(kNErrInvalidInstruction);
}


/*------------------------------------------------------------------------*/
/** シンプル命令の実行
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_simple_instructions(int16_t b)
{
    if (b < kNBCSimpleInstructionsLen)
        (simple_instructions[b])();
    else
        NewtThrow0(kNErrInvalidInstruction);
}


/*------------------------------------------------------------------------*/
/** スタックにプッシュ
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_push(int16_t b)
{
    stk_push(liter_get(b));
}


/*------------------------------------------------------------------------*/
/** スタックに定数をプッシュ
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_push_constant(int16_t b)
{
    newtRefVar	r;

    r = (newtRef)b;

    if (NewtRefIsInteger(r))
	{
		int32_t	n;

		n = NewtRefToInteger(r);

		if (8191 < n)
		{	// 負の数
			n |= 0xFFFFC000;
			r = NewtMakeInt30(r);
		}
	}
	else
	{
        r = (r & 0xffff);
	}

    stk_push(r);
}


/*------------------------------------------------------------------------*/
/** グローバル関数の呼出し
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_call(int16_t b)
{
    newtRefVar	name;
    newtRefVar	fn;

    name = stk_pop();
    fn = NcGetGlobalFn(name);

    if (NewtRefIsNIL(fn))
    {
        NVMThrowBC(kNErrUndefinedGlobalFunction, name, b, true);
        return;
    }

    NVMFuncCall(fn, b);
}


/*------------------------------------------------------------------------*/
/** 関数オブジェクトの実行
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_invoke(int16_t b)
{
    NVMFuncCall(stk_pop(), b);
}


/*------------------------------------------------------------------------*/
/** メソッドの送信
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_send(int16_t b)
{
	newtRef name;
    newtErr	err;

    name = vm_send(b, &err);

    if (err != kNErrNone)
        NVMThrowBC(err, name, 0, true);
}


/*------------------------------------------------------------------------*/
/** メソッドの送信
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 *
 * @note		メソッドが定義されていなくても例外は発生しない
 */

void is_send_if_defined(int16_t b)
{
    newtErr	err;

    vm_send(b, &err);

    if (err != kNErrNone)
        stk_push(kNewtRefNIL);
}


/*------------------------------------------------------------------------*/
/** メソッドの再送信
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_resend(int16_t b)
{
	newtRef name;
    newtErr	err;

    name = vm_resend(b, &err);

    if (err != kNErrNone)
        NVMThrowBC(err, name, 0, true);
}


/*------------------------------------------------------------------------*/
/** メソッドの再送信
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 *
 * @note		メソッドが定義されていなくても例外は発生しない
 */

void is_resend_if_defined(int16_t b)
{
    newtErr	err;

    vm_resend(b, &err);

    if (err != kNErrNone)
        stk_push(kNewtRefNIL);
}


/*------------------------------------------------------------------------*/
/** 分岐
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_branch(int16_t b)
{
    PC = b;
}


/*------------------------------------------------------------------------*/
/** 条件付き分岐（スタックの先頭が真の場合）
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_branch_if_true(int16_t b)
{
    newtRefVar	x;

    x = stk_pop();

    if (NewtRefIsNotNIL(x))
        PC = b;
}


/*------------------------------------------------------------------------*/
/** 条件付き分岐（スタックの先頭が偽の場合）
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_branch_if_false(int16_t b)
{
    newtRefVar	x;

    x = stk_pop();

    if (NewtRefIsNIL(x))
        PC = b;
}


/*------------------------------------------------------------------------*/
/** 変数の検索
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_find_var(int16_t b)
{
    newtRefVar	name;
    newtRefVar	v;

    name = liter_get(b);

    v = NcLexicalLookup(LOCALS, name);

    if (v != kNewtRefUnbind)
    {
        stk_push(v);
        return;
    }

    v = NcFullLookup(RCVR, name);

    if (v != kNewtRefUnbind)
    {
        stk_push(v);
        return;
    }

    if (NewtHasGlobalVar(name))
    {
        stk_push(NcGetGlobalVar(name));
        return;
    }

    NewtThrowSymbol(kNErrUndefinedVariable, name);
    stk_push(kNewtRefUnbind);
}


/*------------------------------------------------------------------------*/
/** ローカル変数の取出し
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_get_var(int16_t b)
{
    stk_push(NewtGetFrameSlot(LOCALS, b));
}


/*------------------------------------------------------------------------*/
/** フレームの作成
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_make_frame(int16_t b)
{
    newtRefVar	map;
    newtRefVar	f;
    newtRefVar	v;
    int16_t	i;

  int16_t mapLength;

    map = stk_pop();
  mapLength = NewtLength(map) - 1;

  // NewtonScript Bytecode Interpreter Specification:
  // The B field may contain a number less than the number of slots in the frame, in which
  // case the remaining slots at the end of the frame are set to nil.
  
  f = NewtMakeFrame(map, mapLength);
  
  for (i = mapLength - 1; b <= i; i--) {
    NewtSetFrameSlot(f, i, kNewtRefNIL);
  }

    for (i = b - 1; 0 <= i; i--)
    {
        v = stk_pop0();
        NewtSetFrameSlot(f, i, v);
    }

    stk_push(f);
}


/*------------------------------------------------------------------------*/
/** 配列の作成
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_make_array(int16_t b)
{
    newtRefVar	c;
    newtRefVar	x;
    newtRefVar	a = kNewtRefNIL;

    c = stk_pop();

    if ((uint16_t)b == 0xFFFF)
    {
        x = stk_pop();

        if (NewtRefIsInteger(x))
            a = NewtMakeArray(kNewtRefUnbind, NewtRefToInteger(x));
    }
    else
    {
        a = NewtMakeArray(kNewtRefUnbind, b);

        if (NewtRefIsNotNIL(a))
        {
            int16_t	i;

            for (i = b - 1; 0 <= i; i--)
            {
                x = stk_pop0();
                NewtSetArraySlot(a, i, x);
            }
        }
    }

    if (NewtRefIsNotNIL(a) && NewtRefIsNotNIL(c))
        NcSetClass(a, c);

    stk_push(a);
}


/*------------------------------------------------------------------------*/
/** オブジェクトのアクセスパスの値を取出す
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_get_path(int16_t b)
{
    newtRefVar	a1;
    newtRefVar	a2;
    newtRefVar	r = kNewtRefNIL;

    a2 = stk_pop();
    a1 = stk_pop();

    if (NewtRefIsNotNIL(a1))
    {
        r = NcGetPath(a1, a2);
    }
    else
    {
        if (b != 0)
            NewtThrow0(kNErrPathFailed);
    }

    stk_push(r);
}


/*------------------------------------------------------------------------*/
/** オブジェクトのアクセスパスに値をセットする
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_set_path(int16_t b)
{
    newtRefVar	a1;
    newtRefVar	a2;
    newtRefVar	a3;
    newtRefVar	r = kNewtRefNIL;

    a3 = stk_pop0();
    a2 = stk_pop();
    a1 = stk_pop();

    if (NewtRefIsNotNIL(a1))
        r = NcSetPath(a1, a2, a3);

    if (b == 1)
        stk_push(r);
}


/*------------------------------------------------------------------------*/
/** ローカル変数に値をセットする
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_set_var(int16_t b)
{
    newtRefVar	x;

    x = stk_pop0();
    NewtSetFrameSlot(LOCALS, b, x);
}


/*------------------------------------------------------------------------*/
/** 変数に値をセットする
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_find_and_set_var(int16_t b)
{
    newtRefVar	name;
    newtRefVar	v;

    name = liter_get(b);
    v = stk_pop0();

    if (NewtLexicalAssignment(LOCALS, name, v))
        return;

    if (NewtAssignment(RCVR, name, v))
        return;

    if (NewtHasGlobalVar(name))
    {
        NcDefGlobalVar(name, v);
        return;
    }

    NcSetSlot(LOCALS, name, v);
}


/*------------------------------------------------------------------------*/
/** 増加
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_incr_var(int16_t b)
{
    newtRefVar	addend;
    newtRefVar	n;
    int32_t	v;

    addend = stk_top();
    n = NewtGetFrameSlot(LOCALS, b);
    v = NewtRefToInteger(n) + NewtRefToInteger(addend);
    n = NewtMakeInteger(v);

    NewtSetFrameSlot(LOCALS, b, n);
    stk_push(n);
}


/*------------------------------------------------------------------------*/
/** ループ終了条件でブランチ
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_branch_if_loop_not_done(int16_t b)
{
    newtRefVar	r_incr;
    newtRefVar	r_index;
    newtRefVar	r_limit;
    int32_t	incr;
    int32_t	index;
    int32_t	limit;

    r_limit = stk_pop();
    r_index = stk_pop();
    r_incr = stk_pop();

    if (! NewtRefIsInteger(r_incr))
    {
        NewtThrow(kNErrNotAnInteger, r_incr);
        return;
    }

    if (! NewtRefIsInteger(r_index))
    {
        NewtThrow(kNErrNotAnInteger, r_index);
        return;
    }

    if (! NewtRefIsInteger(r_limit))
    {
        NewtThrow(kNErrNotAnInteger, r_limit);
        return;
    }

    incr = NewtRefToInteger(r_incr);
    index = NewtRefToInteger(r_index);
    limit = NewtRefToInteger(r_limit);

    if (incr == 0)
    {
        NewtThrow(kNErrZeroForLoopIncr, incr);
        return;
    }

    if (incr > 0 && index <= limit)
        PC = b;
    else if (incr < 0 && index >= limit)
        PC = b;
}


/*------------------------------------------------------------------------*/
/** 関数命令の実行
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_freq_func(int16_t b)
{
    if (b < kBCFuncsLen)
        (fn_instructions[b])();
    else
        NewtThrow0(kNErrInvalidInstruction);
}


/*------------------------------------------------------------------------*/
/** 例外ハンドラの作成
 *
 * @param b		[in] オペデータ
 *
 * @return		なし
 */

void is_new_handlers(int16_t b)
{
    newtRefVar	a1;
    newtRefVar	a2;
    int16_t	i;

    for (i = 0; i < b; i++)
    {
        a2 = stk_pop();
        a1 = stk_pop();

        if (! excp_push(a1, a2))
        {
            if (0 < i)
                excp_pop_handlers();
        }
    }
}


#if 0
#pragma mark *** ダンプ
#endif
/*------------------------------------------------------------------------*/
/** 出力ファイルに命令コードの名前をダンプ出力
 *
 * @param f			[in] 出力ファイル
 * @param a			[in] オペコード
 * @param b			[in] オペデータ
 *
 * @return			なし
 */

void NVMDumpInstName(FILE * f, uint8_t a, int16_t b)
{
    bool	dump_b;
    char *	name;

    dump_b = false;
    name = NULL;

    if (a < kNBCInstructionsLen)
    {
        switch (a)
        {
            case 0:
                if (b < kNBCSimpleInstructionsLen)
                    name = simple_instruction_names[b];
                break;

            case 24:	// freq-func (A = 24)
                if (b < kBCFuncsLen)
                    name = fn_instruction_names[b];
                break;

            default:
                name = vm_instruction_names[a];
                dump_b = true;
                break;
        }
    }

    if (name == NULL)
        name = "bad instructions";

    NewtFprintf(f, " %24s", name);

    if (dump_b)
        NewtFprintf(f, " (b = % 3d)", b);
    else
        NewtFprintf(f, "%10c", ' ');
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにスタックの先頭とローカルフレームをダンプ出力
 *
 * @param f			[in] 出力ファイル
 *
 * @return			なし
 */

void NVMDumpInstResult(FILE * f)
{
    NewtFprintf(f, " ");
    NVMDumpStackTop(f, "\t");
    NewtPrintObj(f, LOCALS);
    NewtFprintf(f, "\n");
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに命令コードをダンプ出力
 *
 * @param f			[in] 出力ファイル
 * @param bc		[in] バイトコード
 * @param pc		[in] プログラムカウンタ
 * @param len		[in] 命令コードのバイト数
 *
 * @return			なし
 */

void NVMDumpInstCode(FILE * f, uint8_t * bc, uint32_t pc, uint16_t len)
{
    NewtFprintf(stderr, "%6d : %02x", pc, bc[pc]);

    if (len == 3)
    {
        NewtFprintf(f, " %02x", bc[pc + 1]);
        NewtFprintf(f, " %02x", bc[pc + 2]);
    }
    else
    {
        NewtFprintf(f, "      ");
    }
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにスタックの先頭をダンプ出力
 *
 * @param f			[in] 出力ファイル
 * @param s			[in] 区切り文字列
 *
 * @return			なし
 */

void NVMDumpStackTop(FILE * f, char * s)
{
    NewtPrintObj(f, stk_top());

    if (s != NULL)
        NewtFputs(s, f);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにバイトコードをダンプ出力
 *
 * @param f			[in] 出力ファイル
 * @param bc		[in] バイトコード
 * @param len		[in] バイトコードの長さ
 *
 * @return			なし
 */

void NVMDumpCode(FILE * f, uint8_t * bc, uint32_t len)
{
    uint8_t	op;
    uint8_t	a;
    int16_t	b;
    uint16_t	oplen;
    uint32_t	pc = 0;

    while (pc < len)
    {
        op = bc[pc];
        b = op & kNBCFieldMask;
        a = (op & 0xff)>>3;

        if (b == kNBCFieldMask)
        {
            oplen = 3;

            b = (int16_t)bc[pc + 1] << 8;
            b += bc[pc + 2];
    
            if (a == 0)
            {
                if (b == 0x01)
                    b = kNBCFieldMask;
            }
        }
        else
        {
            oplen = 1;
        }

        NVMDumpInstCode(f, bc, pc, oplen);
        NVMDumpInstName(f, a, b);
        NewtFprintf(f, "\n");

        pc += oplen;
    }
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにバイトコードをダンプ出力
 *
 * @param f				[in] 出力ファイル
 * @param instructions	[in] バイトコード
 *
 * @return			なし
 */

void NVMDumpBC(FILE * f, newtRefArg instructions)
{
    if (NewtRefIsBinary(instructions))
    {
        uint8_t *	bc;
        uint32_t	len;
    
        len = NewtLength(instructions);
        bc = NewtRefToBinary(instructions);

        NVMDumpCode(f, bc, len);
    }
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに関数オブジェクトをダンプ出力
 *
 * @param f			[in] 出力ファイル
 * @param func		[in] 関数オブジェクト
 *
 * @return			なし
 */

void NVMDumpFn(FILE * f, newtRefArg func)
{
    newtRefVar	fn;

    fn = (newtRef)func;

    if (NewtRefIsNIL(fn))
        fn = FUNC;
	else if (NewtRefIsSymbol(fn))
		fn = NcGetGlobalFn(fn);

    if (NewtRefIsFunction(fn))
    {
        newtRefVar	instructions;
    
        NewtPrintObject(f, fn);
        NewtFprintf(f, "\n");
    
        instructions = NcGetSlot(fn, NSSYM0(instructions));
        NVMDumpBC(f, instructions);
    }
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにスタックをダンプ出力
 *
 * @param f			[in] 出力ファイル
 *
 * @return			なし
 */

void NVMDumpStacks(FILE * f)
{
    int32_t	i;

    for (i = SP - 1; 0 <= i; i--)
    {
        NewtPrintObject(f, STACK[i]);
    }
}


#if 0
#pragma mark -
#pragma mark *** インタプリタ
#endif
/*------------------------------------------------------------------------*/
/** VM環境をプッシュする
 *
 * @param next		[in] 新しいVM環境
 *
 * @return			なし
 */

void vm_env_push(vm_env_t * next)
{
	*next = vm_env;
	vm_env.next = next;
}


/*------------------------------------------------------------------------*/
/** VM環境をポップする
 *
 * @return			なし
 */

void vm_env_pop(void)
{
	vm_env_t *	next = vm_env.next;

	if (next)
	{
		vm_env = *next;
	}
}


/*------------------------------------------------------------------------*/
/** レジスタの初期化 
 *
 * @return			なし
 */

void NVMInitREG(void)
{
    FUNC = kNewtRefNIL;
    PC = 0;
    SP = 0;
    LOCALS = kNewtRefNIL;
    RCVR = kNewtRefNIL;
    IMPL = kNewtRefNIL;

    BC = NULL;
    BCLEN = 0;
}


/*------------------------------------------------------------------------*/
/** スタックの初期化 
 *
 * @return			なし
 */

void NVMInitSTACK(void)
{
    NewtStackSetup(&vm_env.stack, NEWT_POOL, sizeof(newtRef), NEWT_NUM_STACK);
    NewtStackSetup(&vm_env.callstack, NEWT_POOL, sizeof(vm_reg_t), NEWT_NUM_CALLSTACK);
    NewtStackSetup(&vm_env.excpstack, NEWT_POOL, sizeof(vm_excp_t), NEWT_NUM_EXCPSTACK);

    CURREXCP = kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** スタックの後始末 
 *
 * @return			なし
 */

void NVMCleanSTACK(void)
{
    NewtStackFree(&vm_env.stack);
    NewtStackFree(&vm_env.callstack);
    NewtStackFree(&vm_env.excpstack);
}


/*------------------------------------------------------------------------*/
/** 必須グローバル関数の初期化 
 *
 * @return			なし
 */

void NVMInitGlobalFns0(void)
{
    NewtDefGlobalFunc(NSSYM0(hasVariable),	NsHasVariable,		2, "HasVariable(frame, name)");
    NewtDefGlobalFunc(NSSYM0(hasVar),		NsHasVar,			1, "HasVar(name)");
  NewtDefGlobalFunc(NSSYM(Ref),		NsRef,			1, "Ref(integer)");
  NewtDefGlobalFunc(NSSYM(RefOf),		NsRefOf,			1, "RefOf(object)");
    NewtDefGlobalFunc(NSSYM0(defGlobalFn),	NsDefGlobalFn,		2, "DefGlobalFn(name, fn)");
    NewtDefGlobalFunc(NSSYM0(defGlobalVar),	NsDefGlobalVar,		2, "DefGlobalVar(name, value)");
//    NewtDefGlobalFunc(NSSYM0(and),			NsAnd,				2, "And(n1, n2)");
//    NewtDefGlobalFunc(NSSYM0(or),			NsOr,				2, "Or(n1, n2)");
    NewtDefGlobalFunc(NSSYM0(mod),			NsMod,				2, "Mod(n1, n2)");
    NewtDefGlobalFunc(NSSYM0(shiftLeft),	NsShiftLeft,		2, "ShiftLeft(n1, n2)");
    NewtDefGlobalFunc(NSSYM0(shiftRight),	NsShiftRight,		2, "ShiftRight(n1, n2)");
  NewtDefGlobalFunc(NSSYM(negate),	NsNegate,		1, "negate(number)");
  NewtDefGlobalFunc(NSSYM(<<),	NsShiftLeft,		2, "<<(n1, n2)");
  NewtDefGlobalFunc(NSSYM(>>),	NsShiftRight,		2, ">>(n1, n2)");
    NewtDefGlobalFunc(NSSYM0(objectEqual),	NsObjectEqual,		2, "ObjectEqual(obj1, obj2)");		// 独自拡張
    NewtDefGlobalFunc(NSSYM0(defMagicPointer),NsDefMagicPointer,2, "DefMagicPointer(mp, value)");	// 独自拡張

#ifdef __NAMED_MAGIC_POINTER__
    NewtDefGlobalFunc(NSSYM0(makeRegex),	NsMakeRegex,		2, "MakeRegex(pattern, opt)");		// 独自拡張
#endif /* __NAMED_MAGIC_POINTER__ */

	NewtDefGlobalFunc(NSSYM(RemoveSlot),	NsRemoveSlot,		2, "RemoveSlot(obj, slot)");

    NewtDefGlobalFunc(NSSYM(Throw),			NsThrow,			2, "Throw(name, data)");
    NewtDefGlobalFunc(NSSYM(Rethrow),		NsRethrow,			0, "Rethrow()");
    NewtDefGlobalFunc(NSSYM(CurrentException),NsCurrentException,	0, "CurrentException()");
}


/*------------------------------------------------------------------------*/
/** 組込みグローバル関数の初期化
 *
 * @return			なし
 */

void NVMInitGlobalFns1(void)
{
    NewtDefGlobalFunc(NSSYM(PrimClassOf),	NsPrimClassOf,		1, "PrimClassOf(obj)");
    NewtDefGlobalFunc(NSSYM(TotalClone),	NsTotalClone,		1, "TotalClone(obj)");
    NewtDefGlobalFunc(NSSYM(HasSubclass),	NsHasSubclass,		2, "HasSubclass(sub, super)");
    NewtDefGlobalFunc(NSSYM(IsSubclass),	NsIsSubclass,		2, "IsSubclass(sub, super)");
    NewtDefGlobalFunc(NSSYM(IsInstance),	NsIsInstance,		2, "IsInstance(obj, class)");
    NewtDefGlobalFunc(NSSYM(IsArray),		NsIsArray,			1, "IsArray(obj)");
    NewtDefGlobalFunc(NSSYM(IsFrame),		NsIsFrame,			1, "IsFrame(obj)");
    NewtDefGlobalFunc(NSSYM(IsBinary),		NsIsBinary,			1, "IsBinary(obj)");
    NewtDefGlobalFunc(NSSYM(IsSymbol),		NsIsSymbol,			1, "IsSymbol(obj)");
    NewtDefGlobalFunc(NSSYM(IsString),		NsIsString,			1, "IsString(obj)");
    NewtDefGlobalFunc(NSSYM(IsCharacter),	NsIsCharacter,		1, "IsCharacter(obj)");
    NewtDefGlobalFunc(NSSYM(IsInteger),		NsIsInteger,		1, "IsInteger(obj)");
    NewtDefGlobalFunc(NSSYM(IsReal),		NsIsReal,			1, "IsReal(obj)");
    NewtDefGlobalFunc(NSSYM(IsNumber),		NsIsNumber,			1, "IsNumber(obj)");
    NewtDefGlobalFunc(NSSYM(IsImmediate),	NsIsImmediate,		1, "IsImmediate(obj)");
    NewtDefGlobalFunc(NSSYM(IsFunction),	NsIsFunction,		1, "IsFunction(obj)");
    NewtDefGlobalFunc(NSSYM(IsReadonly),	NsIsReadonly,		1, "IsReadonly(obj)");

    NewtDefGlobalFunc(NSSYM(Intern),		NsMakeSymbol,		1, "Intern(str)");
    NewtDefGlobalFunc(NSSYM(MakeBinary),	NsMakeBinary,		2, "MakeBinary(length, class)");
    NewtDefGlobalFunc(NSSYM(SetLength),		NsSetLength,		2, "SetLength(obj, len)");

    NewtDefGlobalFunc(NSSYM(HasSlot),		NsHasSlot,			2, "HasSlot(frame, slot)");
    NewtDefGlobalFunc(NSSYM(GetSlot),		NsGetSlot,			2, "GetSlot(frame, slot)");
    NewtDefGlobalFunc(NSSYM(SetSlot),		NsSetSlot,			3, "SetSlot(frame, slot, v)");
    NewtDefGlobalFunc(NSSYM(GetVariable),	NsGetVariable,		2, "GetVariable(frame, slot)");
    NewtDefGlobalFunc(NSSYM(SetVariable),	NsSetVariable,		3, "SetVariable(frame, slot, v)");

    NewtDefGlobalFunc(NSSYM(GetRoot),		NsGetRoot,			0, "GetRoot()");
    NewtDefGlobalFunc(NSSYM(GetGlobals),	NsGetGlobals,		0, "GetGlobals()");
    NewtDefGlobalFunc(NSSYM(GC),			NsGC,				0, "GC()");
    NewtDefGlobalFunc(NSSYM(Compile),		NsCompile,			1, "Compile(str)");

    NewtDefGlobalFunc(NSSYM(GetGlobalFn),	NsGetGlobalFn,		1, "GetGlobalFn(symbol)");
	NewtDefGlobalFunc(NSSYM(GetGlobalVar),	NsGetGlobalVar,		1, "GetGlobalVar(symbol)");
    NewtDefGlobalFunc(NSSYM(GlobalFnExists),NsGlobalFnExists,	1, "GlobalFnExists(symbol)");
    NewtDefGlobalFunc(NSSYM(GlobalVarExists),NsGlobalVarExists,	1, "GlobalVarExists(symbol)");
    NewtDefGlobalFunc(NSSYM(UndefGlobalFn),	NsUndefGlobalFn,	1, "UndefGlobalFn(symbol)");
    NewtDefGlobalFunc(NSSYM(UndefGlobalVar),NsUndefGlobalVar,	1, "UndefGlobalVar(symbol)");

    NewtDefGlobalFunc(NSSYM(Apply),			NsApply,			2, "Apply(func, params)");
    NewtDefGlobalFunc(NSSYM(Perform),		NsPerform,			3, "Perform(frame, message, params)");
    NewtDefGlobalFunc(NSSYM(PerformIfDefined),NsPerformIfDefined,3, "PerformIfDefined(frame, message, params)");
    NewtDefGlobalFunc(NSSYM(ProtoPerform),	NsProtoPerform,		3, "ProtoPerform(frame, message, params)");
    NewtDefGlobalFunc(NSSYM(ProtoPerformIfDefined),NsProtoPerformIfDefined,3, "ProtoPerformIfDefined(frame, message, params)");

    NewtDefGlobalFunc(NSSYM(Chr),			NsChr,				1, "Chr(integer)");
    NewtDefGlobalFunc(NSSYM(Ord),			NsOrd,				1, "Ord(char)");
    NewtDefGlobalFunc(NSSYM(StrLen),		NsStrLen,			1, "StrLen(str)");
    NewtDefGlobalFunc(NSSYM(SubStr),		NsSubStr,			3, "SubStr(str, start, count)");
    NewtDefGlobalFunc(NSSYM(StrEqual),		NsStrEqual,			2, "StrEqual(a, b)");
    NewtDefGlobalFunc(NSSYM(StrExactCompare),NsStrExactCompare,	2, "StrExactCompare(a, b)");
    NewtDefGlobalFunc(NSSYM(BeginsWith),	NsBeginsWith,		2, "BeginsWith(str, sub)");
    NewtDefGlobalFunc(NSSYM(EndsWith),		NsEndsWith,			2, "EndsWith(str, sub)");
    NewtDefGlobalFunc(NSSYM(SPrintObject),	NsSPrintObject,		1, "SPrintObject(obj)");
    NewtDefGlobalFunc(NSSYM(SymbolCompareLex),	NsSymbolCompareLex,	2, "SymbolCompareLex(symbol1, symbol2)");
  
  NewtDefGlobalFunc(NSSYM(Array),	NsMakeArray,	2, "Array(size, initialValue)");
  NewtDefGlobalFunc(NSSYM(SetContains),	NsSetContains,	2, "SetContains( array, item )");
}


/*------------------------------------------------------------------------*/
/** 追加グローバル関数の初期化
 *
 * @return			なし
 */

void NVMInitExGlobalFns(void)
{
    NewtDefGlobalFunc(NSSYM(P),			NsPrintObject,		1, "P(obj)");
    NewtDefGlobalFunc(NSSYM(Print),		NsPrint,			1, "Print(obj)");

    NewtDefGlobalFunc(NSSYM(CompileFile),NsCompileFile,		1, "CompileFile(file)");

#ifdef HAVE_DLOPEN
    NewtDefGlobalFunc(NSSYM(LoadLib),	NsLoadLib,			1, "LoadLib(file)");
#endif /* HAVE_DLOPEN */

    NewtDefGlobalFunc(NSSYM(Load),		NsLoad,				1, "Load(file)");
	NewtDefGlobalFunc(NSSYM(Require),	NsRequire,			1, "Require(str)");

    NewtDefGlobalFunc(NSSYM(LoadBinary),		NsLoadBinary,		1, "LoadBinary(filename)");
    NewtDefGlobalFunc(NSSYM(SaveBinary),		NsSaveBinary,		2, "SaveBinary(data, filename)");
    NewtDefGlobalFunc(NSSYM(MakeBinaryFromHex),		NsMakeBinaryFromHex,	2, "MakeBinaryFromHex(hexString, class)");

	NewtDefGlobalFunc(NSSYM(MakeNSOF),	NsMakeNSOF,			2, "MakeNSOF(obj, ver)");
	NewtDefGlobalFunc(NSSYM(ReadNSOF),	NsReadNSOF,			1, "ReadNSOF(nsof)");

	NewtDefGlobalFunc(NSSYM(MakePkg),	NsMakePkg,			1, "MakePkg(obj)");
	NewtDefGlobalFunc(NSSYM(ReadPkg),	NsReadPkg,			1, "ReadPkg(pkg)");

    NewtDefGlobalFunc(NSSYM(GetEnv),	NsGetEnv,			1, "GetEnv(str)");

    NewtDefGlobalFunc(NSSYM(FileExists),NsFileExists,		1, "FileExists(path)");

    NewtDefGlobalFunc(NSSYM(DirName),	NsDirName,			1, "DirName(path)");
    NewtDefGlobalFunc(NSSYM(BaseName),	NsBaseName,			1, "BaseName(path)");
    NewtDefGlobalFunc(NSSYM(JoinPath),	NsJoinPath,			2, "JoinPath(dir, fname)");
    NewtDefGlobalFunc(NSSYM(ExpandPath),NsExpandPath,		1, "ExpandPath(path)");

    NewtDefGlobalFunc(NSSYM(Split),		NsSplit,			2, "Split(str, sep)");
  NewtDefGlobalFunc(NSSYM(StrPos),		NsStrPos,			3, "StrPos(hastack, needle, position)");
  NewtDefGlobalFunc(NSSYM(StrReplace),		NsStrReplace,			4, "StrReplace(string, substr, replacement, count)");
    NewtDefGlobalFunc(NSSYM(ParamStr),	NsParamStr,			2, "ParamStr(baseString, paramStrArray)");
    NewtDefGlobalFunc(NSSYM(StrCat),	NsStrCat,			2, "StrCat(str1, str2)");

    NewtDefGlobalFunc(NSSYM(ExtractByte),NsExtractByte,		2, "ExtractByte(data, offset)");
  NewtDefGlobalFunc(NSSYM(ExtractWord),NsExtractWord,		2, "ExtractWord(data, offset)");

    NewtDefGlobalFunc(NSSYM(Gets),		NsGets,				0, "Gets()");
    NewtDefGlobalFunc(NSSYM(Getc),		NsGetc,				0, "Getc()");
    NewtDefGlobalFunc(NSSYM(Getch),		NsGetch,			0, "Getch()");
}


/*------------------------------------------------------------------------*/
/**　デバッグ用グローバル関数の初期化
 *
 * @return			なし
 */

void NVMInitDebugGlobalFns(void)
{
    NewtDefGlobalFunc(NSSYM(DumpFn),		NsDumpFn,			1, "DumpFn(fn)");
    NewtDefGlobalFunc(NSSYM(DumpBC),		NsDumpBC,			1, "DumpBC(instructions)");
    NewtDefGlobalFunc(NSSYM(DumpStacks),	NsDumpStacks,		0, "DumpStacks()");
}


/*------------------------------------------------------------------------*/
/**　グローバル関数の初期化
 *
 * @return			なし
 */

void NVMInitGlobalFns(void)
{
    NVMInitGlobalFns0();		// 必須
    NVMInitGlobalFns1();		// 組込み関数
    NVMInitExGlobalFns();		// 追加
    NVMInitDebugGlobalFns();	// デバッグ用
}


/*------------------------------------------------------------------------*/
/**　グローバル変数の初期化
 *
 * @return			なし
 */

void NVMInitGlobalVars(void)
{
    NcDefGlobalVar(NSSYM0(printDepth), NSINT(3));
    NcDefGlobalVar(NSSYM0(printIndent), NSINT(1));
    NcDefGlobalVar(NSSYM0(printLength), NSINT(10));
    NcDefGlobalVar(NSSYM0(printBinaries), NSINT(0));

    NcDefGlobalVar(NSSYM0(_STDOUT_), kNewtRefNIL);
    NcDefGlobalVar(NSSYM0(_STDERR_), kNewtRefNIL);
}


/*------------------------------------------------------------------------*/
/**　VM の初期化
 *
 * @return			なし
 */

void NVMInit(void)
{
	newtRefVar  result;

	vm_env.level = 0;

    NVMInitREG();
    NVMInitSTACK();
    NVMInitGlobalFns();
    NVMInitGlobalVars();

	// 標準ライブラリのロード
	result = NcRequire0(NSSTR("egg"));
	NVMClearCurrException();

	if (result != kNewtRefUnbind)
	{
		stk_pop();
	}
}


/*------------------------------------------------------------------------*/
/**　VM の後始末
 *
 * @return			なし
 */

void NVMClean(void)
{
    NVMCleanSTACK();
}


/*------------------------------------------------------------------------*/
/**　VMループ
 *
 * @param callsp	[in] 呼出しスタックのスタックポインタ
 *
 * @return			なし
 */

void NVMLoop(uint32_t callsp)
{
    uint16_t	oplen;
    int16_t	b;
    uint8_t	op;
    uint8_t	a;

	vm_env.level++;

	if (NEWT_DEBUG)
		NewtDebugMsg("VM", "VM Level = %d\n", vm_env.level);

    while (callsp < CALLSP && PC < BCLEN)
    {
        op = BC[PC];

        b = op & kNBCFieldMask;
        a = (op & 0xff)>>3;

        if (b == kNBCFieldMask)
        {
            oplen = 3;

            b = (int16_t)BC[PC + 1] << 8;
            b += BC[PC + 2];
    
            if (a == 0)
            {
                if (b == 0x01)
                    b = kNBCFieldMask;
            }
        }
        else
        {
            oplen = 1;
        }
    
        if (NEWT_TRACE)
        {
            NVMDumpInstCode(stderr, BC, PC, oplen);
            NVMDumpInstName(stderr, a, b);
        }

        PC += oplen;
    
        if (a < kNBCInstructionsLen)
            (is_instructions[a])(b);

        if (NEWT_TRACE)
            NVMDumpInstResult(stderr);

        if (NEWT_NEEDGC)
            NewtGC();
    }

	vm_env.level--;
}


/*------------------------------------------------------------------------*/
/** 呼出す関数オブジェクトをセット
 *
 * @param fn		[in] 関数オブジェクト
 * @param numArgs	[in] 引数の数
 *
 * @return			なし
 */

void NVMFnCall(newtRefArg fn, int16_t numArgs)
{
    stk_push(fn);  
    si_set_lex_scope();
    is_invoke(numArgs);
}


/*------------------------------------------------------------------------*/
/** 関数オブジェクトを実行
 *
 * @param fn		[in] 関数オブジェクト
 * @param numArgs	[in] 引数の数
 * @param errP		[out]エラーコード
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NVMCall(newtRefArg fn, int16_t numArgs, newtErr * errP)
{
	newtRefVar	result = kNewtRefUnbind;
	newtErr		err = kNErrNone;

    if (NewtRefIsNotNIL(fn))
    {
		vm_env_t	saveVM;

		// save the VM
		vm_env_push(&saveVM);

        NVMFnCall(fn, numArgs);
        NVMLoop(CALLSP - 1);
        result = stk_top();

		// restore the VM
		vm_env_pop();
    }

    if (errP != NULL)
	{
		if (err == kNErrNone)
			err = NVMGetExceptionErrCode(CURREXCP, true);

        *errP = err;
	}

    return result;
}


/*------------------------------------------------------------------------*/
/** 関数オブジェクトを実行
 *
 * @param fn		[in] 関数オブジェクト
 * @param errP		[out]エラーコード
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NVMInterpret(newtRefArg fn, newtErr * errP)
{
	newtRefVar	result = kNewtRefUnbind;
	newtErr		err = kNErrNone;

    if (NewtRefIsNotNIL(fn))
    {
        if (NEWT_TRACE)
            NewtFprintf(stderr, "*** trace ***\n");

		NVMInit();
		NewtGC();

        result = NVMCall(fn, 0, &err);

        NVMClean();
    }

    if (errP != NULL)
        *errP = err;

    return result;
}


/*------------------------------------------------------------------------*/
/** 構文木をバイトコードに変換して実行
 *
 * @param stree		[in] 構文木
 * @param numStree  [in] 構文木の長さ
 * @param errP		[out]エラーコード
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NVMInterpret2(nps_syntax_node_t * stree, uint32_t numStree, newtErr * errP)
{
    newtRefVar	result = kNewtRefUnbind;

    if (errP != NULL)
        *errP = kNErrNone;

    if (stree != NULL)
    {
        newtRefVar	fn = kNewtRefNIL;

        NBCInit();
        fn = NBCGenBC(stree, numStree, true);
        NBCCleanup();
        NPSCleanup();

        result = NVMInterpret(fn, errP);
    }

    return result;
}


/*------------------------------------------------------------------------*/
/** 指定された関数の情報を表示
 *
 * @param name		[in] 関数名
 *
 * @return			エラーコード
 */

newtErr NVMInfo(const char * name)
{
    newtErr	err = kNErrNone;

    NVMInit();

    if (name != NULL)
        NewtInfo(NewtMakeSymbol(name));
    else
        NewtInfoGlobalFns();

    NVMClean();

    return err;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** ファイルを読込んでスクリプトを実行
 *
 * @param path		[in] ファイルのパス
 * @param errP		[out]エラーコード
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NVMInterpretFile(const char * path, newtErr * errP)
{
    nps_syntax_node_t *	stree;
    uint32_t	numStree;
    newtRefVar	result = kNewtRefUnbind;
    newtErr	err;

    err = NPSParseFile(path, &stree, &numStree);

    if (err == kNErrNone)
        result = NVMInterpret2(stree, numStree, &err);

    if (errP != NULL)
        *errP = err;

    return result;
}


/*------------------------------------------------------------------------*/
/** 文字列をコンパイルしてスクリプトを実行
 *
 * @param s			[in] 文字列
 * @param errP		[out]エラーコード
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NVMInterpretStr(const char * s, newtErr * errP)
{
    nps_syntax_node_t *	stree;
    uint32_t	numStree;
    newtRefVar	result = kNewtRefUnbind;
    newtErr	err;

    err = NPSParseStr(s, &stree, &numStree);

    if (err == kNErrNone)
        result = NVMInterpret2(stree, numStree, &err);

    if (errP != NULL)
        *errP = err;

    return result;
}


/**
 * Execute a function as if it was a method of an object.
 *
 * @param inImpl		implementor.
 * @param inRcvr		object.
 * @param inFunction	function to execute.
 * @param inArgs		array of arguments.
 * @return the result of the call.
 */
newtRef
NVMMessageSendWithArgArray(
	newtRefArg inImpl,
	newtRefArg inRcvr,
	newtRefArg inFunction,
	newtRefArg inArgs)
{
	newtRefVar	result;
	vm_env_t	saveVM;
	int16_t		nbArgs;
	int			indexArgs;

	/* save the VM */
	vm_env_push(&saveVM);

	nbArgs = NewtArrayLength(inArgs);
	/* Push the arguments on the stack */
	for (indexArgs = 0; indexArgs < nbArgs; indexArgs++)
	{
		stk_push(NewtGetArraySlot(inArgs, indexArgs));
	}
	
	/* Send the message */
	NVMMessageSend(inImpl, inRcvr, inFunction, nbArgs);
	NVMLoop(CALLSP - 1);
	result = stk_top();

	/* restore the VM */
	vm_env_pop();

	return result;
}


/*------------------------------------------------------------------------*/
/** 関数オブジェクトを va_list で実行
 *
 * @param fn		[in] 関数オブジェクト
 * @param argc		[in] 引数の数
 * @param ap		[in] va_list
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NVMFuncCallWithValist(newtRefArg fn, int argc, va_list ap)
{
	vm_env_t	saveVM;
	newtRefVar	result;

	// save the VM
	vm_env_push(&saveVM);

	// Push the arguments on the stack
	stk_push_varg(argc, ap);

	// Call function
	NVMFuncCall(fn, argc);
	NVMLoop(CALLSP - 1);
	result = stk_top();

	// restore the VM
	vm_env_pop();

	return result;
}


/*------------------------------------------------------------------------*/
/** メソッドを va_list で実行
 *
 * @param impl		[in] インプリメンタ
 * @param receiver	[in] レシーバー
 * @param fn		[in] 関数オブジェクト
 * @param argc		[in] 引数の数
 * @param ap		[in] va_list
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NVMMessageSendWithValist(newtRefArg impl, newtRefArg receiver, newtRefArg fn, int argc, va_list ap)
{
	newtRefVar	result;
	vm_env_t	saveVM;

	// save the VM
	vm_env_push(&saveVM);
	
	// Push the arguments on the stack
	stk_push_varg(argc, ap);

	// Send the message
	NVMMessageSend(impl, receiver, fn, argc);
	NVMLoop(CALLSP - 1);
	result = stk_top();

	/* restore the VM */
	vm_env_pop();

	return result;
}


/*------------------------------------------------------------------------*/
/** 関数オブジェクトを可変引数で実行
 *
 * @param fn		[in] 関数オブジェクト
 * @param argc		[in] 引数の数
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NcCall(newtRefArg fn, int argc, ...)
{
	newtRefVar	result;
	va_list		ap;

	va_start(ap, argc);
	result = NVMFuncCallWithValist(fn, argc, ap);
	va_end(ap);

	return result;
}


/*------------------------------------------------------------------------*/
/** 配列を引数にして関数オブジェクトを実行
 *
 * @param fn		[in] 関数オブジェクト
 * @param args		[in] 配列
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NcCallWithArgArray(newtRefArg fn, newtRefArg args)
{
	vm_env_t	saveVM;
	newtRefVar	result;
	newtErr		err;

	if (! NewtRefIsArray(args))
	{
		return NewtThrow(kNErrNotAnArray, args);
	}

	// save the VM
	vm_env_push(&saveVM);

	stk_push_array(args);
	result = NVMCall(fn, NewtArrayLength(args), &err);

	// restore the VM
	vm_env_pop();

	return result;
}


/*------------------------------------------------------------------------*/
/** グローバル関数を可変引数で実行
 *
 * @param sym		[in] シンボル
 * @param argc		[in] 引数の数
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NcCallGlobalFn(newtRefArg sym, int argc, ...)
{
	newtRefVar	result;
    newtRefVar	fn;
	va_list		ap;

    fn = NcGetGlobalFn(sym);
    if (NewtRefIsNIL(fn))
	{
		return NewtThrow(kNErrUndefinedGlobalFunction, sym);
	}

	va_start(ap, argc);
	result = NVMFuncCallWithValist(fn, argc, ap);
	va_end(ap);

	return result;
}


/*------------------------------------------------------------------------*/
/** 配列を引数にしてグローバル関数を実行
 *
 * @param sym		[in] シンボル
 * @param args		[in] 配列
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NcCallGlobalFnWithArgArray(newtRefArg sym, newtRefArg args)
{
    newtRefVar	fn;

    fn = NcGetGlobalFn(sym);
    if (NewtRefIsNIL(fn))
	{
		return NewtThrow(kNErrUndefinedGlobalFunction, sym);
	}

	return NcCallWithArgArray(fn, args);
}


/*------------------------------------------------------------------------*/
/** メソッドを可変引数で実行
 *
 * @param receiver	[in] レシーバー
 * @param sym		[in] シンボル
 * @param ignore	[in] メソッド未定定義の例外を無視する
 * @param argc		[in] 引数の数
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NcSend(newtRefArg receiver, newtRefArg sym, bool ignore, int argc, ...)
{
	newtRefVar	result = kNewtRefUnbind;
    newtRefVar	impl;
    newtRefVar	fn;

	if (! NewtRefIsSymbol(sym))
	{
		return NewtThrow(kNErrNotASymbol, sym);
	}

	if (! NewtRefIsFrame(receiver) && ! NewtRefIsNIL(receiver))
	{
		return NewtThrow(kNErrNotAFrame, receiver);
	}

    impl = NcFullLookupFrame(receiver, sym);

    if (impl != kNewtRefUnbind)
	{
		va_list		ap;

		fn = NcGetSlot(impl, sym);

		va_start(ap, argc);
		result = NVMMessageSendWithValist(impl, receiver, fn, argc, ap);
		va_end(ap);
    }
	else
	{
		if (! ignore)
			return NewtThrow(kNErrUndefinedMethod, sym);
	}

	return result;
}


/*------------------------------------------------------------------------*/
/** 配列を引数にしてメソッドを実行
 *
 * @param receiver	[in] レシーバー
 * @param sym		[in] シンボル
 * @param ignore	[in] メソッド未定定義の例外を無視する
 * @param args		[in] 配列
 *
 * @return			スタックのトップオブジェクト
 */

newtRef NcSendWithArgArray(newtRefArg receiver, newtRefArg sym, bool ignore, newtRefArg args)
{
	newtRefVar	result = kNewtRefUnbind;
    newtRefVar	impl;
    newtRefVar	fn;

	if (! NewtRefIsSymbol(sym))
	{
		return NewtThrow(kNErrNotASymbol, sym);
	}

	if (! NewtRefIsFrame(receiver) && ! NewtRefIsNIL(receiver))
	{
		return NewtThrow(kNErrNotAFrame, receiver);
	}

    impl = NcFullLookupFrame(receiver, sym);

    if (impl != kNewtRefUnbind)
	{
		fn = NcGetSlot(impl, sym);
		result = NVMMessageSendWithArgArray(impl, receiver, fn, args);
    }
	else
	{
		if (! ignore)
			return NewtThrow(kNErrUndefinedMethod, sym);
	}

	return result;
}


/*------------------------------------------------------------------------*/
/** メソッドを可変引数で実行（プロト呼出し）
 *
 * @param receiver	[in] レシーバー
 * @param sym		[in] シンボル
 * @param ignore	[in] メソッド未定定義の例外を無視する
 * @param argc		[in] 引数の数
 *
 * @return			スタックのトップオブジェクト
 *
 * @note			プロト継承のみ行う
 */

newtRef NcSendProto(newtRefArg receiver, newtRefArg sym, bool ignore, int argc, ...)
{
	newtRefVar	result = kNewtRefUnbind;
    newtRefVar	impl;
    newtRefVar	fn;

	if (! NewtRefIsSymbol(sym))
	{
		return NewtThrow(kNErrNotASymbol, sym);
	}

	if (! NewtRefIsFrame(receiver) && ! NewtRefIsNIL(receiver))
	{
		return NewtThrow(kNErrNotAFrame, receiver);
	}

	impl = NcProtoLookupFrame(receiver, sym);

    if (impl != kNewtRefUnbind)
	{
		va_list		ap;

		fn = NcGetSlot(impl, sym);

		va_start(ap, argc);
		result = NVMMessageSendWithValist(impl, receiver, fn, argc, ap);
		va_end(ap);
    }
	else
	{
		if (! ignore)
			return NewtThrow(kNErrUndefinedMethod, sym);
	}

	return result;
}


/*------------------------------------------------------------------------*/
/** 配列を引数にしてメソッドを実行（プロト呼出し）
 *
 * @param receiver	[in] レシーバー
 * @param sym		[in] シンボル
 * @param ignore	[in] メソッド未定定義の例外を無視する
 * @param args		[in] 配列
 *
 * @return			スタックのトップオブジェクト
 *
 * @note			プロト継承のみ行う
 */

newtRef NcSendProtoWithArgArray(newtRefArg receiver, newtRefArg sym, bool ignore, newtRefArg args)
{
	newtRefVar	result = kNewtRefUnbind;
    newtRefVar	impl;
    newtRefVar	fn;

	if (! NewtRefIsSymbol(sym))
	{
		return NewtThrow(kNErrNotASymbol, sym);
	}

	if (! NewtRefIsFrame(receiver) && ! NewtRefIsNIL(receiver))
	{
		return NewtThrow(kNErrNotAFrame, receiver);
	}

	impl = NcProtoLookupFrame(receiver, sym);

    if (impl != kNewtRefUnbind)
	{
		fn = NcGetSlot(impl, sym);
		result = NVMMessageSendWithArgArray(impl, receiver, fn, args);
    }
	else
	{
		if (! ignore)
			return NewtThrow(kNErrUndefinedMethod, sym);
	}

	return result;
}
