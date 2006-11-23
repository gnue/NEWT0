/*------------------------------------------------------------------------*/
/**
 * @file	yacc.h
 * @brief   yacc 関連の宣言
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	YACC_H
#define	YACC_H

/* グローバル変数 */

/// パーサの入力ファイル
extern FILE *	yyin;


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


extern int		yylex();
extern void		yyerror(char * s);
extern int		yyparse();


#ifdef __cplusplus
}
#endif


#endif /* YACC_H */
