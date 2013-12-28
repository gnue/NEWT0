/*------------------------------------------------------------------------*/
/**
 * @file	newt.l
 * @brief   字句解析
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */

/*
%option interactive
*/

%option noyywrap

%{

/* ヘッダファイル */
#include <string.h>

#include "NewtErrs.h"
#include "NewtObj.h"
#include "NewtEnv.h"
#include "NewtParser.h"
#include "NewtPrint.h"
#include "NewtIO.h"

#include "lookup_words.h"
#include "yacc.h"
#include "y.tab.h"


/* 関数プロトタイプ */
static int			lex_update_tokenpos(int op);
static void			lex_newline(void);

static char *		lex_truncstr(char *s);
static char *		lex_escapestr(char *s);
static char			lex_escapechar(char c);
static int			lex_htoi(char *s, int len);
static uint32_t		lex_getop(char *s);
static int			lex_op(char *s, int op);
static int			lex_reserveword(int tokn, char *s);

static int			lex_makesymbol(char *s, bool trunc);
static int			lex_makestring(char *s, bool trunc);
static int			lex_makeregex(char *s, bool trunc);
static int			lex_makechar(int c);
static int			lex_makeinteger(int n);
static int			lex_makereal(double n);

static int			lex_makemagicpointer(int n);
static int			lex_makenamedmp(char *s, bool trunc);

static void			lex_debug(int type);


/* マクロ */
#undef YY_INPUT
#define YY_INPUT(b, r, ms) (r=nps_yyinput(yyin, b, ms))

#define WORN_NOS2(msg)		if (NEWT_MODE_NOS2) NPSErrorStr('W', msg);

%}

%x COMMENT
%x SHELL
%s BINARY
%x STRING
%x REGEX

ALPHA					[A-Za-z]
DIGIT					[0-9]
HEX-DIGIT				({DIGIT}|[A-Fa-f])
REAL					{DIGIT}+"."{DIGIT}*([eE]-?{DIGIT}+)?

NEWLINE					("\r\n"|"\r"|"\n")
SPACE					[ \t]
STRING-CHARACTER		[\x20-\x21\x23-\x5b\x5d-\x7f]
SYMBOL-CHARACTER		[\x20-\x5b\x5d-\x7b\x7d-\x7f]
REGEX-CHARACTER			({STRING-CHARACTER}|"\"")
NON-ESCAPE-CHARACTER	[\x20-\x5b\x5d-\x7f]
ESCAPE-CHAR				("\""|[\\nrt/])
ESCAPE-SEQUENCE			(("\\"{ESCAPE-CHAR})|("\\u"({HEX-DIGIT}{4})*"\\u"))
TRUNCATED-ESCAPE		"\\u"({HEX-DIGIT}{4})*

CHARACTER-SEQUENCE		({STRING-CHARACTER}|{ESCAPE-SEQUENCE})*{TRUNCATED-ESCAPE}?
SYMBOL					({ALPHA}|"_")({ALPHA}|{DIGIT}|"_")*
SYMBOL-SEQUENCE			("\\"[|\\]|{SYMBOL-CHARACTER})*
REGEX-SEQUENCE			([^*/]|{ESCAPE-SEQUENCE})({REGEX-CHARACTER}|{ESCAPE-SEQUENCE})*{TRUNCATED-ESCAPE}?

ELSE					[Ee][Ll][Ss][Ee]

%%

%{
    if (nps_env.first_time)
    {   //初回起動時
        nps_env.first_time = false;
        BEGIN SHELL;
    }
%}


"//".*$

"/*"							{
									BEGIN COMMENT;
									lex_update_tokenpos(0);
								}

<COMMENT>"*/"					{
									BEGIN INITIAL;
									lex_update_tokenpos(0);
								}

<COMMENT>{NEWLINE}				lex_newline();
<COMMENT>(.|{SPACE})			lex_update_tokenpos(0);

"<#"							{
									BEGIN BINARY;
									return lex_update_tokenpos(kBINBG);
								}

<BINARY>{NEWLINE}				lex_newline();
<BINARY>">"						{
									BEGIN INITIAL;
									return lex_update_tokenpos(kBINED);
								}

<STRING>{NEWLINE}				{
									nps_env.lineno++;
									yymore();
								}

<STRING>{CHARACTER-SEQUENCE}	yymore();
<STRING>"\""					{
									BEGIN INITIAL;
									return lex_makestring(yytext, true);
								}

<REGEX>{ALPHA}+					{ return lex_makestring(yytext, false); }
<REGEX>.						{
									unput(*yytext);
									BEGIN INITIAL;
								}

{NEWLINE}						lex_newline();
{SPACE}+						lex_update_tokenpos(0);

__FILE__						{ return lex_makestring((char *)(nps_env.fname?nps_env.fname:"-"), false); }
__LINE__						{ return lex_makeinteger(nps_env.lineno); }

[+-]							{ return lex_op(yytext, kADDOP); }
[*/]							{ return lex_op(yytext, kMULOP); }
("&"|"&&")						{ return lex_op(yytext, kSTROP); }
("<<"|">>")						{ return lex_op(yytext, kSFTOP); }
([<>]"="?|"="|"<>"|"==")		{ return lex_op(yytext, kRELOP); }
"..."							{ return lex_update_tokenpos(k3READER); }

":="							{ return lex_update_tokenpos(kASNOP); }
":?"							{ return lex_update_tokenpos(kSNDOP); }

"\""{CHARACTER-SEQUENCE}"\""	{ return lex_makestring(yytext, true); }

"\""							{
									BEGIN STRING;
									yymore();
								}

{SYMBOL}						{ return lex_makesymbol(yytext, false); }
"|"{SYMBOL-SEQUENCE}"|"			{ return lex_makesymbol(yytext, true); }

"/"{REGEX-SEQUENCE}"/"			{
									BEGIN REGEX;
									return lex_makeregex(yytext, true);
								}

"$\\"{ESCAPE-CHAR}				{ return lex_makechar(lex_escapechar(yytext[2])); }
"$\\"{HEX-DIGIT}{2}				{ return lex_makechar(lex_htoi(yytext + 2, -1)); }
"$\\u"{HEX-DIGIT}{4}			{ return lex_makechar(lex_htoi(yytext + 3, -1)); }
"$"{NON-ESCAPE-CHARACTER}		{ return lex_makechar(yytext[1]); }

"@"{DIGIT}+						{ return lex_makemagicpointer(atoi(yytext + 1)); }
"@"{SYMBOL}						{ return lex_makenamedmp(yytext + 1, false); }
"@""|"{SYMBOL-SEQUENCE}"|"		{ return lex_makenamedmp(yytext + 1, true); }

{DIGIT}+						{ return lex_makeinteger(atoi(yytext)); }
"0x"{HEX-DIGIT}+				{ return lex_makeinteger(lex_htoi(yytext + 2, -1)); }

{REAL}							{ return lex_makereal(atof(yytext)); }

";"({NEWLINE}|{SPACE})*{ELSE}	{ return lex_update_tokenpos(kELSE); }

.								{ return lex_update_tokenpos(yytext[0]); }

<SHELL>^"#".*$					{ BEGIN INITIAL; }
<SHELL>{NEWLINE}				lex_newline();
<SHELL>.						{
									unput(*yytext);
									BEGIN INITIAL;
								}

%%


/// 一部の lex で YY_FLUSH_BUFFER が undef されてしまう問題に対応
#ifndef YY_FLUSH_BUFFER
	#define YY_FLUSH_BUFFER yy_flush_buffer(YY_CURRENT_BUFFER )
#endif


/// 予約語
static keyword_t	reserved_words[] = {
		// アルファベット順にソートしておくこと
		{"AND",			kANDOP},		// 予約語
        {"BEGIN",		kBEGIN},		// 予約語
        {"BREAK",		kBREAK},		// 予約語
        {"BY",			kBY},			// 予約語
        {"CALL",		kCALL},			// 予約語
//		{"COLLECT",		kCOLLECT},
        {"CONSTANT",	kCONSTANT},		// 予約語
//		{"DEEPLY",		kDEEPLY},
        {"DIV",			kMULOP},		// 予約語
        {"DO",			kDO},			// 予約語
        {"ELSE",		kELSE},			// 予約語
        {"END",			kEND},			// 予約語
        {"EXISTS",		kEXISTS},		// 予約語
        {"FOR",			kFOR},			// 予約語
        {"FOREACH",		kFOREACH},		// 予約語
        {"FUNC",		kFUNC},			// 予約語
        {"GLOBAL",		kGLOBAL},		// 予約語
        {"IF",			kIF},			// 予約語
        {"IN",			kIN},			// 予約語
        {"INHERITED",	kINHERITED},	// 予約語
        {"LOCAL",		kLOCAL},		// 予約語
        {"LOOP",		kLOOP},			// 予約語
        {"MOD",			kMULOP},		// 予約語
        {"NATIVE",		kNATIVE},		// 予約語
        {"NIL",			kNIL},
        {"NOT",			kNOTOP},		// 予約語
        {"ONEXCEPTION",	kONEXCEPTION},	// 予約語
        {"OR",			kOROP},			// 予約語
        {"REPEAT",		kREPEAT},		// 予約語
        {"RETURN",		kRETURN},		// 予約語
        {"SELF",		kSELF},			// 予約語
        {"THEN",		kTHEN},			// 予約語
        {"TO",			kTO},			// 予約語
        {"TRUE",		kTRUE},
        {"TRY",			kTRY},			// 予約語
        {"UNTIL",		kUNTIL},		// 予約語
        {"WHILE",		kWHILE},		// 予約語
        {"WITH",		kWITH},			// 予約語
    };

/// オペレータ
static keyword_t	operator_words[] = {
		// アルファベット順にソートしておくこと
        {"&&",			kNPS_CONCAT2},
        {"<<",			kNPS_SHIFT_LEFT},
        {"<=",			kNPS_LESS_EQUAL},
        {"<>",			kNPS_NOT_EQUAL},
        {"==",			kNPS_OBJECT_EQUAL},
        {">=",			kNPS_GREATER_EQUAL},
        {">>",			kNPS_SHIFT_RIGHT},
	};


/*------------------------------------------------------------------------*/
/** トークンの位置を更新
 *
 * @param op		[in] オペレータ
 *
 * @return			オペレータをそのまま返す
 */

int lex_update_tokenpos(int op)
{
	strncat(nps_env.linebuf, yytext, sizeof(nps_env.linebuf));

	nps_env.tokenpos += yyleng;
	nps_env.yyleng = yyleng;
	return op;
}


/*------------------------------------------------------------------------*/
/** 新しい行の初期化
 *
 * @return			なし
 *
 * @note			エラー箇所を適切に表示するために入力行を保持する
 */

void lex_newline(void)
{
	uint32_t	n = 1;

	if (yytext[0] == '\r' && yytext[1] == '\n')
		n++;

	nps_env.linebuf[0] = '\0';
	nps_env.tokenpos = 0;
	nps_env.yyleng = 0;
	nps_env.lineno++;
	yyless(n);
}


/*------------------------------------------------------------------------*/
/** 文字列の前後１文字を切り詰める
 *
 * @param s			[in] 文字列
 *
 * @return			切り詰めたられた文字列
 */

char * lex_truncstr(char *s)
{
    s[strlen(s) - 1] = '\0';

    return s + 1;
}


/*------------------------------------------------------------------------*/
/** エスケープされた文字列を処理する
 *
 * @param s			[in] エスケープされた文字列
 *
 * @return			文字列
 */

char * lex_escapestr(char *s)
{
    int		uni = 0;
    char *	src;
    char *	dst;

    dst = src = s;

    while (*src)
    {
        if (*src =='\\')
        {
            src++;

            if (*src == 'u')
            {
                uni = ! uni;
            }
            else
            {
              	*dst = lex_escapechar(*src);
                dst++;
            }

            src++;
        }
        else if (uni)
        {
/*
            *dst = lex_htoi(src, 4);
            src += 4;
            dst++;
*/
            *dst = lex_htoi(src, 2);
            src += 2;
            dst++;

            *dst = lex_htoi(src, 2);
            src += 2;
            dst++;
        }
        else
        {
            if (dst != src)
                *dst = *src;

            dst++;
            src++;
        }
    }

    *dst = '\0';

    return s;
}


/*------------------------------------------------------------------------*/
/** エスケープ文字を処理する
 *
 * @param c			[in] エスケープ文字
 *
 * @return			文字
 */

char lex_escapechar(char c)
{
    switch (c)
    {
        case 'n':
            c = '\n';
            break;

        case 'r':
            c = '\r';
            break;

        case 't':
            c = '\t';
            break;
    }

    return c;
}


/*------------------------------------------------------------------------*/
/** 16進数文字列を整数に変換
 *
 * @param s			[in] 16進数文字列
 * @param len		[in] 16進数文字列の長さ
 *
 * @return			整数
 */

int lex_htoi(char *s, int len)
{
    int	n = 0;

    if (len < 0)
        len = strlen(s) + 1 + len;

    while (0 < len)
    {
        n *= 16;

        if ('A' <= *s && *s <= 'F')
            n += *s - 'A' + 10;
        else if ('a' <= *s && *s <= 'f')
            n += *s - 'a' + 10;
        else
            n += *s - '0';

        s++;
        len--;
    }

    return n;
}


/*------------------------------------------------------------------------*/
/** オペレータ文字列からオペレータIDを求める
 *
 * @param s			[in] オペレータ文字列
 *
 * @return			オペレータID
 */

uint32_t lex_getop(char *s)
{
    uint32_t	op;
    int	len;

	len = strlen(s);

    if (len == 1)
    {
        op = s[0];
    }
    else
    {
        int	wlen;

        wlen = sizeof(operator_words) / sizeof(keyword_t);
        op = lookup_words(operator_words, wlen, s);
    }

    return op;
}


/*------------------------------------------------------------------------*/
/** オペレータトークンの作成
 *
 * @param s			[in] オペレータ文字列
 *
 * @return			トークンID
 */

int lex_op(char *s, int op)
{
    yylval.op = lex_getop(yytext);
	return lex_update_tokenpos(op);
}


/*------------------------------------------------------------------------*/
/** 予約語のトークンを作成
 *
 * @param tokn		[in] トークンID
 * @param s			[in] オペレータ文字列
 *
 * @return			トークンID
 */

int lex_reserveword(int tokn, char *s)
{
    switch (tokn)
    {
        case kMULOP:
            if (strcasecmp(s, "div") == 0)
                yylval.op = kNPS_DIV;
            else if (strcasecmp(s, "mod") == 0)
                yylval.op = kNPS_MOD;
            break;

        case kNOTOP:
            yylval.op = kNPS_NOT;
            break;
    }

    return tokn;
}


/*------------------------------------------------------------------------*/
/** シンボルトークンの作成
 *
 * @param s			[in] 文字列
 * @param trunc		[in] 前後の切り詰め
 *
 * @return			トークンID
 */

int lex_makesymbol(char *s, bool trunc)
{
    if (trunc)
    {
        s = lex_escapestr(lex_truncstr(s));
    }
    else
    {
        int	tokn;
        int	wlen;

        wlen = sizeof(reserved_words) / sizeof(keyword_t);
        tokn = lookup_words(reserved_words, wlen, s);

        if (tokn != -1)
            return lex_update_tokenpos(lex_reserveword(tokn, s));
    }

    yylval.obj = NewtMakeSymbol(s);
    lex_debug(kSYMBOL);

	return lex_update_tokenpos(kSYMBOL);
}


/*------------------------------------------------------------------------*/
/** 文字列トークンの作成
 *
 * @param s			[in] 文字列
 * @param trunc		[in] 前後の切り詰め
 *
 * @return			トークンID
 */

int lex_makestring(char *s, bool trunc)
{
    if (trunc)
        s = lex_escapestr(lex_truncstr(s));

    yylval.obj = NewtMakeString(s, true);
    lex_debug(kSTRING);

	return lex_update_tokenpos(kSTRING);
}


/*------------------------------------------------------------------------*/
/** 正規表現トークンの作成
 *
 * @param s			[in] 文字列
 * @param trunc		[in] 前後の切り詰め
 *
 * @return			トークンID
 */

int lex_makeregex(char *s, bool trunc)
{
    if (trunc)
        s = lex_escapestr(lex_truncstr(s));

    yylval.obj = NewtMakeString(s, true);
    lex_debug(kREGEX);

	return lex_update_tokenpos(kREGEX);
}


/*------------------------------------------------------------------------*/
/** 文字トークンの作成
 *
 * @param c			[in] 文字
 *
 * @return			トークンID
 */

int lex_makechar(int c)
{
    yylval.obj = NewtMakeCharacter(c);
    lex_debug(kCHARACTER);

	return lex_update_tokenpos(kCHARACTER);
}


/*------------------------------------------------------------------------*/
/** 整数トークンの作成
 *
 * @param n			[in] 整数
 *
 * @return			トークンID
 */

int lex_makeinteger(int n)
{
    yylval.obj = NewtMakeInteger(n);
    lex_debug(kINTEGER);

	return lex_update_tokenpos(kINTEGER);
}


/*------------------------------------------------------------------------*/
/** 浮動小数点トークンの作成
 *
 * @param n			[in] 浮動小数点
 *
 * @return			トークンID
 */

int lex_makereal(double n)
{
    yylval.obj = NewtMakeReal(n);
    lex_debug(kREAL);

	return lex_update_tokenpos(kREAL);
}


#ifdef __NAMED_MAGIC_POINTER__

/*------------------------------------------------------------------------*/
/** 名前付マジックポインタの作成
 *
 * @param s			[in] 文字列
 * @param trunc		[in] 前後の切り詰め
 *
 * @return			トークンID
 */

int lex_makenamedmp(char *s, bool trunc)
{
	int		pos;

    if (trunc)
    {
        s = lex_escapestr(lex_truncstr(s));
    }

    yylval.obj = NewtMakeNamedMP(s);
    lex_debug(kMAGICPOINTER);

	pos = lex_update_tokenpos(kMAGICPOINTER);

	// NOS2 非互換
	WORN_NOS2("Named Magic Pinter");

	return pos;
}

#else

/*------------------------------------------------------------------------*/
/** 名前付マジックポインタの作成
 *
 * @param s			[in] 文字列
 * @param trunc		[in] 前後の切り詰め
 *
 * @return			トークンID
 */

int lex_makenamedmp(char *s, bool trunc)
{
	return lex_update_tokenpos(kERROR);
}

#endif


/*------------------------------------------------------------------------*/
/** マジックポインタトークンの作成
 *
 * @param n			[in] マジックポインタ番号
 *
 * @return			トークンID
 */

int lex_makemagicpointer(int n)
{
	if (0x03ff < n)
	{	// 数値が範囲外
		return lex_update_tokenpos(kERROR);
	}

	yylval.obj = NewtMakeMagicPointer(0, n);
    lex_debug(kMAGICPOINTER);

	return lex_update_tokenpos(kMAGICPOINTER);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** デバッグ表示
 *
 * @param type		[in] トークンID
 *
 * @return			なし
 */

void lex_debug(int type)
{
    char *	name = "unknown";

    if (! NEWT_DUMPLEX)
        return;

    switch (type)
    {
        case kSYMBOL:
            name = "symbol";
            break;

        case kSTRING:
            name = "string";
            break;

        case kINTEGER:
            name = "integer";
            break;

        case kREAL:
            name = "real";
            break;

		case kMAGICPOINTER:
            name = "magic pointer";
			break;
    }

    NewtFprintf(stderr, "  %s\t==> ", name);
    NewtPrintObject(stderr, yylval.obj);
}


#if 0
#pragma mark -
#endif
//--------------------------------------------------------------------------
/** 字句解析のための初期化
 *
 * @return			なし
 */

void nps_yyinit(void)
{
	static bool inited = 0; // was "false"

	if (! inited)
	{   //単語テーブルのソート
		lookup_sorttable(reserved_words, sizeof(reserved_words) / sizeof(keyword_t));
		lookup_sorttable(operator_words, sizeof(operator_words) / sizeof(keyword_t));

		inited = true;
	}
}


//--------------------------------------------------------------------------
/** 字句解析の後始末
 *
 * @return			なし
 */

int nps_yycleanup(void)
{
	BEGIN INITIAL;
	YY_FLUSH_BUFFER;
  memset(nps_env.linebuf, '\0', sizeof(nps_env.linebuf));
	yyterminate();
}
