/*------------------------------------------------------------------------*/
/**
 * @file	lookup_words.c
 * @brief   単語の検索
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "lookup_words.h"


/*------------------------------------------------------------------------*/
/** 単語の検索
 *
 * @param words		[in] 単語テーブル
 * @param len		[in] 単語テーブルの長さ
 * @param s			[in] 検索する文字列
 *
 * @retval			-1以外	トークンID
 * @retval			-1		単語がみつからなかった
 */

int lookup_words(keyword_t words[], int len, const char * s)
{
    int	p, q, r, comp;

    p =	0;
    q =	len - 1;

    while (p <= q)
    {
        r = (p + q) / 2;
        comp = strcasecmp(s, words[r].name);

        if (comp == 0)
            return words[r].tokn;

        if (comp < 0)
            q = r - 1;
        else
            p = r + 1;
    }

    return -1;
}


/*------------------------------------------------------------------------*/
/** 単語テーブルのソート用比較関数
 */

int lookup_membcompare(const void * a1, const void * a2)
{
	return strcasecmp(((keyword_t *)a1)->name, ((keyword_t *)a2)->name);
}


/*------------------------------------------------------------------------*/
/** 単語テーブルのソート
 *
 * @param words		[in] 単語テーブル
 * @param len		[in] 単語テーブルの長さ
 *
 * @return			なし
 */

void lookup_sorttable(keyword_t words[], int len)
{
	qsort((void *)words, len, sizeof(keyword_t), &lookup_membcompare);
}
