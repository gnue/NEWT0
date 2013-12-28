/*------------------------------------------------------------------------*/
/**
 * @file	NewtStr.h
 * @brief   文字列処理
 *
 * @author  M.Nukui
 * @date	2004-01-25
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTSTR_H
#define	NEWTSTR_H


/* ヘッダファイル */
#include "NewtType.h"


/* マクロ */
#define	NcChr(r)				NsChr(kNewtRefNIL, r)
#define	NcOrd(r)				NsOrd(kNewtRefNIL, r)
#define	NcStrLen(r)				NsStrLen(kNewtRefNIL, r)

#define	NcSPrintObject(r)		NsSPrintObject(kNewtRefNIL, r)
#define	NcSplit(r, sep)			NsSplit(kNewtRefNIL, r, sep)
#define	NcParamStr(base, array)	NsParamStr(kNewtRefNIL, base, array)


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


newtRef		NsChr(newtRefArg rcvr, newtRefArg r);
newtRef		NsOrd(newtRefArg rcvr, newtRefArg r);
newtRef		NsStrLen(newtRefArg rcvr, newtRefArg r);
newtRef		NsSubStr(newtRefArg rcvr, newtRefArg r, newtRefArg start, newtRefArg count);
newtRef		NsStrEqual(newtRefArg rcvr, newtRefArg a, newtRefArg b);
newtRef		NsStrExactCompare(newtRefArg rcvr, newtRefArg a, newtRefArg b);
newtRef		NsBeginsWith(newtRefArg rcvr, newtRefArg str, newtRefArg sub);
newtRef		NsEndsWith(newtRefArg rcvr, newtRefArg str, newtRefArg sub);

newtRef		NsSPrintObject(newtRefArg rcvr, newtRefArg r);
newtRef		NsSplit(newtRefArg rcvr, newtRefArg r, newtRefArg sep);
newtRef   NsStrPos(newtRefArg rcvr, newtRefArg haystack, newtRefArg needle, newtRefArg start);
newtRef		NsParamStr(newtRefArg rcvr, newtRefArg baseString, newtRefArg paramStrArray);
newtRef NsStrReplace(newtRefArg rcvr, newtRefArg string, newtRefArg substr, newtRefArg replacement, newtRefArg count);


#ifdef __cplusplus
}
#endif


#endif /* NEWTSTR_H */
