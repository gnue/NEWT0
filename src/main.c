/*------------------------------------------------------------------------*/
/**
 * @file	main.c
 * \~japanese @brief メイン関数（CUI コマンド）
 * \~english  @brief the main function (CUI command)
 * \~
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "NewtCore.h"
#include "NewtBC.h"
#include "NewtVM.h"
#include "NewtParser.h"
#include "lookup_words.h"
#include "yacc.h"
#include "version.h"


/* 定数 */

/// オプション
enum {
    optNone			= 0,
    optNos2,
    optCopyright,
    optVersion,
    optStaff,
};


/* ローカル変数 */

/// オプションキーワードのルックアップテーブル
static keyword_t	reserved_words[] = {
        // アルファベット順にソートしておくこと
        {"copyright",	optCopyright},
        {"newton",		optNos2},
        {"nos2",		optNos2},
        {"staff",		optStaff},
        {"version",		optVersion},
    };


/// 作業ディレクトリ
static const char *		newt_currdir;


/* 関数プロトタイプ */
#ifdef __cplusplus
extern "C" {
#endif


int main (int argc, const char * argv[]);


#ifdef __cplusplus
}
#endif


static void		newt_result_message(newtRefArg r, newtErr err);

static newtErr  newt_info(int argc, const char * argv[], int n);
static newtErr  newt_interpret_str(int argc, const char * argv[], int n);
static newtErr  newt_interpret_file(int argc, const char * argv[], int n);

static void		newt_chdir(void);

static void		newt_show_copyright(void);
static void		newt_show_version(void);
static void		newt_show_staff(void);
static void		newt_show_usage(void);

static void		newt_invalid_option(char c);
static void		newt_option_switchs(const char * s);
static void		newt_option(const char * s);
static newtErr  newt_option_with_arg(char c, int argc, const char * argv[], int n);


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 
 * \~japanese
 * 結果を表示
 * @param r			[in] オブジェクト
 * @param err		[in] エラーコード
 * @return			なし
 
 * \~english
 * Show message for this error code
 * @param r [in] object 
 * @param err [in] error code 
 * @return no return value  
 
 * \~ 
 */

void newt_result_message(newtRefArg r, newtErr err)
{
    if (err != kNErrNone)
        NewtErrMessage(err);
    else if (NEWT_DEBUG)
        NsPrint(kNewtRefNIL, r);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** コマンドライン引数で指定された関数の情報を表示
 *
 * @param argc		[in] コマンドライン引数の数
 * @param argv		[in] コマンドライン引数の配列
 * @param n			[in] コマンドライン引数の位置
 *
 * @return			エラーコード
 */

newtErr newt_info(int argc, const char * argv[], int n)
{
    newtErr	err = kNErrNone;
    int		i;

    NewtInit(argc, argv, argc);
	newt_chdir();

    if (n < argc)
    {
        for (i = n; i < argc; i++)
        {
            err = NVMInfo(argv[i]);
            NewtErrMessage(err);
        }
    }
    else
    {   //引数がない場合は関数一覧を表示
        err = NVMInfo(NULL);
        NewtErrMessage(err);
    }

    NewtCleanup();

    return err;
}


/*------------------------------------------------------------------------*/
/** コマンドライン引数で指定された文字列をインタプリタ実行する
 *
 * @param argc		[in] コマンドライン引数の数
 * @param argv		[in] コマンドライン引数の配列
 * @param n			[in] コマンドライン引数の位置
 *
 * @return			エラーコード
 */

newtErr newt_interpret_str(int argc, const char * argv[], int n)
{
    newtRefVar	result;
    newtErr	err;

    NewtInit(argc, argv, n + 1);
	newt_chdir();
    result = NVMInterpretStr(argv[n], &err);
    newt_result_message(result, err);
    NewtCleanup();

    return err;
}


/*------------------------------------------------------------------------*/
/** ファイルをインタプリタ実行する
 *
 * @param argc		[in] コマンドライン引数の数
 * @param argv		[in] コマンドライン引数の配列
 * @param n			[in] コマンドライン引数の位置
 *
 * @return			エラーコード
 */

newtErr newt_interpret_file(int argc, const char * argv[], int n)
{
    const char * path = NULL;
    newtRefVar	result;
    newtErr	err;

    if (n < argc)
    {
        path = argv[n];
		n++;
    }

    NewtInit(argc, argv, n);
	newt_chdir();
    result = NVMInterpretFile(path, &err);
    newt_result_message(result, err);
    NewtCleanup();

    return err;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 作業ディレクトリを変更 */
void newt_chdir(void)
{
#ifdef HAVE_CHDIR 
	if (newt_currdir != NULL)
		chdir(newt_currdir);
#endif /* HAVE_CHDIR */
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** コピーライトを表示 */
void newt_show_copyright(void)
{
    fprintf(stderr, "%s - %s\n", NEWT_NAME, NEWT_COPYRIGHT);
}


/** バージョン情報を表示 */
void newt_show_version(void)
{
    fprintf(stderr, "%s%s %s(%s)\n", NEWT_NAME, NEWT_PROTO,
                NEWT_VERSION, NEWT_BUILD);
}


/** スタッフ情報を表示 */
void newt_show_staff(void)
{
    fprintf(stderr, "%s%s %s\n\n%s\n", NEWT_NAME, NEWT_PROTO, NEWT_VERSION,
                NEWT_STAFF);
}


/** 使用法を表示 */
void newt_show_usage(void)
{
    fprintf(stderr, "Usage: %s %s\n%s", NEWT_NAME, NEWT_PARAMS, NEWT_USAGE);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** オプションエラーを表示
 *
 * @param c			[in] オプション文字
 *
 * @return			なし
 */

void newt_invalid_option(char c)
{
    fprintf(stderr, "invalid option -%c (-h will show valid options)\n", c);
}


/*------------------------------------------------------------------------*/
/** オプションスイッチの解析
 *
 * @param s			[in] オプションスイッチ
 *
 * @return			なし
 */

void newt_option_switchs(const char * s)
{
    while (*s)
    {
        switch (*s)
        {
            case 'd':
                NEWT_DEBUG = true;
                break;

            case 't':
                NEWT_TRACE = true;
                break;

            case 'l':
                NEWT_DUMPLEX = true;
                break;

            case 's':
                NEWT_DUMPSYNTAX = true;
                break;

            case 'b':
                NEWT_DUMPBC = true;
                break;

            case 'I':
                NEWT_INDENT = 1;
                break;

            case 'v':
                newt_show_version();
                exit(0);
                break;

            case 'h':
                newt_show_usage();
                exit(0);
                break;

            default:
                newt_invalid_option(*s);
                exit(0);
                break;
        }

        s++;
    }
}


/*------------------------------------------------------------------------*/
/** オプション文字列の解析
 *
 * @param s			[in] オプション文字列
 *
 * @return			なし
 */

void newt_option(const char * s)
{
    int	wlen;

    wlen = sizeof(reserved_words) / sizeof(keyword_t);

    switch (lookup_words(reserved_words, wlen, s))
    {
		// NOS2 コンパチブル
		case optNos2:
			NEWT_MODE_NOS2 = true;
			break;

        // コピーライト
        case optCopyright:
            newt_show_copyright();
            exit(0);
            break;

        // バージョン
        case optVersion:
            newt_show_version();
            exit(0);
            break;

        // スタッフロール
        case optStaff:
            newt_show_staff();
            exit(0);
            break;
    }
}


/*------------------------------------------------------------------------*/
/** オプションの引数を解析する
 *
 * @param c			[in] オプション文字
 * @param argc		[in] コマンドライン引数の数
 * @param argv		[in] コマンドライン引数の配列
 * @param n			[in] コマンドライン引数の位置
 *
 * @return			エラーコード
 */

newtErr newt_option_with_arg(char c, int argc, const char * argv[], int n)
{
    newtErr	err = kNErrNone;

    switch (c)
    {
        case 'i':
            err = newt_info(argc, argv, n);
            break;

        case 'e':
            if (n < argc)
                err = newt_interpret_str(argc, argv, n);
            break;
    }

    return err;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** main 関数（CUI コマンド)
 *
 * @brief NewtonScript インタプリタ
 *
 * @param argc		[in] コマンドライン引数の数
 * @param argv		[in] コマンドライン引数の配列
 *
 * @return			エラーコード
 */

int main (int argc, const char * argv[])
{
    const char *	s;
    newtErr	err = kNErrNone;
    int		i;

    for (i = 1; i < argc; i++)
    {
        s = argv[i];

        if (strlen(s) == 0) break;
        if (*s != '-') break;

        s++;

        if (*s == '-')
        {
            newt_option(s + 1);
            i++;
            break;
        }

        switch (*s)
        {
            case 'C':
                i++;
				newt_currdir = argv[i];
                break;

            case 'i':
            case 'e':
                i++;
                err = newt_option_with_arg(*s, argc, argv, i);
                exit(err);
                break;

            default:
                newt_option_switchs(s);
                break;
        }
    }

    err = newt_interpret_file(argc, argv, i);

    return err;
}


/*------------------------------------------------------------------------*/
/** パーサエラー
 *
 * @param s			[in] エラーメッセージ文字列
 *
 * @return			なし
 */

void yyerror(char * s)
{
	if (s[0] && s[1] == ':')
		NPSErrorStr(*s, s + 2);
	else
		NPSErrorStr('E', s);
}
