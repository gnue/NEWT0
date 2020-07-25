/*------------------------------------------------------------------------*/
/**
 * @file	NewtEnv.h
 * @brief   実行環境
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTENV_H
#define	NEWTENV_H


/* ヘッダファイル */
#include "NewtType.h"
#include "NewtMem.h"


/* マクロ */
#define NEWT_DEBUG			(newt_env._debug)				///< デバッグフラグ
#define NEWT_TRACE			(newt_env._trace)				///< トレースフラグ
#define NEWT_DUMPLEX		(newt_env._dumpLex)				///< ダンプ字句解析フラグ
#define NEWT_DUMPSYNTAX		(newt_env._dumpSyntax)			///< ダンプ構文木フラグ
#define NEWT_DUMPBC			(newt_env._dumpBC)				///< ダンプバイトコードフラグ
#define NEWT_INDENT			(newt_env._indent)				///< Enable indenting when printing
#define NEWT_POOL			(newt_env.pool)					///< メモリプール
#define NEWT_SWEEP			(newt_env.sweep)				///< SWEEPフラグ
#define NEWT_NEEDGC			(newt_env.needgc)				///< GCフラグ
#define NEWT_MODE_NOS2		(newt_env.mode.nos2)			///< NOS2 コンパチブル
#define NEWT_MODE_NOS1_FUNCTIONS	(newt_env.mode.nos1Functions)	///< As opposed to NewtonOS 2.x-only faster functions

#define NSSTR(s)			(NewtMakeString(s, false))		///< 文字列オブジェクトの作成
#define NSSTRCONST(s)		(NewtMakeString(s, true))		///< 文字列定数オブジェクトの作成
#define NSINT(n)			(NewtMakeInteger(n))			///< 整数オブジェクトの作成
#define NSREAL(n)			(NewtMakeReal(n))				///< 浮動小数点オブジェクトの作成

#define NSSYM0(name)		newt_sym.name					///< 保管場所からシンボルオブジェクトを取得
#define NSSYM(name)			(NewtMakeSymbol(#name))			///< シンボルオブジェクトの作成

#define NSNAMEDMP(name)		(NewtMakeNamedMP(#name))		///< 名前付マジックポインタの作成
#define NSNAMEDMP0(name)	(NewtSymbolToMP(NSSYM0(name)))	///< 保管場所から名前付マジックポインタを取得
#define NSMP(n)				(NewtMakeMagicPointer(0, n))	///< マジックポインタの作成

#define NS_CLASS			NSSYM0(__class)					///< class シンボル
#define NS_INT				NSSYM0(__int)					///< int シンボル
#define NS_CHAR				NSSYM0(__char)					///< char シンボル

#define NcGlobalFnExists(r)				NsGlobalFnExists(kNewtRefNIL, r)
#define NcGetGlobalFn(r)				NsGetGlobalFn(kNewtRefNIL, r)
#define NcDefGlobalFn(r, fn)			NsDefGlobalFn(kNewtRefNIL, r, fn)
#define NcUndefGlobalFn(r)				NsUndefGlobalFn(kNewtRefNIL, r)
#define NcGlobalVarExists(r)			NsGlobalVarExists(kNewtRefNIL, r)
#define NcGetGlobalVar(r)				NsGetGlobalVar(kNewtRefNIL, r)
#define NcDefGlobalVar(r, v)			NsDefGlobalVar(kNewtRefNIL, r, v)
#define NcUndefGlobalVar(r)				NsUndefGlobalVar(kNewtRefNIL, r)
#define NcDefMagicPointer(r, v)			NsDefMagicPointer(kNewtRefNIL, r, v)
#define NcGetRoot()						NsGetRoot(kNewtRefNIL)
#define NcGetGlobals()					NsGetGlobals(kNewtRefNIL)
#define NcGetGlobalFns()				NsGetGlobalFns(kNewtRefNIL)
#define NcGetMagicPointers()			NsGetMagicPointers(kNewtRefNIL)
#define NcGetSymTable()					NsGetSymTable(kNewtRefNIL)

// OBSOLETE
#define NcHasGlobalFn(r)				NsGlobalFnExists(kNewtRefNIL, r)
#define NcHasGlobalVar(r)				NsGlobalVarExists(kNewtRefNIL, r)
#define NcSetGlobalVar(r, v)			NsDefGlobalVar(kNewtRefNIL, r, v)


/// 実行環境
typedef struct {
    newtRefVar	sym_table;		///< シンボルテーブル
    newtRefVar	root;			///< ルート
    newtRefVar	globals;		///< グローバル変数テーブル
    newtRefVar	global_fns;		///< グローバル関数テーブル
    newtRefVar	magic_pointers;	///< マジックポインタテーブル

#ifdef __NAMED_MAGIC_POINTER__
    newtRefVar	named_mps;		///< 名前付マジックポインタテーブル
#endif /* __NAMED_MAGIC_POINTER__ */

	// メモリ関係
    newtPool	pool;			///< メモリプール
    bool		sweep;			///< 現在の sweep 状態（トグルする）
    bool		needgc;			///< GC が必要

	/// モード
	struct {
		bool	nos1Functions;	///< As opposed to NewtonOS 2.x-only faster functions
		bool	nos2;			///< NOS2 コンパチブル
	} mode;

    // デバッグ
    bool		_debug;			///< デバッグフラグ
    bool		_trace;			///< トレースフラグ
    bool		_dumpLex;		///< 字句解析ダンプフラグ
    bool		_dumpSyntax;	///< 構文木ダンプフラグ
    bool		_dumpBC;		///< バイトコードダンプフラグ
	int32_t		_indent;		///< number of tabs for indenting a printout
	int32_t		_indentDepth;	///< base for calculating the indent depth
	int32_t		_printBinaries;	///< print binary objects so that they can be regenerated from the printout
} newt_env_t;


/// よく使うシンボル
typedef struct {
    // frame
    newtRefVar	_proto;				///< _proto
    newtRefVar	_parent;			///< _parent

    // function
    newtRefVar	_implementor;		///< _implementor
    newtRefVar	_nextArgFrame;		///< _nextArgFrame
    newtRefVar	CodeBlock;			///< CodeBlock
    newtRefVar	__class;			///< class
    newtRefVar	instructions;		///< instructions
    newtRefVar	literals;			///< literals
    newtRefVar	argFrame;			///< argFrame
    newtRefVar	numArgs;			///< numArgs
    newtRefVar	indefinite;			///< indefinite

    // native function

	/// function
    struct {
        newtRefVar	native0;		///< function.native0 （rcvrなし）
        newtRefVar	native;			///< function.native （rcvrあり）
    } _function;

    newtRefVar	funcPtr;			///< funcPtr
    newtRefVar	docString;			///< docString

    // classes or types
    newtRefVar	binary;				///< binary
    newtRefVar	string;				///< string
    newtRefVar	real;				///< real
    newtRefVar	array;				///< array
    newtRefVar	frame;				///< frame
    newtRefVar	__int;				///< int
    newtRefVar	int32;				///< int32
    newtRefVar  int64;              ///< int64
    newtRefVar	pathExpr;			///< pathExpr
    newtRefVar	bits;			///< bits
    newtRefVar	cbits;			///< cbits
    newtRefVar	nativeModule;			///< NTKC native module
    newtRefVar	CObject;			///< GC-aware CObjects
    newtRefVar	code;			///< NTKC code block

    // for loop
    newtRefVar	collect;			///< collect
    newtRefVar	deeply;				///< deeply

    // class	
    newtRefVar	__char;				///< char
    newtRefVar	boolean;			///< boolean
    newtRefVar	weird_immediate;	///< weird_immediate
    newtRefVar	forEachState;		///< forEachState

    // functions（必須）
    newtRefVar	hasVariable;		///< hasVariable
    newtRefVar	hasVar;				///< hasVar
    newtRefVar	defGlobalFn;		///< defGlobalFn
    newtRefVar	defGlobalVar;		///< defGlobalVar
//    newtRefVar	and;				///< and
//    newtRefVar	or;					///< or
    newtRefVar	mod;				///< mod
    newtRefVar	shiftLeft;			///< <<
    newtRefVar	shiftRight;			///< >>
    newtRefVar	objectEqual;		///< ==
    newtRefVar	defMagicPointer;	///< @0 := value
    newtRefVar	makeRegex;			///< makeRegex

    // exception frame

	/// type
    struct {
        newtRefVar	ref;			///< type.ref
    } type;

	/// evt
    struct {
		/// evt.ex
        struct {
            newtRefVar	msg;		///< evt.ex.msg
        } ex;
    } evt;

    newtRefVar	name;				///< name
    newtRefVar	data;				///< data
    newtRefVar	message;			///< message
    newtRefVar	error;				///< error

    newtRefVar	errorCode;			///< errorCode
    newtRefVar	symbol;				///< symbol
    newtRefVar	value;				///< value
    newtRefVar	index;				///< index

    // root
    newtRefVar	sym_table;			///< sym_table
    newtRefVar	globals;			///< globals
    newtRefVar	global_fns;			///< global_fns
    newtRefVar	magic_pointers;		///< magic_pointers
    newtRefVar	named_mps;			///< named_mps

    // for print
    newtRefVar	printDepth;			///< printDepth
    newtRefVar	printIndent;			///< printIndent
    newtRefVar	printLength;		///< printLength
    newtRefVar	printBinaries;		///< printBinaries

    // for regex
    newtRefVar	protoREGEX;			///< @protoREGEX
    newtRefVar	regex;				///< regex
    newtRefVar	pattern;			///< pattern
    newtRefVar	option;				///< option

	// for require
    newtRefVar	requires;			///< requires

	// ENV
    newtRefVar	_ENV_;				///< _ENV_
    newtRefVar	NEWTLIB;			///< NEWTLIB

	// Compile options
    newtRefVar	nosCompatible;		///< nosCompatible
    newtRefVar	nos1Functions;		///< nos1Functions

	// ARGV
    newtRefVar	_ARGV_;				///< _ARGV_
	newtRefVar  _EXEDIR_;			///< _EXEDIR_

	// stdout, stderr
	newtRefVar  _STDOUT_;			///< _STDOUT_
	newtRefVar  _STDERR_;			///< _STDERR_
} newt_sym_t;


/* グローバル変数 */
extern newt_env_t	newt_env;		///< 実行環境
extern newt_sym_t	newt_sym;		///< よく使うシンボルの保管場所


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


char *		NewtDefaultEncoding(void);
void		NewtInit(int argc, const char * argv[], int n);
void		NewtCleanup(void);

newtRef		NewtLookupSymbolTable(const char * name);

bool		NewtHasGlobalFn(newtRefArg r);
bool		NewtHasGlobalVar(newtRefArg r);

// NewtonScript native functions(new style)
newtRef		NsGlobalFnExists(newtRefArg rcvr, newtRefArg r);
newtRef		NsGetGlobalFn(newtRefArg rcvr, newtRefArg r);
newtRef		NsDefGlobalFn(newtRefArg rcvr, newtRefArg r, newtRefArg fn);
newtRef		NsUndefGlobalFn(newtRefArg rcvr, newtRefArg r);
newtRef		NsGlobalVarExists(newtRefArg rcvr, newtRefArg r);
newtRef		NsGetGlobalVar(newtRefArg rcvr, newtRefArg r);
newtRef		NsDefGlobalVar(newtRefArg rcvr, newtRefArg r, newtRefArg v);
newtRef		NsUndefGlobalVar(newtRefArg rcvr, newtRefArg r);
newtRef		NcResolveMagicPointer(newtRefArg r);
newtRef		NsDefMagicPointer(newtRefArg rcvr, newtRefArg r, newtRefArg v);

newtRef		NsGetRoot(newtRefArg rcvr);
newtRef		NsGetGlobals(newtRefArg rcvr);
newtRef		NsGetGlobalFns(newtRefArg rcvr);
newtRef		NsGetMagicPointers(newtRefArg rcvr);
newtRef		NsGetSymTable(newtRefArg rcvr);

#ifdef __USE_OBSOLETE_STYLE__
newtRef		NsHasGlobalFn(newtRefArg rcvr, newtRefArg r);					// OBSOLETE
newtRef		NsHasGlobalVar(newtRefArg rcvr, newtRefArg r);					// OBSOLETE
newtRef		NsSetGlobalVar(newtRefArg rcvr, newtRefArg r, newtRefArg v);	// OBSOLETE
#endif /* __USE_OBSOLETE_STYLE__ */


#ifdef __cplusplus
}
#endif


#endif /* NEWTENV_H */

