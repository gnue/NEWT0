//--------------------------------------------------------------------------
/**
 * @file	NewtParser.c
 * @brief   構文木の生成
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <stdlib.h>
#include <string.h>

#include "NewtParser.h"
#include "NewtErrs.h"
#include "NewtMem.h"
#include "NewtObj.h"
#include "NewtEnv.h"
#include "NewtFns.h"
#include "NewtVM.h"
#include "NewtIO.h"
#include "NewtPrint.h"

#include "yacc.h"


/* 型宣言 */

/// 入力データ
typedef struct {
	const char *	data;		///< 入力データ
	const char *	currp;		///< 現在の入力位置
	const char *	limit;		///< 入力データの最後
} nps_inputdata_t;


/* グローバル変数 */
nps_env_t				nps_env;		///< パーサ環境


/* ローカル変数 */
static newtStack		nps_stree;		///< 構文木スタック情報
static nps_inputdata_t  nps_inputdata;  ///< 入力データ


/* マクロ */
#define STREESTACK		((nps_syntax_node_t *)nps_stree.stackp)		///< 構文木スタック
#define CX				(nps_stree.sp)								///< 構文木スタックポインタ


/* 関数プロトタイプ */
static void		NPSBindParserInput(const char * s);
static int		nps_yyinput_str(char * buff, int max_size);

static void		NPSInit(newtPool pool);

static void		NPSPrintNode(FILE * f, nps_node_t r);
static void		NPSPrintSyntaxCode(FILE * f, uint32_t code);


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 構文解析する文字列をセットする
 *
 * @param s			[in] 文字列
 *
 * @return			なし
 */

void NPSBindParserInput(const char * s)
{
    nps_inputdata.data = nps_inputdata.currp = s;

    if (s != NULL)
        nps_inputdata.limit = s + strlen(s);
    else
        nps_inputdata.limit = NULL;
}


/*------------------------------------------------------------------------*/
/** 構文解析するデータを文字列から取出す
 *
 * @param buff		[out]バッファ
 * @param max_size	[in] 最大長
 *
 * @return			取出したデータサイズ
 */

int nps_yyinput_str(char * buff, int max_size)
{
    int	n;

    n = nps_inputdata.limit - nps_inputdata.currp;
    if (max_size < n) n = max_size;

    if (0 < n)
    {
        memcpy(buff, nps_inputdata.currp, n);
        nps_inputdata.currp += n;
    }

    return n;
}


/*------------------------------------------------------------------------*/
/** 構文解析するデータを取出す
 *
 * @param yyin		[in] 入力ファイル
 * @param buff		[out]バッファ
 * @param max_size	[in] 最大長
 *
 * @return			取出したデータサイズ
 */

int nps_yyinput(FILE * yyin, char * buff, int max_size)
{
    if (nps_inputdata.data != NULL)
        return nps_yyinput_str(buff, max_size);
    else
        return fread(buff, 1, max_size, yyin);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 構文解析のための初期化
 *
 * @param pool		[in] メモリプール
 *
 * @return			なし
 */

void NPSInit(newtPool pool)
{
	nps_yyinit();

	nps_env.numwarns = 0;
	nps_env.numerrs = 0;
	nps_env.fname = NULL;
	nps_env.lineno = 1;
	nps_env.tokenpos = 0;
	nps_env.first_time = true;
  memset(nps_env.linebuf, 0x00, sizeof(nps_env.linebuf) - 1);

    NewtStackSetup(&nps_stree, NEWT_POOL, sizeof(nps_syntax_node_t), NEWT_NUM_STREESTACK);
}


/*------------------------------------------------------------------------*/
/** 構文解析する
 *
 * @param path		[in] 入力ファイルのパス
 * @param streeP	[out]構文木
 * @param sizeP		[out]構文木のサイズ
 * @param is_file	[in] ファイルかどうか（#! 処理をおこなうかどうか）
 *
 * @return			エラーコード
 */

newtErr NPSParse(const char * path, nps_syntax_node_t ** streeP, uint32_t * sizeP, bool is_file)
{
    newtErr	err = kNErrNone;

    NPSInit(NULL);
	nps_env.fname = path;
	nps_env.first_time = is_file;

    if (yyparse() != 0 || 0 < nps_env.numerrs)
        err = kNErrSyntaxError;

	nps_yycleanup();

    if (NEWT_DUMPSYNTAX)
    {
        NewtFprintf(stderr, "*** syntax tree ***\n");
        NPSDumpSyntaxTree(stderr, STREESTACK, CX);
        NewtFprintf(stderr, "\n");
    }

    if (err == kNErrNone && 0 < CX)
        NewtStackSlim(&nps_stree, CX);
    else
        NPSCleanup();

    *streeP = STREESTACK;
    *sizeP = CX;

    return err;
}


/*------------------------------------------------------------------------*/
/** 指定されたファイルをソースに構文解析する
 *
 * @param path		[in] 入力ファイルのパス
 * @param streeP	[out]構文木
 * @param sizeP		[out]構文木のサイズ
 *
 * @return			エラーコード
 */

newtErr NPSParseFile(const char * path,
        nps_syntax_node_t ** streeP, uint32_t * sizeP)
{
	newtErr err;

    if (path != NULL)
    {
        yyin = fopen(path, "r");

        if (yyin == NULL)
        {
            NewtFprintf(stderr, "not open file.\n");
            return kNErrFileNotOpen;
        }

		err = NPSParse(path, streeP, sizeP, true);
		fclose(yyin);
    }
	else
	{
		err = NPSParse(path, streeP, sizeP, true);
	}

	return err;
}


/*------------------------------------------------------------------------*/
/** 文字列をソースに構文解析する
 *
 * @param s			[in] 入力データ
 * @param streeP	[out]構文木
 * @param sizeP		[out]構文木のサイズ
 *
 * @return			エラーコード
 */

newtErr NPSParseStr(const char * s,
        nps_syntax_node_t ** streeP, uint32_t * sizeP)
{
    newtErr	err;

    NPSBindParserInput(s);
    err = NPSParse(NULL, streeP, sizeP, false);
    NPSBindParserInput(NULL);

    return err;
}


/*------------------------------------------------------------------------*/
/** 構文解析の後始末
 *
 * @return			なし
 */

void NPSCleanup(void)
{
    NewtStackFree(&nps_stree);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 構文木のノードを印字する
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] ノード
 *
 * @return			なし
 */

void NPSPrintNode(FILE * f, nps_node_t r)
{
    if (NPSRefIsSyntaxNode(r))
        NewtFprintf(f, "*%d", (int)NPSRefToSyntaxNode(r));
    else
        NewtPrintObj(f, (newtRef)r);
}


/*------------------------------------------------------------------------*/
/** 構文コードを印字する
 *
 * @param f			[in] 出力ファイル
 * @param code		[in] 構文コード
 *
 * @return			なし
 */

void NPSPrintSyntaxCode(FILE * f, uint32_t code)
{
    char *	s = "Unknown";

    switch (code)
    {
        case kNPSConstituentList:
            s = ";";
            break;

        case kNPSCommaList:
            s = ",";
            break;

        case kNPSConstant:
            s = "constant";
            break;

        case kNPSGlobal:
            s = "global";
            break;

        case kNPSLocal:
            s = "local";
            break;

        case kNPSGlobalFn:
            s = "global func";
            break;

        case kNPSFunc:
            s = "func";
            break;

        case kNPSArg:
            s = "arg";
            break;

        case kNPSMessage:
            s = "message";
            break;

        case kNPSLvalue:
            s = "lvalue";
            break;

        case kNPSAssign:
            s = ":=";
            break;

        case kNPSExists:
            s = "exists";
            break;

        case kNPSMethodExists:
            s = "method exists";
            break;

        case kNPSTry:
            s = "try";
            break;

        case kNPSOnexception:
            s = "onexception";
            break;

        case kNPSOnexceptionList:
            s = "onexception list";
            break;

        case kNPSIf:
            s = "if";
            break;

        case kNPSLoop:
            s = "loop";
            break;

        case kNPSFor:
            s = "for";
            break;

        case kNPSForeach:
            s = "foreach";
            break;

        case kNPSWhile:
            s = "while";
            break;

        case kNPSRepeat:
            s = "repeat";
            break;

        case kNPSBreak:
            s = "break";
            break;

        case kNPSSlot:
            s = "slot";
            break;
    }

    NewtFprintf(f, "%24s\t", s);
}


/*------------------------------------------------------------------------*/
/** 構文木をダンプする
 *
 * @param f			[in] 出力ファイル
 * @param stree		[in] 構文木
 * @param size		[in] 構文木のサイズ
 *
 * @return			なし
 */

void NPSDumpSyntaxTree(FILE * f, nps_syntax_node_t * stree, uint32_t size)
{
    nps_syntax_node_t *	node;
    uint32_t	i;

    for (i = 0, node = stree; i < size; i++, node++)
    {
        NewtFprintf(f, "%d\t", (int)i);

        if (kNPSConstituentList <= node->code)
        {
            NPSPrintSyntaxCode(f, node->code);
        }
        else
        {
            if (node->code <= kNPSPopHandlers)
                NVMDumpInstName(f, 0, node->code);
            else if (kNPSAdd <= node->code)
                NVMDumpInstName(f, kNPSFreqFunc >> 3, node->code - kNPSAdd);
            else
                NVMDumpInstName(f, node->code >> 3, 0);
        }

        NewtFprintf(f, "\t");
        NPSPrintNode(f, node->op1);
        NewtFprintf(f, "\t");
        NPSPrintNode(f, node->op2);
        NewtFprintf(f, "\n");
    }
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 引数０のノードを作成
 *
 * @param code		[in] 構文コード
 *
 * @return			ノード
 */

nps_node_t NPSGenNode0(uint32_t code)
{
    return NPSGenNode2(code, kNewtRefUnbind, kNewtRefUnbind);
}


/*------------------------------------------------------------------------*/
/** 引数１のノードを作成
 *
 * @param code		[in] 構文コード
 * @param op1		[in] 引数１
 *
 * @return			ノード
 */

nps_node_t NPSGenNode1(uint32_t code, nps_node_t op1)
{
    return NPSGenNode2(code, op1, kNewtRefUnbind);
}


/*------------------------------------------------------------------------*/
/** 引数２のノードを作成
 *
 * @param code		[in] 構文コード
 * @param op1		[in] 引数１
 * @param op2		[in] 引数２
 *
 * @return			ノード
 */

nps_node_t NPSGenNode2(uint32_t code, nps_node_t op1, nps_node_t op2)
{
    nps_syntax_node_t *	node;

    if (! NewtStackExpand(&nps_stree, CX + 1))
    {
        yyerror("Syntax tree overflow");
        return -1;
    }

    node = &STREESTACK[CX];

    node->code = code;
    node->op1 = op1;
    node->op2 = op2;

    CX++;

    return NPSMakeSyntaxNode(CX - 1);
}


/*------------------------------------------------------------------------*/
/** 引数１のオペレータノードを作成
 *
 * @param op		[in] オペコード
 * @param op1		[in] 引数１
 *
 * @return			ノード
 */

nps_node_t NPSGenOP1(uint32_t op, nps_node_t op1)
{
    uint32_t	b = KNPSUnknownCode;

    switch (op)
    {
        case kNPS_NOT:
            b = kNPSNot;
            break;

        default:
            NPSError(kNErrSyntaxError);
            return kNewtRefUnbind;
    }

    return NPSGenNode1(b, op1);
}


/*------------------------------------------------------------------------*/
/** 引数２のオペレータノードを作成
 *
 * @param op		[in] オペコード
 * @param op1		[in] 引数１
 * @param op2		[in] 引数２
 *
 * @return			ノード
 */

nps_node_t NPSGenOP2(uint32_t op, nps_node_t op1, nps_node_t op2)
{
    uint32_t	b = KNPSUnknownCode;

    switch (op)
    {
        case '+':
            b = kNPSAdd;
            break;

        case '-':
            b = kNPSSubtract;
            break;

        case '=':
            b = kNPSEquals;
            break;

        case kNPS_NOT_EQUAL:
            b = kNPSNotEqual;
            break;

        case kNPS_OBJECT_EQUAL:
            b = kNPSObjectEqual;
            break;

        case '<':
            b = kNPSLessThan;
            break;

        case '>':
            b = kNPSGreaterThan;
            break;

        case kNPS_GREATER_EQUAL:
            b = kNPSGreaterOrEqual;
            break;

        case kNPS_LESS_EQUAL:
            b = kNPSLessOrEqual;
            break;

        case '*':
            b = kNPSMultiply;
            break;

        case '/':
            b = kNPSDivide;
            break;

        case kNPS_DIV:
            b = kNPSDiv;
            break;

        case kNPS_MOD:
            b = kNPSMod;
            break;

		case kNPS_SHIFT_LEFT:
            b = kNPSShiftLeft;
            break;

		case kNPS_SHIFT_RIGHT:
            b = kNPSShiftRight;
            break;

        case '&':
            b = kNPSConcat;
            break;

        case kNPS_CONCAT2:
            b = kNPSConcat2;
            break;

        default:
            NPSError(kNErrSyntaxError);
            return kNewtRefUnbind;
    }

    return NPSGenNode2(b, op1, op2);
}


/*------------------------------------------------------------------------*/
/** メッセージ送信のオペノードを作成
 *
 * @param receiver	[in] レシーバ
 * @param op		[in] オペコード
 * @param msg		[in] メッセージ
 * @param args		[in] メッセージの引数
 *
 * @return			ノード
 */

nps_node_t NPSGenSend(nps_node_t receiver,
                uint32_t op, nps_node_t msg, nps_node_t args)
{
    nps_node_t	node;

    op = (op == ':')?kNPSSend:kNPSSendIfDefined;

    node = NPSGenNode2(kNPSMessage, msg, args);
    return NPSGenNode2(op, receiver, node);
}


/*------------------------------------------------------------------------*/
/** メッセージ再送信のオペノードを作成
 *
 * @param op		[in] オペコード
 * @param msg		[in] メッセージ
 * @param args		[in] メッセージの引数
 *
 * @return			ノード
 */

nps_node_t NPSGenResend(uint32_t op, nps_node_t msg, nps_node_t args)
{
    op = (op == ':')?kNPSResend:kNPSResendIfDefined;

    return NPSGenNode2(op, msg, args);
}


//--------------------------------------------------------------------------
/** 条件文のオペノードを作成
 *
 * @param cond		[in] 条件式
 * @param ifthen	[in] THEN 式
 * @param ifelse	[in] ELSE 式
 *
 * @return			ノード
 */

nps_node_t NPSGenIfThenElse(nps_node_t cond, nps_node_t ifthen, nps_node_t ifelse)
{
    nps_node_t	node;

    if (ifelse == kNewtRefUnbind)
    {
        node = ifthen;
    }
    else
    {
        node = NewtMakeArray(kNewtRefUnbind, 2);
        NewtSetArraySlot(node, 0, ifthen);
        NewtSetArraySlot(node, 1, ifelse);
    }

    return NPSGenNode2(kNPSIf, cond, node);
}


//--------------------------------------------------------------------------
/** FOR 文のオペノードを作成
 *
 * @param index		[in] インデックス変数
 * @param v			[in] 初期値
 * @param to		[in] 終了値
 * @param by		[in] ステップ値
 * @param expr		[in] 繰り返し式
 *
 * @return			ノード
 */

nps_node_t NPSGenForLoop(nps_node_t index, nps_node_t v,
                nps_node_t to, nps_node_t by, nps_node_t expr)
{
    newtRefVar	r;

    r = NewtMakeArray(kNewtRefUnbind, 4);
    NewtSetArraySlot(r, 0, index);
    NewtSetArraySlot(r, 1, v);
    NewtSetArraySlot(r, 2, to);
    NewtSetArraySlot(r, 3, by);

    return NPSGenNode2(kNPSFor, r, expr);
}


//--------------------------------------------------------------------------
/** FOREACH 文のオペノードを作成
 *
 * @param index		[in] インデックス変数
 * @param val		[in] 値を格納する変数
 * @param obj		[in] ループの対象となるオブジェクト
 * @param deeply	[in] deeply フラグ
 * @param op		[in] オペレーション種別（DO or COLLECT）
 * @param expr		[in] 繰り返し式
 *
 * @return			ノード
 */

nps_node_t NPSGenForeach(nps_node_t index, nps_node_t val, nps_node_t obj,
                nps_node_t deeply, nps_node_t op, nps_node_t expr)
{
    newtRefVar	r;

    r = NewtMakeArray(kNewtRefUnbind, 5);
    NewtSetArraySlot(r, 0, index);
    NewtSetArraySlot(r, 1, val);
    NewtSetArraySlot(r, 2, obj);
    NewtSetArraySlot(r, 3, deeply);
    NewtSetArraySlot(r, 4, op);

    return NPSGenNode2(kNPSForeach, r, expr);
}


//--------------------------------------------------------------------------
/** グローバル関数のオペノードを作成
 *
 * @param name		[in] 関数名
 * @param args		[in] 関数の引数
 * @param expr		[in] 実行式
 *
 * @return			ノード
 */

nps_node_t NPSGenGlobalFn(nps_node_t name, nps_node_t args, nps_node_t expr)
{
    nps_node_t	node;

    node = NPSGenNode2(kNPSFunc, args, expr);
    return NPSGenNode2(kNPSGlobalFn, name, node);
}


#if 0
#pragma mark -
#endif
//--------------------------------------------------------------------------
/** 参照パスオブジェクトの作成
 *
 * @param sym1		[in] シンボル１
 * @param sym2		[in] シンボル２
 *
 * @return			参照パスオブジェクト
 */

newtRef NPSMakePathExpr(newtRefArg sym1, newtRefArg sym2)
{
    newtRefVar	r;

    r = NewtMakeArray(NSSYM0(pathExpr), 2);
    NewtSetArraySlot(r, 0, sym1);
    NewtSetArraySlot(r, 1, sym2);

    return r;
}


//--------------------------------------------------------------------------
/** 配列オブジェクトの作成
 *
 * @param v			[in] 初期値
 *
 * @return			配列オブジェクト
 */

newtRef NPSMakeArray(newtRefArg v)
{
    newtRefVar	r;

    if (v == kNewtRefUnbind)
    {
        r = NewtMakeArray(kNewtRefUnbind, 0);
    }
    else
    {
        r = NewtMakeArray(kNewtRefUnbind, 1);
        NewtSetArraySlot(r, 0, v);
    }

    return r;
}


//--------------------------------------------------------------------------
/** 配列オブジェクトの最後にオブジェクトを追加する
 *
 * @param r			[in] 配列オブジェクト
 * @param v			[in] 追加するオブジェクト
 *
 * @return			配列オブジェクト
 */

newtRef NPSAddArraySlot(newtRefArg r, newtRefArg v)
{
    NcAddArraySlot(r, v);
    return r;
}


//--------------------------------------------------------------------------
/** 配列オブジェクトのオブジェクトを挿入する
 *
 * @param r			[in] 配列オブジェクト
 * @param p			[in] 挿入する位置
 * @param v			[in] 挿入るオブジェクト
 *
 * @return			配列オブジェクト
 */

newtRef NPSInsertArraySlot(newtRefArg r, uint32_t p, newtRefArg v)
{
    NewtInsertArraySlot(r, p, v);

    return (newtRef)r;
}


//--------------------------------------------------------------------------
/** フレームマップオブジェクトの作成
 *
 * @param v			[in] 初期値
 *
 * @return			フレームマップオブジェクト
 */

newtRef NPSMakeMap(newtRefArg v)
{
    newtRefVar	r;

    if (v == kNewtRefUnbind)
    {
        r = NewtMakeMap(kNewtRefNIL, 0, NULL);
    }
    else
    {
        r = NewtMakeMap(kNewtRefNIL, 1, NULL);
        NewtSetArraySlot(r, 1, v);
    }

    return r;
}


//--------------------------------------------------------------------------
/** フレームオブジェクトの作成
 *
 * @param slot		[in] スロットシンボル
 * @param v			[in] 初期値
 *
 * @return			フレームオブジェクト
 */

newtRef NPSMakeFrame(newtRefArg slot, newtRefArg v)
{
    newtRefVar	r;

    if (NewtRefIsNIL(slot))
    {
        r = NcMakeFrame();
    }
    else
    {
        newtRefVar	def[] = {slot, v};

        r = NewtMakeFrame2(1, def);
    }

    return r;
}


//--------------------------------------------------------------------------
/** フレームのスロットにオブジェクトをセットする
 *
 * @param r			[in] フレームオブジェクト
 * @param slot		[in] スロットシンボル
 * @param v			[in] オブジェクト
 *
 * @return			フレームオブジェクト
 */

newtRef NPSSetSlot(newtRefArg r, newtRefArg slot, newtRefArg v)
{
    NcSetSlot(r, slot, v);
    return r;
}


//--------------------------------------------------------------------------
/** バイナリオブジェクトの作成
 *
 * @param v			[in] 初期値
 *
 * @return			バイナリオブジェクト
 */

newtRef NPSMakeBinary(newtRefArg v)
{
    newtRefVar	r;

    if (v == kNewtRefUnbind)
    {
        r = NewtMakeBinary(kNewtRefUnbind, NULL, 0, false);
    }
    else if (NewtRefIsString(v))
    {
        r = NcClone(v);
        NcSetClass(r, kNewtRefUnbind);
        NewtBinarySetLength(r, NewtStringLength(v));
    }
    else
    {
        r = NewtMakeBinary(kNewtRefUnbind, NULL, 1, false);
        NewtSetARef(r, 0, v);
    }

    return r;
}


//--------------------------------------------------------------------------
/** バイナリオブジェクトの最後にデータを追加
 *
 * @param r			[in] バイナリオブジェクト
 * @param v			[in] 追加するデータ
 *
 * @return			バイナリオブジェクト
 */

newtRef NPSAddARef(newtRefArg r, newtRefArg v)
{
    int32_t	len;

    len = NewtBinaryLength(r);
    NewtBinarySetLength(r, len + 1);
    NewtSetARef(r, len, v);

    return r;
}


#if 0
#pragma mark -
#endif
//--------------------------------------------------------------------------
/** エラーメッセージの表示
 *
 * @param c			[in] エラー種別
 * @param s			[in] エラーメッセージ
 *
 * @return			なし
 */

void NPSErrorStr(char c, char * s)
{
    NewtFprintf(stderr, "%c:", c);

	switch (c)
	{
		case 'W':
			nps_env.numwarns++;
			break;

		case 'E':
		default:
			nps_env.numerrs++;
			break;
	}

	if (nps_env.fname != NULL)
		NewtFprintf(stderr, "\"%s\" ", nps_env.fname);

    NewtFprintf(stderr, "lines %d: %s:\n%s\n", nps_env.lineno, s, nps_env.linebuf);
#ifdef NEWT0_IGNORE_TABS
    NewtFprintf(stderr, "%*s\n", nps_env.tokenpos - nps_env.yyleng + 1, "^");
#else
    int i = 0, n = nps_env.tokenpos - nps_env.yyleng, m = strlen(nps_env.linebuf);
    for ( ; i<n; ++i ) {
        if (i<m && nps_env.linebuf[i]=='\t') 
            NewtFprintf(stderr, "\t");
        else
            NewtFprintf(stderr, " ");
    }
    NewtFprintf(stderr, "^\n");
#endif
}


//--------------------------------------------------------------------------
/** 構文エラー
 *
 * @param err		[in] エラーコード
 *
 * @return			なし
 */

void NPSError(int32_t err)
{
    yyerror("E:Syntax error");
}
