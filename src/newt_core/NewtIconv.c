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

#if _MSC_VER

#include <string.h>

/*
 * This is a minimalistic "iconv" version which simply maps into WIN32
 * native calls (not even at this point!). 
 * 
 * Requested conversions are:
 *  UTF-16BE to UTF-8 and back
 *  MACROMAN to UTF-8 and back
 *  where UTF-8 is defined at runtime and could be something else...
 */
iconv_t iconv_open(const char *tocode, const char *fromcode)
{
  iconv_t mode = 0;
  if (!tocode || !fromcode)
    return mode;
  if (strcmp(fromcode, "UTF-16BE")==0)
    mode |= 1;
  if (strcmp(tocode, "UTF-16BE")==0)
    mode |= 2;
  //printf("Requested iconv form '%s' to '%s' (our code: %d)\n", fromcode, tocode, mode);
  return mode;
}

size_t iconv(iconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
  int i, n;
  const char *s;
  char *d;

  if (!inbuf || !*inbuf || !outbuf || !*outbuf) 
    return 0;

  n = *inbytesleft;
  s = *inbuf;
  d = *outbuf;

  switch (cd) {
  case 0:
  case 3:
    memmove(*outbuf, *inbuf, n);
    *inbytesleft -= n;
    *inbuf += n;
    *outbytesleft -= n;
    *outbuf += n;
    break;
  case 1: // from 16bit to 8bit
    for (i=0; i<n; i++) {
      s++;
      *d++ = *s++;
    }
    *inbytesleft -= 2*n;
    *inbuf += 2*n;
    *outbytesleft -= n;
    *outbuf += n;
    break;
  case 2: // from 8bit to 16bit
    for (i=0; i<n; i++) {
      *d++ = 0;
      *d++ = *s++;
    }
    *inbytesleft -= n;
    *inbuf += n;
    *outbytesleft -= 2*n;
    *outbuf += 2*n;
    break;
  }
  return 0;
}

int iconv_close(iconv_t type)
{
  return 0;
}

#endif


#endif /* HAVE_LIBICONV */
