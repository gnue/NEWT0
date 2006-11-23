/*------------------------------------------------------------------------*/
/**
 * @file	lookup_words.h
 * @brief   単語の検索
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	LOOKUP_WORDS_H
#define	LOOKUP_WORDS_H


/* 型宣言 */

/// 単語テーブルの要素構造体
typedef struct {
    char *	name;   ///< 単語
    int		tokn;   ///< トークンID
} keyword_t;


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


int		lookup_words(keyword_t words[], int len, const char * s);
void	lookup_sorttable(keyword_t words[], int len);


#ifdef __cplusplus
}
#endif


#endif /* LOOKUP_WORDS_H */
