/**
 * @file	NewtStr.c
 * @brief   文字列処理
 *
 * @author  M.Nukui
 * @date	2004-01-25
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <string.h>

#include "config.h"

#include "NewtCore.h"
#include "NewtStr.h"


/* 関数プロトタイプ */
static newtRef  NewtParamStr(char * baseStr, size_t baseStrLen, newtRefArg paramStrArray, bool ifthen);
static bool		NewtBeginsWith(const char * str, const char * sub);
static bool		NewtEndsWith(const char * str, const char * sub);


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** ベース文字列のパラメータを置き換えて新しい文字列を作成する
 *
 * @param baseStr		[in] ベース文字列（C文字列）
 * @param baseStrLen	[in] ベース文字列の長さ
 * @param paramStrArray [in] パラメータ配列
 * @param ifthen		[in] 条件処理
 *
 * @return 文字列オブジェクト
 */

newtRef NewtParamStr(char * baseStr, size_t baseStrLen, newtRefArg paramStrArray, bool ifthen)
{
	newtRefVar	dstStr;
	newtRefVar  r;
	size_t	fpos = 0;
	size_t	fst = 0;
	size_t	len;
	size_t	n;
	size_t	truePos;
	size_t	trueLen;
	size_t	falsePos;
	size_t	falseLen;
	char *	found;
	char	c;

	dstStr = NewtMakeString("", false);

	do
	{
		found = memchr(&baseStr[fst], '^', baseStrLen - fst);

		if (found == NULL)
			break;

		c = found[1];

		len = found - (baseStr + fpos);
		NewtStrCat2(dstStr, &baseStr[fpos], len);
		fpos += len;
		fst = fpos + 1;

		if ('0' <= c && c <= '9')
		{
			fpos += 2;
			fst = fpos;

			r = NewtGetArraySlot(paramStrArray, c - '0');

			if (NewtRefIsNotNIL(r))
			{
				NcStrCat(dstStr, r);
			}
		}
		else if (ifthen && c == '?')
		{
			c = found[2];

			if ('0' <= c && c <= '9')
			{
				n = c - '0';

				truePos = fpos + 3;
				found = memchr(&baseStr[truePos], '|', baseStrLen - truePos);

				if (found != NULL)
				{
					falsePos = found + 1 - baseStr;
					trueLen = falsePos - truePos - 1;

					found = memchr(&baseStr[falsePos], '|', baseStrLen - falsePos);

					if (found != NULL)
					{
						fpos = found + 1 - baseStr;
					}
					else
					{
						fpos = baseStrLen;
					}

					falseLen = fpos - falsePos - 1;
				}
				else
				{
					trueLen = baseStrLen - truePos;

					falsePos = baseStrLen;
					falseLen = 0;

					fpos = baseStrLen;
				}

				fst = fpos;

				r = NewtGetArraySlot(paramStrArray, n);

				if (NewtRefIsNotNIL(r))
				{
					r = NewtParamStr(&baseStr[truePos], trueLen, paramStrArray, false);
					NcStrCat(dstStr, r);
				}
				else
				{
					if (falsePos == baseStrLen)
						break;

					r = NewtParamStr(&baseStr[falsePos], falseLen, paramStrArray, false);
					NcStrCat(dstStr, r);
				}
			}
		}
	} while(true);

	len = baseStrLen - fpos;

	if (0 < len)
	{
		NewtStrCat2(dstStr, &baseStr[fpos], len);
	}

	return dstStr;
}


/*------------------------------------------------------------------------*/
/** 文字列の前半部が部分文字列と一致するかチェックする
 *
 * @param str		[in] 文字列
 * @param sub		[in] 部分文字列
 *
 * @retval			true	前半部が部分文字列と一致する
 * @retval			false	前半部が部分文字列と一致しない
 */

bool NewtBeginsWith(const char * str, const char * sub)
{
	int32_t	len;
	int32_t	sublen;

	len = strlen(str);
	sublen = strlen(sub);

	if (len < sublen)
		return false;
	else
		return (strncasecmp(str, sub, sublen) == 0);
}


/*------------------------------------------------------------------------*/
/** 文字列の最後尾が部分文字列と一致するかチェックする
 *
 * @param str		[in] 文字列
 * @param sub		[in] 部分文字列
 *
 * @retval			true	最後尾が部分文字列と一致する
 * @retval			false	最後尾が部分文字列と一致しない
 */

bool NewtEndsWith(const char * str, const char * sub)
{
	int32_t	st;

	st = strlen(str) - strlen(sub);

	if (st < 0)
		return false;
	else
		return (strcasecmp(str + st, sub) == 0);
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 整数を文字に変換する
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] 整数
 *
 * @return			文字
 *
 * @note			グローバル関数用
 */

newtRef NsChr(newtRefArg rcvr, newtRefArg r)
{
    if (! NewtRefIsInteger(r))
        return NewtThrow(kNErrNotAnInteger, r);

    return NewtMakeCharacter(NewtRefToInteger(r));
}


/*------------------------------------------------------------------------*/
/** 文字を整数に変換する
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] 文字
 *
 * @return			整数
 *
 * @note			グローバル関数用
 */

newtRef NsOrd(newtRefArg rcvr, newtRefArg r)
{
    if (! NewtRefIsCharacter(r))
        return NewtThrow(kNErrNotAnInteger, r);

    return NewtMakeInteger(NewtRefToCharacter(r));
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 文字列の長さを取得
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] 文字列オブジェクト
 *
 * @return			文字列の長さ
 *
 * @note			グローバル関数用
 */

newtRef NsStrLen(newtRefArg rcvr, newtRefArg r)
{
    if (! NewtRefIsString(r))
        return NewtThrow(kNErrNotAString, r);

    return NewtMakeInteger(NewtStringLength(r));
}


/*------------------------------------------------------------------------*/
/** オブジェクトを表示可能な文字列に変換する
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] オブジェクト
 *
 * @return			文字列オブジェクト
 *
 * @note			グローバル関数用
 */

newtRef NsSPrintObject(newtRefArg rcvr, newtRefArg r)
{
    newtRefVar	str;

    str = NSSTR("");

	NcStrCat(str, r);

    return str;
}


/*------------------------------------------------------------------------*/
/** 文字列を指定の区切り文字で分解する
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] 文字列オブジェクト
 * @param sep		[in] 区切り文字
 *
 * @return			配列オブジェクト
 *
 * @note			グローバル関数用
 */

newtRef NsSplit(newtRefArg rcvr, newtRefArg r, newtRefArg sep)
{
	newtRefVar  result;

    if (! NewtRefIsString(r))
        return NewtThrow(kNErrNotAString, r);

	switch (NewtGetRefType(sep, true))
	{
		case kNewtCharacter:
			{
				newtRefVar  v;
				char *		next;
				char *		s;
				int			c;

				s = NewtRefToString(r);
				c = NewtRefToCharacter(sep);

				result = NewtMakeArray(kNewtRefUnbind, 0);

				while (*s)
				{
					next = strchr(s, c);
					if (next == NULL) break;

					v = NewtMakeString2(s, next - s, false);
					NcAddArraySlot(result, v);
					s = next + 1;
				}

				if (s == NewtRefToString(r))
					v = r;
				else
					v = NSSTR(s);

				NcAddArraySlot(result, v);
			}
			break;

		default:
			{
				newtRefVar	initObj[] = {r};

				result = NewtMakeArray2(kNewtRefNIL, sizeof(initObj) / sizeof(newtRefVar), initObj);
			}
			break;
	}

    return result;
}

newtRef NsStrPos(newtRefArg rcvr, newtRefArg haystack, newtRefArg needle, newtRefArg start)
{
  if (! NewtRefIsString(haystack))
    return NewtThrow(kNErrNotAString, haystack);

  char *s = NewtRefToString(haystack);

  if (!NewtRefIsInteger(start)) {
    return NewtThrow(kNErrNotAnInteger, start);
  }
  
  int offset = NewtRefToInteger(start);
  if (offset < 0) {
    offset = -1; //strlen(s) + offset;
  }
  
  if (offset < 0 || offset >= strlen(s)) {
    return kNewtRefNIL;
  }
  
  const char *match = NULL;
  
  if (NewtRefIsString(needle)) {
    const char *c = NewtRefToString(needle);
    match = strcasestr(s + offset, c);
  }
  else if (NewtRefIsCharacter(needle)) {
    const char c = NewtRefToCharacter(needle);
    match = strchr(s + offset, c);
  }
  else {
    return NewtThrow(kNErrNotAString, needle);
  }


  if (match == NULL) {
    return kNewtRefNIL;
  }
        
  return NewtMakeInt32(match - s);
}

newtRef NsStrReplace(newtRefArg rcvr, newtRefArg string, newtRefArg substr, newtRefArg replacement, newtRefArg count) {
  const char *ins;    // the next insert point
  char *tmp;    // varies
  int len_rep;  // length of rep
  int len_with; // length of with
  int len_front; // distance between rep and end of last rep
  
  int occurences;    // number of replacements

  if (! NewtRefIsString(string))
    return NewtThrow(kNErrNotAString, string);
  if (! NewtRefIsString(substr))
    return NewtThrow(kNErrNotAString, substr);
  if (! NewtRefIsString(replacement))
    return NewtThrow(kNErrNotAString, replacement);
  
  const char *orig = NewtRefToString(string);
  const char *rep = NewtRefToString(substr);
  const char *with = NewtRefToString(replacement);

  ins = orig;
  
  // count == 0 ? nothing to do
  // count == nil ? replace all occurences
  
  
  len_with = strlen(with);
  len_rep = strlen(rep);
  
  if (len_rep == 0) {
    return NewtMakeInteger(0);
  }
  
  for (occurences = 0; (tmp = strstr(ins, rep)); ++occurences) {
    ins = tmp + len_rep;
  }
  
  if (NewtRefIsInteger(count)) {
    if (occurences > NewtRefToInteger(count)) {
      occurences = NewtRefToInteger(count);
    }
  }
  
  if (occurences == 0) {
    return NewtMakeInt32(0);
  }
  
  int mallocLen = strlen(orig) + (len_with - len_rep) * occurences + 1;
  char *result = tmp = calloc(mallocLen, sizeof(char *));
  
  if (!tmp) {
    return NewtThrow(kNErrOutOfObjectMemory, rcvr);
  }

  int remaining = occurences;
  while (remaining--) {
    ins = strstr(orig, rep);
    len_front = ins - orig;
    strncpy(tmp, orig, len_front);
    tmp+=len_front;
    strncpy(tmp, with, len_with);
    tmp+=len_with;
    orig += len_front + len_rep;
  }
  strcpy(tmp, orig);

  orig = NewtRefToString(string);
  NewtSetLength(string, strlen(result));
  strcpy(NewtRefToData(string), result);
  free(result);
  
  return NewtMakeInt32(occurences);
}

/*------------------------------------------------------------------------*/
/** ベース文字列のパラメータを置き換えて新しい文字列を作成する
 *
 * @param rcvr			[in] レシーバ
 * @param baseString	[in] ベース文字列
 * @param paramStrArray [in] パラメータ配列
 *
 * @return			文字列オブジェクト
 *
 * @note			グローバル関数用
 */

newtRef NsParamStr(newtRefArg rcvr, newtRefArg baseString, newtRefArg paramStrArray)
{
    if (! NewtRefIsString(baseString))
        return NewtThrow(kNErrNotAString, baseString);

    if (! NewtRefIsArray(paramStrArray))
        return NewtThrow(kNErrNotAnArray, paramStrArray);

	return NewtParamStr(NewtRefToString(baseString), NewtStringLength(baseString), paramStrArray, true);
}

/*------------------------------------------------------------------------*/
/**
 * Extract the substring of a string.
 *
 * @param rcvr	self (ignored).
 * @param r		the string to create a substring of.
 * @param start	the offset of the first character of the substring.
 * @param count	the number of characters to extract or NIL to go til the end.
 * @return a new string
 *
 * @note highly unefficient.
 */

newtRef NsSubStr(newtRefArg rcvr, newtRefArg r, newtRefArg start, newtRefArg count)
{
	char* theString;
	char* theBuffer;
	int theStart, theEnd;
	size_t theLen;
	newtRefVar theResult;
	
	(void) rcvr;
	
	/* check parameters */
  if (! NewtRefIsString(r))
    return NewtThrow(kNErrNotAString, r);
  if (! NewtRefIsInteger(start))
    return NewtThrow(kNErrNotAnInteger, start);
  
  theString = NewtRefToString(r);
  theLen = strlen(theString);
  
  theStart = NewtRefToInteger(start);
  if (theStart > theLen) {
    return NewtMakeString("", true);
  }
  
  if (!NewtRefIsNIL(count))
  {
    if (!NewtRefIsInteger(count))
    {
      return NewtThrow(kNErrNotAnInteger, count);
    }
    theEnd = theStart + NewtRefToInteger(count);
    if (theEnd > theLen)
    {
      theEnd = theLen;
    }
	} else {
		theEnd = theLen;
	}
  
	/* new length */
	theLen = theEnd - theStart;
	
	/* create a buffer to copy the characters to */
	theBuffer = (char*) malloc(theLen + 1);
	(void) memcpy(theBuffer, (const char*) &theString[theStart], theLen);
	theBuffer[theLen] = 0;
	theResult = NewtMakeString(theBuffer, false);
	free(theBuffer);
	
	return theResult;
}

/*------------------------------------------------------------------------*/
/**
 * Determine if two strings are equal, ignoring case.
 *
 * @param rcvr	self (ignored).
 * @param a		the first string to consider.
 * @param b		the second string to consider.
 * @return true if the two strings are equal, nil otherwise.
 */

newtRef NsStrEqual(newtRefArg rcvr, newtRefArg a, newtRefArg b)
{
	char* aString;
	char* bString;
	newtRefVar theResult = kNewtRefNIL;
	
	(void) rcvr;

	/* check parameters */
    if (! NewtRefIsString(a))
    {
    	theResult = NewtThrow(kNErrNotAString, a);
    } else if (! NewtRefIsString(b)) {
        theResult = NewtThrow(kNErrNotAString, b);
	} else if (a == b) {
		theResult = kNewtRefTRUE;
	} else {    
		aString = NewtRefToString(a);
		bString = NewtRefToString(b);
	
		if (strcasecmp(aString, bString) == 0)
		{
			theResult = kNewtRefTRUE;
		}
	}
	
	return theResult;
}

/*------------------------------------------------------------------------*/
/**
 * Compare two strings, returning an integer representing the result of the
 * comparison. The comparison is case sensitive.
 *
 * @param rcvr	self (ignored).
 * @param a		the first string to consider.
 * @param b		the second string to consider.
 * @return an integer representing the result of the comparison (a < b -> < 0,
 *         a = b -> 0, a > b -> > 0)
 */

newtRef NsStrExactCompare(newtRefArg rcvr, newtRefArg a, newtRefArg b)
{
	char* aString;
	char* bString;
	newtRefVar theResult;
	
	(void) rcvr;

	/* check parameters */
    if (! NewtRefIsString(a))
    {
    	theResult = NewtThrow(kNErrNotAString, a);
    } else if (! NewtRefIsString(b)) {
        theResult = NewtThrow(kNErrNotAString, b);
	} else if (a == b) {
		theResult = NewtMakeInteger(0);
	} else {    
		aString = NewtRefToString(a);
		bString = NewtRefToString(b);
	
		theResult = NewtMakeInteger(strcmp(aString, bString));
	}
	
	return theResult;
}


/*------------------------------------------------------------------------*/
/** 文字列の前半部が部分文字列と一致するかチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param str		[in] 文字列
 * @param sub		[in] 部分文字列
 *
 * @retval			TRUE	前半部が部分文字列と一致する
 * @retval			NIL		前半部が部分文字列と一致しない
 */

newtRef NsBeginsWith(newtRefArg rcvr, newtRefArg str, newtRefArg sub)
{
	bool	result;

    if (! NewtRefIsString(str))
        return NewtThrow(kNErrNotAString, str);

    if (! NewtRefIsString(sub))
        return NewtThrow(kNErrNotAString, sub);

	result = NewtBeginsWith(NewtRefToString(str), NewtRefToString(sub));

	return NewtMakeBoolean(result);
}


/*------------------------------------------------------------------------*/
/** 文字列の最後尾が部分文字列と一致するかチェックする
 *
 * @param rcvr		[in] レシーバ
 * @param str		[in] 文字列
 * @param sub		[in] 部分文字列
 *
 * @retval			TRUE	最後尾が部分文字列と一致する
 * @retval			NIL		最後尾が部分文字列と一致しない
 */

newtRef NsEndsWith(newtRefArg rcvr, newtRefArg str, newtRefArg sub)
{
	bool	result;

    if (! NewtRefIsString(str))
        return NewtThrow(kNErrNotAString, str);

    if (! NewtRefIsString(sub))
        return NewtThrow(kNErrNotAString, sub);

	result = NewtEndsWith(NewtRefToString(str), NewtRefToString(sub));

	return NewtMakeBoolean(result);
}
