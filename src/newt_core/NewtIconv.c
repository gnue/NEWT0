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
			status = iconv(cd, (char **) &inbuf_p, &inbytesleft, &outbuf_p, &outbytesleft);

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
#include <windows.h>
#include <winnls.h>

/**
 * This is a minimalistic "iconv" for VC6.
 * This version simply maps into WIN32 native calls and is pretty limited
 * but likely to be sufficient within NEWT/0.
 * 
 * Requested conversions in NEWT/0 as of 03/24/2007 are:
 *  UTF-16BE to current code page and back
 *  MACROMAN to current code page and back
 *
 * So we define the iconv_t bits ...00ss00dd where ss is the source format and
 * dd is the destination format with:
 *   00 = current code page
 *   01 = Mac Roman
 *   10 = UTF-16BE, Newton Format
 *   11 = UTF-16LE., MSWindows Format
 * iconv_t is -1 on error or unsupported conversion
 *
 * \todo move this into the VC6 directories
 */
iconv_t iconv_open(const char *tocode, const char *fromcode)
{
  iconv_t mode = 0;
  // avoid a crash if the user does not privide encodings
  if (!tocode || !fromcode)
    return -1;

  // determine the source text format
  // if we can't identify the string, we assume the current codepage
  if (strcmp(fromcode, "MACROMAN")==0)
    mode |= 0x10;
  else if (strcmp(fromcode, "UTF-16BE")==0)
    mode |= 0x20;
  else if (strcmp(fromcode, "UTF-16LE")==0)
    mode |= 0x30;

  // determine the destination text format of the text source
  if (strcmp(tocode, "MACROMAN")==0)
    mode |= 0x01;
  else if (strcmp(tocode, "UTF-16BE")==0)
    mode |= 0x02;
  else if (strcmp(tocode, "UTF-16LE")==0)
    mode |= 0x03;

  return mode;
}

/*
 * Flip the endianness of a 16 bit per char string
 */
static void ic_flip_endian(void *dst, const void *src, int n) {
  unsigned char *d = (unsigned char *)dst;
  unsigned char *s = (unsigned char *)src;
  for ( ; n>0; n--) {
    unsigned char c = *s++;
    *d++ = *s++;
    *d++ = c;
  }
}

/**
 * Convert a string of text from one encoding to another.
 *
 * \param cd[in] conversion descriptor, see iconv_open()
 * \param inbuf[inout] source buffer, will be incremented for each converted character; this
 *        value may be incorrect if the output buffer is too small.
 * \param inbytesleft[inout] number of bytes in inbuffer, will be decremented for each conversion
 * \param outbuf[inout] destination buffer, will be incremented
 * \param outbytesleft[inout] number of bytes still free in buffer, will be decremented
 * \return number of characters converted that can not be converted back (not implemented);
 *         we return 0 for a complete conversion, and -1 for any error
 */
size_t iconv(iconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
  // reusable buffer for temporary conversion
  static unsigned short *wbuf = 0L;
  static int NWbuf = 0;
# define MAKE_ROOM(n) if (NWbuf<(n)) { NWbuf=(n)+32; wbuf = realloc(wbuf, NWbuf); }

  // addresses require by WIN32 calls
  static char *dflt = ".";
  BOOL dfltUsed;

  // some variables
  const char *src;
  char *dst;
  unsigned short *tmp;
  int sn, dn, tn;
  size_t ret = 0;

  // handle the special cases first
  if (inbuf==0L || *inbuf==0L) {
    if (outbuf==0L || *outbuf==0L) {
      // sepcial case: initialize converter
      // (nothing to do here)
      return 0;
    } else {
      // special case: write a format indicator
      // (not implemented)
      return 0;
    }
  }

  // catch faulty parameters
  if (!inbytesleft || !outbytesleft || outbuf==0L || *outbuf==0L)
    return -1;

  src = *inbuf; dst = *outbuf; sn = *inbytesleft; dn = *outbytesleft;
  if (sn==0) 
    return 0;

  // take care of all cases without any conversion
  if ( (cd&0x3)==((cd>>4)&0x3) ) {
    if (sn<=dn) {
      dn = sn;
    } else {
      sn = dn;
      ret = -1;
    }
    memcpy(dst, src, sn);
    goto fixup_return_values;
  }

  // now, the conversion on WIN32 is always a two-step process
  // because WIN32 can only convert to and from UTF-16LE

  // convert from old format to UTF-16LE
  switch (cd & 0x30) {
  case 0x00: // from local code page, WIN32 does that
    MAKE_ROOM(sn*2);
    tn = 2 * MultiByteToWideChar(CP_THREAD_ACP, MB_PRECOMPOSED, src, sn, wbuf, sn);
    tmp = wbuf;
    break;
  case 0x10: // from Mac Roman, WIN32 does that
    MAKE_ROOM(sn*2);
    tn = 2 * MultiByteToWideChar(CP_MACCP, MB_PRECOMPOSED, src, sn, wbuf, sn);
    tmp = wbuf;
    break;
  case 0x20: // from UTF-16BE, flip the byte order
    MAKE_ROOM(sn);
    ic_flip_endian(wbuf, src, sn/2);
    tmp = wbuf; tn = sn;
    break;
  case 0x30: // from UTF-16LE, make the source buffer the temp buffer
    tmp = (unsigned short*)src; tn = sn;
    break;
  }

  // convert from UTF-16LE to new format
  switch (cd & 0x03) {
  case 0x00: // to local code page
    dn = WideCharToMultiByte(CP_THREAD_ACP, 0, tmp, tn/2, dst, dn, dflt, &dfltUsed);
    if (dn==0) 
      ret = -1;
    break;
  case 0x01: // to Mac Roman
    dn = WideCharToMultiByte(CP_MACCP, 0, tmp, tn/2, dst, dn, dflt, &dfltUsed);
    if (dn==0) 
      ret = -1;
    break;
  case 0x02: // to UTF-16BE
    if (tn<=dn) {
      dn = tn;
    } else {
      tn = dn;
      ret = -1;
    }
    ic_flip_endian(dst, tmp, tn/2);
    break;
  case 0x03: // to UTF-16LE
    if (tn<=dn) {
      dn = tn;
    } else {
      tn = dn;
      ret = -1;
    }
    memcpy(dst, tmp, tn);
    break;
  }

fixup_return_values:
  *inbuf  += sn; *inbytesleft  -= sn;
  *outbuf += dn; *outbytesleft -= dn;
  if (ret==-1) 
    errno = 7;
  return ret;

#undef MAKE_ROOM
}

int iconv_close(iconv_t type)
{
  return 0;
}

#endif


#endif /* HAVE_LIBICONV */
