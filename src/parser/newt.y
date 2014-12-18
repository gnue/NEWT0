/*------------------------------------------------------------------------*/
/**
 * @file	newt.y
 * @brief   構文解析
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */

%{

/* ヘッダファイル */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yacc.h"

#include "NewtCore.h"
#include "NewtParser.h"


/* マクロ */
#define SYMCHECK(v, sym)	if (v != sym) NPSError(kNErrSyntaxError);
#define TYPECHECK(v)		if (! (v == NS_INT || v == NSSYM0(array))) NPSError(kNErrSyntaxError);
#define ERR_NOS2C(msg)		if (NEWT_MODE_NOS2) yyerror(msg);


%}

%union {
    newtRefVar	obj;	// オブジェクト
    uint32_t	op;		// 演算子
    nps_node_t	node;	// ノード
}

   /* タイプ */

%type	<node>	constituent_list	constituent
%type	<node>	expr
%type	<node>	simple_expr
%type	<node>	compound_expr
%type	<node>	expr_sequence
%type	<node>	constructor
%type	<node>	lvalue
%type	<node>	exists_expr
%type	<node>	local_declaration	l_init_clause_list	init_clause
%type	<node>	c_init_clause_list	c_init_clause
%type	<node>	if_expr
%type	<node>	iteration
%type	<node>	function_call
%type	<node>	actual_argument_list
%type	<node>	message_send
%type	<node>	by_expr
%type	<node>	expr_list
%type	<node>	onexcp_list	onexcp
%type	<node>	frame_constructor_list	frame_slot_value	frame_slot_list
%type	<node>	frame_accessor	array_accessor
%type	<node>	formal_argument_list	formal_argument_list2	formal_argument	indefinite_argument
%type	<node>	global_declaration	global_function_decl	function_decl
%type	<node>	type

%type	<obj>	literal
%type	<obj>	simple_literal
%type	<obj>	object
%type	<obj>	array	array_item_list
%type	<obj>	frame
%type	<obj>	binary	binary_item_list
%type	<obj>	path_expr
%type	<obj>	foreach_operator	deeply


%token  <op>	kBINBG	kBINED	// バイナリデータ（独自拡張）


   /* 予約語 */

%token		kBEGIN	kEND
			kFUNC	kNATIVE		kCALL	kWITH
			kFOR	kFOREACH	kTO		kBY
			kSELF	kINHERITED
			kIF
			kTRY	kONEXCEPTION
			kLOOP	kWHILE	kREPEAT kUNTIL  kDO		kIN
			kGLOBAL	kLOCAL	kCONSTANT
			k3READER		// ３点リーダー（独自拡張）
			kERROR			// エラー

    /* オブジェクト */

%token		<obj>	kSYMBOL		kSTRING		kREGEX
%token		<obj>	kINTEGER	kREAL		kCHARACTER	kMAGICPOINTER
%token		<obj>	kTRUE		kNIL		kUNBIND

// 優先順位

    /* 式 */

%left		kEXPR2

    /* 予約語 */

%left		kBREAK	kRETURN
%left		//kTRY  kONEXCEPTION
			kLOOP	kWHILE	kREPEAT kUNTIL	kDO	kIN
			kGLOBAL	kLOCAL	kCONSTANT
			kSYMBOL
%left		','		//  ';'

    /* IF 文 */

%nonassoc	kTHEN
%nonassoc	kELSE

    /* 演算子 */

/*
表2-5 には、NewtonScript の全演算子の優先順位と、高いものから低いもの
へ、上から下に並べて示す。一緒のグループに入っている演算子は、等価な
優先順位を持つことに注意。

.				スロットアクセス					→
: :?			(条件付)メッセージ送信			→
[]				配列要素						→
-				単項マイナス					→
<< >>			左シフト右シフト					→
* / div mod		乗算、除算、整数除算、余り			→
+ -				加算、減算						→
& &&			文字列合成、文字列スペース入り合成	→
exists			変数・スロットの存在確認			なし
< <= > >= = <>	比較							→
not				論理否定						→
and or			論理AND, 論理OR				→
:=				代入							←
*/

%right		<op>	kASNOP			// 代入							←
%left		<op>	kANDOP, kOROP	// 論理AND, 論理OR				→
%left		<op>	kNOTOP			// 論理否定						→
%left		<op>	kRELOP			// 比較							→
%left		<op>	kEXISTS			// 変数・スロットの存在確認			なし
%left		<op>	kSTROP			// 文字列合成、文字列スペース入り合成	→
%left		<op>	kADDOP			// 加算、減算						→
%left		<op>	kMULOP			// 乗算、除算、整数除算、余り			→
%left		<op>	kSFTOP			// 左シフト右シフト					→
%nonassoc			kUMINUS			// 単項マイナス					→
%left				'[' ']'			// 配列要素						→
%left		<op>	kSNDOP			// (条件付)メッセージ送信			→
					':'
%left				'.'				// スロットアクセス					→

    /* 括弧 */

%left				'(' ')'

%%

input
		: constituent_list  {}
		;

constituent_list
		: /* empty */					{ $$ = kNewtRefUnbind; }
		| constituent					{ $$ = NPSGenNode1(kNPSConstituentList, $1); }
		| constituent_list ';'
//		| constituent ';' constituent_list	// 右再帰
		| constituent_list ';' constituent	// 左再帰
										{ $$ = NPSGenNode2(kNPSConstituentList, $1, $3); }
		| constituent_list error ';'	{ yyerrok; }
		;

constituent
		: expr
		| global_declaration
		;

expr
		: simple_expr
		| compound_expr
		| literal						{ $$ = $1; }
		| constructor
		| lvalue						{ $$ = NPSGenNode1(kNPSLvalue, $1); }
		| lvalue kASNOP expr			{ $$ = NPSGenNode2(kNPSAssign, $1, $3); }
		| kMAGICPOINTER kASNOP expr		{	// マジックポインタの定義（独自拡張）
											ERR_NOS2C("Assign Magic Pointer");	// NOS2 非互換
											$$ = NPSGenNode2(kNPSAssign, $1, $3);
										}
		| exists_expr
		| function_call
		| message_send
		| if_expr
		| iteration

		// break_expr
		| kBREAK						{ $$ = NPSGenNode0(kNPSBreak); }
		| kBREAK expr					{ $$ = NPSGenNode1(kNPSBreak, $2); }

		// try_expr
		| kTRY expr_sequence onexcp_list
										{ $$ = NPSGenNode2(kNPSTry, $2, $3); }

		| local_declaration
		| kCONSTANT c_init_clause_list	{ $$ = NPSGenNode1(kNPSConstant, $2); }

		// return_expr
		| kRETURN						{ $$ = NPSGenNode0(kNPSReturn); }
		| kRETURN expr					{ $$ = NPSGenNode1(kNPSReturn, $2); }
		;

lvalue
		: kSYMBOL						{ $$ = $1; }
		| frame_accessor
		| array_accessor
		;

simple_expr
		// binary_operator
		: expr kADDOP expr	{ $$ = NPSGenOP2($2, $1, $3); }
		| expr kMULOP expr	{ $$ = NPSGenOP2($2, $1, $3); }
		| expr kSFTOP expr	{ $$ = NPSGenOP2($2, $1, $3); }
		| expr kRELOP expr	{ $$ = NPSGenOP2($2, $1, $3); }
		| expr kANDOP expr	{ $$ = NPSGenNode2(kNPSAnd, $1, $3); }
		| expr kOROP expr	{ $$ = NPSGenNode2(kNPSOr, $1, $3); }
		| expr kSTROP expr	{ $$ = NPSGenOP2($2, $1, $3); }

		// unary_operator
		| kADDOP expr %prec kUMINUS
							{
								if ($1 == '-')
								{
									if (NewtRefIsInteger($2))
										$$ = NewtMakeInteger(- NewtRefToInteger($2));
									else
										$$ = NPSGenOP2('-', NewtMakeInteger(0), $2);
								}
								else
								{
									$$ = $2;
								}
							}

		| kNOTOP expr		{ $$ = NPSGenOP1($1, $2); }

		| '(' expr ')'		{ $$ = $2; }
		| kSELF				{ $$ = NPSGenNode0(kNPSPushSelf); }
		;

compound_expr
		: kBEGIN expr_sequence kEND	{ $$ = $2; }
		;

expr_sequence
		: /* empty */				{ $$ = kNewtRefUnbind; }
		| expr
		| expr_sequence ';'
//		| expr ';' expr_sequence	// 右再帰
		| expr_sequence ';' expr	// 左再帰
									{ $$ = NPSGenNode2(kNPSConstituentList, $1, $3); }
		| expr_sequence error ';'   { yyerrok; }
		;

literal
		: simple_literal
		| kSTRING		{ $$ = NPSGenNode1(kNPSClone, $1); }
		| binary		{	// バイナリデータ（独自拡張）
							ERR_NOS2C("Binary Syntax");	// NOS2 非互換
							$$ = NPSGenNode1(kNPSClone, $1);
						}
		| '\'' object   { $$ = NewtPackLiteral($2); }
		;

simple_literal
//		: kSTRING
		: kINTEGER
		| kREAL
		| kCHARACTER
		| kMAGICPOINTER
		| kTRUE			{ $$ = kNewtRefTRUE; }
		| kNIL			{ $$ = kNewtRefNIL; }
		| kUNBIND		{	// #UNBIND （独自拡張）
							ERR_NOS2C("Unbind Ref");	// NOS2 非互換
							$$ = kNewtRefUnbind;
						}
//		| binaryy		{ ERR_NOS2C("Binary Syntax"); }	// バイナリデータ（独自拡張）
		;

object
		: simple_literal
		| kSTRING
		| kSYMBOL				// NewtonScript の構文図ではこれがない（記述漏れ？）
		| path_expr
		| array
		| frame
		| binary		{ ERR_NOS2C("Binary Syntax"); }	// バイナリデータ（独自拡張）
		;

path_expr
		: kSYMBOL '.' kSYMBOL		{ $$ = NPSMakePathExpr($1, $3); }
		| kSYMBOL '.' path_expr		{ $$ = NPSInsertArraySlot($3, 0, $1); }
		;

array
		: '[' kSYMBOL ':' array_item_list ']'	{ $$ = NcSetClass($4, $2); } 
		| '[' array_item_list ']'				{ $$ = $2; }
		;

array_item_list
		: /* empty */					{ $$ = NPSMakeArray(kNewtRefUnbind); }
		| object						{ $$ = NPSMakeArray($1); }
		| array_item_list ','
		| array_item_list ',' object	{ $$ = NPSAddArraySlot($1, $3); }
		;

frame
		: '{' frame_slot_list '}'		{ $$ = $2; }
		;

frame_slot_list
		: /* empty */					{ $$ = NPSMakeFrame(kNewtRefUnbind, kNewtRefUnbind); }
		| kSYMBOL ':' object			{ $$ = NPSMakeFrame($1, $3); }
		| frame_slot_list ','
		| frame_slot_list ',' kSYMBOL ':' object
                                        { $$ = NPSSetSlot($1, $3, $5); }
		;

binary	// バイナリデータ（独自拡張）
		: kBINBG kSYMBOL ':' binary_item_list kBINED	{ $$ = NcSetClass($4, $2); } 
		| kBINBG binary_item_list kBINED				{ $$ = $2; }
		;

binary_item_list
		: /* empty */					{ $$ = NPSMakeBinary(kNewtRefUnbind); }
		| object						{ $$ = NPSMakeBinary($1); }
		| binary_item_list ',' object	{ $$ = NPSAddARef($1, $3); }
		;

constructor
		// 配列の生成
		: '[' kSYMBOL ':' expr_list ']'		{ $$ = NPSGenNode2(kNPSMakeArray, $2, $4); }
		| '[' expr_list ']'					{ $$ = NPSGenNode2(kNPSMakeArray, kNewtRefUnbind, $2); }

		// フレームの生成
		| '{' frame_constructor_list '}'	{ $$ = NPSGenNode1(kNPSMakeFrame, $2); }

		// 関数オブジェクトの生成
		| kFUNC func_keyword '(' formal_argument_list ')' expr %prec kEXPR2
                        { $$ = NPSGenNode2(kNPSFunc, $4, $6); }

		// 正規表現オブジェクトの生成
		| kREGEX kSTRING			{ $$ = NPSGenNode2(kNPSMakeRegex, $1, $2); }
		| kREGEX					{ $$ = NPSGenNode2(kNPSMakeRegex, $1, kNewtRefNIL); }
		;

expr_list
		: /* empty */				{ $$ = kNewtRefUnbind; }
		| expr
		| expr_list ','
		| expr_list ',' expr
									{ $$ = NPSGenNode2(kNPSCommaList, $1, $3); }
		;

frame_constructor_list
		: /* empty */				{ $$ = kNewtRefUnbind; }
		| frame_slot_value
		| frame_constructor_list ','
		| frame_constructor_list ',' frame_slot_value
									{ $$ = NPSGenNode2(kNPSCommaList, $1, $3); }
		;

frame_slot_value
		: kSYMBOL ':' expr	{ $$ = NPSGenNode2(kNPSSlot, $1, $3); }
		;

func_keyword	// 無視(not supported)
		: /* empty */
		| kNATIVE
		;

formal_argument_list
		: /* empty */				{ $$ = kNewtRefUnbind; }
		| formal_argument_list2
		| formal_argument_list2 ',' indefinite_argument
									{ $$ = NPSGenNode2(kNPSCommaList, $1, $3); }
		;

formal_argument_list2
		: formal_argument
		| formal_argument_list2 ',' formal_argument
									{ $$ = NPSGenNode2(kNPSCommaList, $1, $3); }
		;

formal_argument
		: kSYMBOL					{ $$ = $1; }
		| type kSYMBOL				{ $$ = NPSGenNode2(kNPSArg, $1, $2); }		// type は無視(not supported)
		;

indefinite_argument
		: kSYMBOL k3READER			{	// 不定長（独自拡張）
										ERR_NOS2C("Indefinite Argument");	// NOS2 非互換
										$$ = NPSGenNode1(kNPSIndefinite, $1);
									}
		;

type
		: kSYMBOL					{ TYPECHECK($1); $$ = $1; }
		;

frame_accessor
		: expr '.' kSYMBOL			{ $$ = NPSGenNode2(kNPSGetPath, $1, $3); }
		| expr '.' '(' expr ')'		{ $$ = NPSGenNode2(kNPSGetPath, $1, $4); }
		;

array_accessor
		: expr '[' expr ']'			{ $$ = NPSGenNode2(kNPSAref, $1, $3); }
		;

exists_expr
		: kSYMBOL kEXISTS			{ $$ = NPSGenNode1(kNPSExists, $1); }
		| frame_accessor kEXISTS	{ $$ = NPSGenNode1(kNPSExists, $1); }
		| ':' kSYMBOL kEXISTS		{ $$ = NPSGenNode2(kNPSMethodExists, kNewtRefUnbind, $2); }
		| expr ':' kSYMBOL kEXISTS	{ $$ = NPSGenNode2(kNPSMethodExists, $1, $3); }
		;

function_call
		: kSYMBOL '(' actual_argument_list ')'	{ $$ = NPSGenNode2(kNPSCall, $1, $3); }
		| kCALL expr kWITH '(' actual_argument_list ')'
												{ $$ = NPSGenNode2(kNPSInvoke, $2, $5); }
		;

actual_argument_list
		: /* emtpry */							{ $$ = kNewtRefUnbind; }
		| expr
		| actual_argument_list ',' expr	{ $$ = NPSGenNode2(kNPSCommaList, $1, $3); }
		;

message_send
		: ':' kSYMBOL '(' actual_argument_list ')'
                                    { $$ = NPSGenSend(kNewtRefUnbind, $1, $2, $4); }
		| kSNDOP kSYMBOL '(' actual_argument_list ')'
                                    { $$ = NPSGenSend(kNewtRefUnbind, $1, $2, $4); }

		| expr ':' kSYMBOL '(' actual_argument_list ')'
                                    { $$ = NPSGenSend($1, ':', $3, $5); }
		| expr kSNDOP kSYMBOL '(' actual_argument_list ')'
                                    { $$ = NPSGenSend($1, $2, $3, $5); }

		| kINHERITED ':' kSYMBOL '(' actual_argument_list ')'
                                    { $$ = NPSGenResend(':', $3, $5); }
		| kINHERITED kSNDOP kSYMBOL '(' actual_argument_list ')'
                                    { $$ = NPSGenResend($2, $3, $5); }
		;

if_expr
		: kIF expr kTHEN expr					{ $$ = NPSGenIfThenElse($2, $4, kNewtRefUnbind); }
		| kIF expr kTHEN expr kELSE expr		{ $$ = NPSGenIfThenElse($2, $4, $6); }
//		| kIF expr kTHEN expr ';' kELSE expr	{ $$ = NPSGenIfThenElse($2, $4, $7); }
		;

iteration
		: kLOOP expr						{ $$ = NPSGenNode1(kNPSLoop, $2); }
		| kFOR kSYMBOL kASNOP expr kTO expr by_expr kDO expr
											{ $$ = NPSGenForLoop($2, $4, $6, $7, $9); }
		| kFOREACH kSYMBOL deeply kIN expr foreach_operator expr
											{ $$ = NPSGenForeach(kNewtRefUnbind, $2, $5, $3, $6, $7); }
		| kFOREACH kSYMBOL ',' kSYMBOL deeply kIN expr foreach_operator expr
											{ $$ = NPSGenForeach($2, $4, $7, $5, $8, $9); }
		| kWHILE expr kDO expr				{ $$ = NPSGenNode2(kNPSWhile, $2, $4); }
		| kREPEAT expr_sequence kUNTIL expr { $$ = NPSGenNode2(kNPSRepeat, $2, $4); }
		;

by_expr
		: /* emtpry */		{ $$ = kNewtRefUnbind; }
		| kBY expr			{ $$ = $2; }
		;

foreach_operator
		: kDO				{ $$ = kNewtRefNIL; }
		| kSYMBOL			{ SYMCHECK($1, NSSYM0(collect)); $$ = $1; }		// collect
		;

deeply
		: /* emtpry */		{ $$ = kNewtRefNIL; }
		| kSYMBOL			{ SYMCHECK($1, NSSYM0(deeply)); $$ = kNewtRefTRUE; }	// deeply
		;

onexcp_list
		: onexcp
/*
		| onexcp ';' onexcp_list	{ $$ = NPSGenNode2(kNPSOnexceptionList, $1, $3); }
		| onexcp onexcp_list		{ $$ = NPSGenNode2(kNPSOnexceptionList, $1, $2); }
*/
//		| onexcp_list ';' onexcp	{ $$ = NPSGenNode2(kNPSOnexceptionList, $1, $3); }
		| onexcp_list onexcp		{ $$ = NPSGenNode2(kNPSOnexceptionList, $1, $2); }
		;

onexcp
		: kONEXCEPTION kSYMBOL kDO expr { $$ = NPSGenNode2(kNPSOnexception, $2, $4); }
		;

local_declaration
		: kLOCAL l_init_clause_list  { $$ = NPSGenNode2(kNPSLocal, kNewtRefUnbind, $2); }
		| kLOCAL kSYMBOL l_init_clause_list	// type は無視(not supported)
			{
				TYPECHECK($2);
				$$ = NPSGenNode2(kNPSLocal, $2, $3);
			}
/*
		// これだとコンフリクトが発生する
		| kLOCAL type l_init_clause_list	// type は無視(not supported)
									{ $$ = NPSGenNode2(kNPSLocal, $2, $3); }
*/
		;

l_init_clause_list
		: init_clause
//		| init_clause ',' l_init_clause_list	// 右再帰
		| l_init_clause_list ',' init_clause	// 左再帰
									{ $$ = NPSGenNode2(kNPSCommaList, $1, $3); }
		;

init_clause
		: kSYMBOL					{ $$ = $1; }
		| kSYMBOL kASNOP expr		{ $$ = NPSGenNode2(kNPSAssign, $1, $3); }
		;

c_init_clause_list
		: c_init_clause
		| c_init_clause_list ',' c_init_clause	// 左再帰
									{ $$ = NPSGenNode2(kNPSCommaList, $1, $3); }
		;

c_init_clause
		: kSYMBOL kASNOP expr		{ $$ = NPSGenNode2(kNPSAssign, $1, $3); }
		;

global_declaration
		: kGLOBAL init_clause		{ $$ = NPSGenNode1(kNPSGlobal, $2); }
		| global_function_decl
		;

global_function_decl
		: kGLOBAL function_decl		{ $$ = $2; }
		| kFUNC function_decl		{ $$ = $2; }
		;

function_decl
		: kSYMBOL '(' formal_argument_list ')' expr { $$ = NPSGenGlobalFn($1, $3, $5); }
		;

%%
