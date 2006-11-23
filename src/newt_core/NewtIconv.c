/**
 * @file	NewtIconv.c
 * @brief   文字コード処理（libiconv使用）
 *
 * @author  M.Nukui
 * @date	2005-07-17
 *
 * Copyright (C) 2005 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include "NewtIconv.h"


#ifdef HAVE_LIBICONV
/*------------------------------------------------------------------------*/
/** NSOFバッファを読込んで配列オブジェクトに変換する
 *
 * @param cd		[in] iconv変換ディスクリプター
 * @param src		[in] 変換する文字列
 * @param srclen	[in] 変換する文字列の長さ
 * @param dstlenp	[out]変換された文字列の長さ
 *
 * @return			変換された文字列
 *
 * @note			変換された文字列は呼出し元で free する必要あり
 */

char * NewtIconv(iconv_t cd, char * src, size_t srclen, size_t* dstlenp)
{
	char *	dst = NULL;
	size_t	dstlen = 0;

	if (cd != (iconv_t)-1)
	{
		size_t	bufflen;

		bufflen = srclen * 3;
		dst = malloc(bufflen);

		if (dst)
		{
			const char *	inbuf_p = src;
			char *	outbuf_p = dst;
			size_t	inbytesleft = srclen;
			size_t	outbytesleft = bufflen;
			size_t	status;

			iconv(cd, NULL, NULL, NULL, NULL);
			status = iconv(cd, &inbuf_p, &inbytesleft, &outbuf_p, &outbytesleft);

			if (status == (size_t)-1)
			{	// 変換に失敗したのでバッファを解放する
				free(dst);
				dst = NULL;
			}
			else
			{	// いらない部分のバッファを切り詰める
				dstlen = bufflen - outbytesleft;
				dst = realloc(dst, dstlen);
			}
		}
	}

	if (dstlenp) *dstlenp = dstlen;

	return dst;
}

#endif /* HAVE_LIBICONV */
