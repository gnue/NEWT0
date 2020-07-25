/*------------------------------------------------------------------------*/
/**
 * @file	NewtEnv.c
 * @brief   実行環境
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <string.h>
#include <errno.h>

#include "version.h"
#include "NewtEnv.h"
#include "NewtObj.h"
#include "NewtFns.h"
#include "NewtGC.h"
#include "NewtStr.h"
#include "NewtFile.h"


/* マクロ */
#define SYM_TABLE			(newt_env.sym_table)				///< シンボルテーブル
#define ROOT				(newt_env.root)						///< ルートオブジェクト
#define GLOBALS				(newt_env.globals)					///< グローバル変数テーブル
#define GLOBAL_FNS			(newt_env.global_fns)				///< グローバル関数テーブル
#define MAGIC_POINTERS		(newt_env.magic_pointers)			///< マジックポインタテーブル
#define NAMED_MPS			(newt_env.named_mps)				///< 名前付マジックポインタテーブル

#define INITSYM2(sym, str)	sym = NewtMakeSymbol(str)			///< よく使うシンボルの初期化
#define INITSYM(name)		INITSYM2(newt_sym.name, #name)		///< よく使うシンボルの初期化（特殊文字なし）


/* グローバル変数 */
newt_env_t	newt_env;		///< 実行環境
newt_sym_t	newt_sym;		///< よく使うシンボルの保管場所


/* 関数プロトタイプ */
char *	replacechr(char * str, char srch, char repl);

static void		NewtInitSYM(void);
static void		NewtInitSysEnv(void);
static void		NewtInitARGV(int argc, const char * argv[], int n);
static void		NewtInitVersInfo(void);
static void		NewtInitEnv(int argc, const char * argv[], int n);

static newtRef	NcResolveNamedMP(newtRefArg r);
static newtRef	NsDefNamedMP(newtRefArg rcvr, newtRefArg r, newtRefArg v);


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 文字列の文字を指定された文字で置き換える
 *
 * @param str		[i/o]文字列
 * @param src		[in] 置換えられる文字
 * @param dest		[in] 置換える文字
 *
 * @return			置き換えられた文字列
 */

char * replacechr(char * str, char src, char dest)
{
	char *	s;

	for (s = str; *s; s++)
	{
		if (*s == src) *s = dest;
	}

	return str;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 環境変数からデフォルトエンコーディングを取得
 *
 * @return			デフォルトエンコーディング（文字列）
 */

char * NewtDefaultEncoding(void)
{
	char *	encoding = NULL;
	char *	lang;

	lang = (char *)getenv("LANG");
	if (! lang) lang = (char *)getenv("LC_ALL");
	if (lang) encoding = strchr(lang, '.');

	if (encoding)
		encoding++;
	else
		encoding = NEWT_DEFAULT_ENCODING;

	return encoding;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** よく使うシンボルの初期化
 *
 * @return			なし
 */

void NewtInitSYM(void)
{
    // frame
    INITSYM(_proto);
    INITSYM(_parent);

    // function
    INITSYM(_implementor);
    INITSYM(_nextArgFrame);
    INITSYM(CodeBlock);
    INITSYM2(NS_CLASS, "class");
    INITSYM(instructions);
    INITSYM(literals);
    INITSYM(argFrame);
    INITSYM(numArgs);
    INITSYM(indefinite);

    // native function
    INITSYM(_function.native0);
    INITSYM(_function.native);
    INITSYM(funcPtr);
    INITSYM(docString);

    // classes or types
    INITSYM(binary);
    INITSYM(string);
    INITSYM(real);
    INITSYM(array);
    INITSYM(frame);
    INITSYM2(NS_INT, "int");
    INITSYM(int32);
    INITSYM(int64);
    INITSYM(pathExpr);
    INITSYM(bits);
    INITSYM(cbits);
    INITSYM(nativeModule);
    INITSYM(CObject);
    INITSYM(code);

    // for loop
    INITSYM(collect);
    INITSYM(deeply);

    // class
    INITSYM2(NS_CHAR, "char");
    INITSYM(boolean);
    INITSYM(weird_immediate);
    INITSYM(forEachState);

    // functions（必須）
    INITSYM(hasVariable);
    INITSYM(hasVar);
    INITSYM(defGlobalFn);
    INITSYM(defGlobalVar);
//    INITSYM(and);
//    INITSYM(or);
    INITSYM(mod);
    INITSYM(shiftLeft);
    INITSYM(shiftRight);
    INITSYM(objectEqual);
    INITSYM(defMagicPointer);
	INITSYM(makeRegex);

    // exception type
    INITSYM(type.ref);
    INITSYM(evt.ex.msg);

    // exception frame
    INITSYM(name);
    INITSYM(data);
    INITSYM(message);
    INITSYM(error);

    INITSYM(errorCode);
    INITSYM(symbol);
    INITSYM(value);
    INITSYM(index);

    // root
	INITSYM(sym_table);
    INITSYM(globals);
    INITSYM(global_fns);
    INITSYM(magic_pointers);
#ifdef __NAMED_MAGIC_POINTER__
    INITSYM(named_mps);
#endif /* __NAMED_MAGIC_POINTER__ */

    // for print
    INITSYM(printDepth);
    INITSYM(printIndent);
    INITSYM(printLength);
    INITSYM(printBinaries);

	// for regex
    INITSYM(protoREGEX);
    INITSYM(regex);
    INITSYM(pattern);
    INITSYM(option);

	// for require
    INITSYM(requires);

    // ENV
    INITSYM(_ENV_);
    INITSYM(NEWTLIB);

    // Compile options
    INITSYM(nosCompatible);
    INITSYM(nos1Functions);

	// ARGV
    INITSYM(_ARGV_);
    INITSYM(_EXEDIR_);

    // stdout, stderr
    INITSYM(_STDOUT_);
    INITSYM(_STDERR_);
}


/*------------------------------------------------------------------------*/
/** システム環境変数の初期化
 *
 * @return			なし
 */

void NewtInitSysEnv(void)
{
	struct {
		char *			name;
		newtRefVar  	slot;
		const char *	defaultValue;
	} envs[] = {
		{"NEWTLIB",		NSSYM0(NEWTLIB),	__LIBDIR__ ":."},
		{"PLATFORM",	NSSYM(PLATFORM),	__PLATFORM__},
		{"DYLIBSUFFIX",	NSSYM(DYLIBSUFFIX),	__DYLIBSUFFIX__},
		{NULL,			kNewtRefUnbind,		NULL}
	};

	newtRefVar  env;
	newtRefVar  proto;
	newtRefVar  v;
	uint16_t	i;

	env = NcMakeFrame();
	proto = NcMakeFrame();

	for (i = 0; envs[i].name != NULL; i++)
	{
		NcSetSlot(proto, envs[i].slot, NewtMakeString(envs[i].defaultValue, true));
	}

	/* NEWTLIB is a special case, it can be overridden from the value in
	   the global variables. */
	v = NewtGetEnv("NEWTLIB");
	if (NewtRefIsString(v))
	{
		v = NcSplit(v, NewtMakeCharacter(':'));
		NcSetSlot(proto, NSSYM0(NEWTLIB), v);
	}
	else
	{
		newtRefVar default_newtlib_array[] = {NSSTR(__LIBDIR__), NSSTR(".")};
		newtRefVar default_newtlib = NewtMakeArray2(
				kNewtRefNIL,
				sizeof(default_newtlib_array) / sizeof(newtRefVar),
				default_newtlib_array);
		NcSetSlot(proto, NSSYM0(NEWTLIB), default_newtlib);
	}

	NcSetSlot(env, NSSYM0(_proto), NewtPackLiteral(proto));
    NcSetSlot(GLOBALS, NSSYM0(_ENV_), env);
}


/*------------------------------------------------------------------------*/
/** コマンドライン引数の初期化
 *
 * @param argc		[in] コマンドライン引数の数
 * @param argv		[in] コマンドライン引数の配列
 * @param n			[in] コマンドライン引数の位置
 *
 * @return			なし
 */
 
void NewtInitARGV(int argc, const char * argv[], int n)
{
	newtRefVar  exepath = kNewtRefUnbind;
	newtRefVar  r;
	uint16_t	j;
	uint16_t	i;

	r = NewtMakeArray(kNewtRefUnbind, argc - n);
    NcSetSlot(GLOBALS, NSSYM0(_ARGV_), r);

	for (i = n, j = 0; i < argc; i++, j++)
	{
		NewtSetArraySlot(r, j, NewtMakeString(argv[i], true));
	}

#ifdef __WIN32__
	{
		char	sep;

		sep = NewtGetFileSeparator();

		if (sep != '\\')
		{
			char *	path;

			path = strdup(argv[0]);

			if (path)
			{
				replacechr(path, '\\', sep);
				exepath = NewtExpandPath(path);
				free(path);
			}
			else
			{
				exit(errno);
			}
		}
	}
#endif

	if (NewtRefIsNIL(exepath) && argc > 0) {
    exepath = NewtExpandPath(argv[0]);
  }

  if (exepath != kNewtRefUnbind) {
    NcSetSlot(GLOBALS, NSSYM0(_EXEDIR_), NcDirName(exepath));
  }
}


/*------------------------------------------------------------------------*/
/** バージョン情報の初期化
 *
 * @return			なし
 */

void NewtInitVersInfo(void)
{
	newtRefVar  versInfo;

	versInfo = NcMakeFrame();

	// プロダクト名
    NcSetSlot(versInfo, NSSYM(name), NewtMakeString(NEWT_NAME, true));
	// プロト番号
    NcSetSlot(versInfo, NSSYM(proto), NewtMakeString(NEWT_PROTO, true));
	// バージョン番号
    NcSetSlot(versInfo, NSSYM(version), NewtMakeString(NEWT_VERSION, true));
	// ビルド番号
    NcSetSlot(versInfo, NSSYM(build), NewtMakeString(NEWT_BUILD, true));
	// コピーライト
    NcSetSlot(versInfo, NSSYM(copyright), NewtMakeString(NEWT_COPYRIGHT, true));
	// スタッフロール
    NcSetSlot(versInfo, NSSYM(staff), NewtMakeString(NEWT_STAFF, true));

	// リードオンリーにしてグローバル変数に入れる
    NcSetSlot(GLOBALS, NSSYM(_VERSION_), NewtPackLiteral(versInfo));
}


/*------------------------------------------------------------------------*/
/** 実行環境の初期化
 *
 * @param argc		[in] コマンドライン引数の数
 * @param argv		[in] コマンドライン引数の配列
 * @param n			[in] コマンドライン引数の位置
 *
 * @return			なし
 */

void NewtInitEnv(int argc, const char * argv[], int n)
{
	// シンボルテーブルの作成
    SYM_TABLE = NewtMakeArray(kNewtRefUnbind, 0);
    NewtInitSYM();

	// ルートフレームの作成
    ROOT = NcMakeFrame();
	// グローバル変数テーブルの作成
    GLOBALS = NcMakeFrame();
	// グローバル関数テーブルの作成
    GLOBAL_FNS = NcMakeFrame();
	// マジックポインタテーブルの作成
    MAGIC_POINTERS = NewtMakeArray(kNewtRefUnbind, 0);

	// ルートフレームに各テーブルを格納
    NcSetSlot(ROOT, NSSYM0(globals), GLOBALS);
    NcSetSlot(ROOT, NSSYM0(global_fns), GLOBAL_FNS);
    NcSetSlot(ROOT, NSSYM0(magic_pointers), MAGIC_POINTERS);
    NcSetSlot(ROOT, NSSYM0(sym_table), SYM_TABLE);

#ifdef __NAMED_MAGIC_POINTER__
	// 名前付マジックポインタテーブルの作成
    NAMED_MPS = NcMakeFrame();
	// ルートフレームに名前付マジックポインタテーブルの格納
    NcSetSlot(ROOT, NSSYM0(named_mps), NAMED_MPS);
#endif /* __NAMED_MAGIC_POINTER__ */

	// 環境変数の初期化
	NewtInitSysEnv();
	// ARGV の初期化
	NewtInitARGV(argc, argv, n);
	// バージョン情報の初期化
	NewtInitVersInfo();
}


/*------------------------------------------------------------------------*/
/** インタプリタの初期化
 *
 * @param argc		[in] コマンドライン引数の数
 * @param argv		[in] コマンドライン引数の配列
 * @param n			[in] コマンドライン引数の位置
 *
 * @return			なし
 */

void NewtInit(int argc, const char * argv[], int n)
{
	// メモリプールの確保
    NEWT_POOL = NewtPoolAlloc(NEWT_POOL_EXPANDSPACE);
	// 実行環境の初期化
    NewtInitEnv(argc, argv, n);
}


/*------------------------------------------------------------------------*/
/** インタプリタの後始末
 *
 * @return			なし
 */

void NewtCleanup(void)
{
    // 後始末をすること

	// メモリプールの解放
    if (NEWT_POOL != NULL)
    {
        NewtPoolRelease(NEWT_POOL);
        NewtMemFree(NEWT_POOL);
        NEWT_POOL = NULL;
    }
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** シンボルテーブルからシンボルを検索する
 *
 * @param name		[in] シンボルの名前
 *
 * @return			シンボルオブジェクト
 */

newtRef NewtLookupSymbolTable(const char * name)
{
    return NewtLookupSymbol(SYM_TABLE, name, 0, 0);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** グローバル関数の有無を調べる
 *
 * @param r			[in] シンボルオブジェクト
 *
 * @retval			true	グローバル関数が存在する
 * @retval			false   グローバル関数が存在しない
 */

bool NewtHasGlobalFn(newtRefArg r)
{
    return NewtHasSlot(GLOBAL_FNS, r);
}


/*------------------------------------------------------------------------*/
/** グローバル変数の有無を調べる
 *
 * @param r			[in] シンボルオブジェクト
 *
 * @retval			true	グローバル変数が存在する
 * @retval			false   グローバル変数が存在しない
 */

bool NewtHasGlobalVar(newtRefArg r)
{
    return NewtHasSlot(GLOBALS, r);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** グローバル関数の有無を調べる
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] シンボルオブジェクト
 *
 * @retval			TRUE	グローバル関数が存在する
 * @retval			NIL		グローバル関数が存在しない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsGlobalFnExists(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtHasGlobalFn(r));
}


#ifdef __USE_OBSOLETE_STYLE__
/*------------------------------------------------------------------------*/
/** グローバル関数の有無を調べる (OBSOLETE)
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] シンボルオブジェクト
 *
 * @retval			TRUE	グローバル関数が存在する
 * @retval			NIL		グローバル関数が存在しない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsHasGlobalFn(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtHasGlobalFn(r));
}
#endif /* __USE_OBSOLETE_STYLE__ */


/*------------------------------------------------------------------------*/
/** グローバル関数の取得
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] シンボルオブジェクト
 *
 * @return			関数オブジェクト
 */

newtRef NsGetGlobalFn(newtRefArg rcvr, newtRefArg r)
{
    return NcGetSlot(GLOBAL_FNS, r);
}


/*------------------------------------------------------------------------*/
/** グローバル関数の定義
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] シンボルオブジェクト
 * @param fn		[in] 関数オブジェクト
 *
 * @return			関数オブジェクト
 */

newtRef NsDefGlobalFn(newtRefArg rcvr, newtRefArg r, newtRefArg fn)
{
    return NcSetSlot(GLOBAL_FNS, r, fn);
}


/*------------------------------------------------------------------------*/
/** Undefine a global function.
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] シンボルオブジェクト
 *
 * @return			NIL
 */

newtRef NsUndefGlobalFn(newtRefArg rcvr, newtRefArg r)
{
    (void) NcRemoveSlot(GLOBAL_FNS, r);
    return kNewtRefNIL;
}


/*------------------------------------------------------------------------*/
/** グローバル変数の有無を調べる
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] シンボルオブジェクト
 *
 * @retval			TRUE	グローバル変数が存在する
 * @retval			NIL		グローバル変数が存在しない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsGlobalVarExists(newtRefArg rcvr, newtRefArg r)
{
    return NewtMakeBoolean(NewtHasGlobalVar(r));
}


#ifdef __USE_OBSOLETE_STYLE__
/*------------------------------------------------------------------------*/
/** グローバル変数の有無を調べる (OBSOLETE)
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] シンボルオブジェクト
 *
 * @retval			TRUE	グローバル変数が存在する
 * @retval			NIL		グローバル変数が存在しない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsHasGlobalVar(newtRefArg rcvr, newtRefArg r)
{
    return NsGlobalVarExists(rcvr, r);
}
#endif /* __USE_OBSOLETE_STYLE__ */


/*------------------------------------------------------------------------*/
/** グローバル変数の取得
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] シンボルオブジェクト
 *
 * @return			オブジェクト
 */

newtRef NsGetGlobalVar(newtRefArg rcvr, newtRefArg r)
{
    return NcGetSlot(GLOBALS, r);
}


/*------------------------------------------------------------------------*/
/** グローバル変数に値をセットする
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] シンボルオブジェクト
 * @param v			[in] 値オブジェクト
 *
 * @return			オブジェクト
 */

newtRef NsDefGlobalVar(newtRefArg rcvr, newtRefArg r, newtRefArg v)
{
    return NcSetSlot(GLOBALS, r, v);
}


#ifdef __USE_OBSOLETE_STYLE__
/*------------------------------------------------------------------------*/
/** グローバル変数に値をセットする (OBSOLETE)
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] シンボルオブジェクト
 * @param v			[in] 値オブジェクト
 *
 * @return			オブジェクト
 */

newtRef NsSetGlobalVar(newtRefArg rcvr, newtRefArg r, newtRefArg v)
{
    return NcSetSlot(GLOBALS, r, v);
}
#endif /* __USE_OBSOLETE_STYLE__ */


/*------------------------------------------------------------------------*/
/** Undefine a global variable.
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] シンボルオブジェクト
 *
 * @return			NIL
 */

newtRef NsUndefGlobalVar(newtRefArg rcvr, newtRefArg r)
{
    (void) NcRemoveSlot(GLOBALS, r);
    return kNewtRefNIL;
}


#ifdef __NAMED_MAGIC_POINTER__

/*------------------------------------------------------------------------*/
/** 名前付マジックポインタの参照を解決する
 *
 * @param r			[in] マジックポインタ
 *
 * @return			オブジェクト
 */

newtRef NcResolveNamedMP(newtRefArg r)
{
	newtRefVar	sym;

	if (! NewtRefIsNamedMP(r))
		return r;

	sym = NewtMPToSymbol(r);

	if (NewtHasSlot(NAMED_MPS, sym))
		return NcGetSlot(NAMED_MPS, sym);
	else
		return r;
}


/*------------------------------------------------------------------------*/
/** 名前付マジックポインタの定義
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] マジックポインタ
 * @param v			[in] オブジェクト
 *
 * @return			オブジェクト
 */

newtRef NsDefNamedMP(newtRefArg rcvr, newtRefArg r, newtRefArg v)
{
	newtRefVar	sym;

	if (NewtRefIsNamedMP(r))
	{
		sym = NewtMPToSymbol(r);
	}
	else if (NewtRefIsSymbol(r))
	{
		sym = r;
	}
	else
	{
		return r;
	}

    return NcSetSlot(NAMED_MPS, sym, v);
}

#endif /* __NAMED_MAGIC_POINTER__ */


/*------------------------------------------------------------------------*/
/** マジックポインタの参照を解決する
 *
 * @param r			[in] マジックポインタ
 *
 * @return			オブジェクト
 */

newtRef NcResolveMagicPointer(newtRefArg r)
{
	int32_t	table = 0;
	int32_t	index;

	if (! NewtRefIsNumberedMP(r))
	{
#ifdef __NAMED_MAGIC_POINTER__
		return NcResolveNamedMP(r);
#else
		return r;
#endif
	}

	table = NewtMPToTable(r);
	index = NewtMPToIndex(r);

	if (table != 0)
	{	// テーブル番号 0 以外は未サポート
		return r;
	}

	if (index < NewtLength(MAGIC_POINTERS))
	{
		newtRefVar	result;

		result = NewtGetArraySlot(MAGIC_POINTERS, index);

		if (result != kNewtRefUnbind)
			return result;
	}

	return r;
}


/*------------------------------------------------------------------------*/
/** マジックポインタの定義
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] マジックポインタ
 * @param v			[in] オブジェクト
 *
 * @return			オブジェクト
 */

newtRef NsDefMagicPointer(newtRefArg rcvr, newtRefArg r, newtRefArg v)
{
	int32_t	table = 0;
	intptr_t index;

	if (NewtRefIsNumberedMP(r))
	{
		table = NewtMPToTable(r);
		index = NewtMPToIndex(r);

		if (table != 0)
		{	// テーブル番号 0 以外は未サポート
			return kNewtRefUnbind;
		}
	}
	else if (NewtRefIsInteger(r))
	{
		index = NewtRefToInteger(r);
	}
	else
	{
#ifdef __NAMED_MAGIC_POINTER__
		return NsDefNamedMP(rcvr, r, v);
#else
		return kNewtRefUnbind;
#endif
	}

	if (NewtLength(MAGIC_POINTERS) <= index)
	{	// テーブルの長さを拡張
		NewtSetLength(MAGIC_POINTERS, index + 1);
	}

	return NewtSetArraySlot(MAGIC_POINTERS, index, v);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** ルートオブジェクトの取得
 *
 * @param rcvr		[in] レシーバ
 *
 * @return			ルートオブジェクト
 */

newtRef NsGetRoot(newtRefArg rcvr)
{
    return ROOT;
}


/*------------------------------------------------------------------------*/
/** グローバル変数テーブルの取得
 *
 * @param rcvr		[in] レシーバ
 *
 * @return			グローバル変数テーブル
 */

newtRef NsGetGlobals(newtRefArg rcvr)
{
    return GLOBALS;
}


/*------------------------------------------------------------------------*/
/** グローバル関数テーブルの取得
 *
 * @param rcvr		[in] レシーバ
 *
 * @return			グローバル関数テーブル
 */

newtRef NsGetGlobalFns(newtRefArg rcvr)
{
    return GLOBAL_FNS;
}


/*------------------------------------------------------------------------*/
/** マジックポインタ関数テーブルの取得
 *
 * @param rcvr		[in] レシーバ
 *
 * @return			マジックポインタ関数テーブル
 */

newtRef NsGetMagicPointers(newtRefArg rcvr)
{
    return MAGIC_POINTERS;
}


/*------------------------------------------------------------------------*/
/** シンボルテーブルの取得
 *
 * @param rcvr		[in] レシーバ
 *
 * @return			シンボルテーブル
 */

newtRef NsGetSymTable(newtRefArg rcvr)
{
    return SYM_TABLE;
}
