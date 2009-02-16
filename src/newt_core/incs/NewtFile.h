/*------------------------------------------------------------------------*/
/**
 * @file	NewtFile.h
 * @brief   ファイル処理
 *
 * @author  M.Nukui
 * @date	2004-01-25
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTFILE_H
#define	NEWTFILE_H


/* ヘッダファイル */
#include "NewtType.h"


/* マクロ */
#define NcLoadLib(r)				NsLoadLib(kNewtRefNIL, r)
#define NcLoad(r)					NsLoad(kNewtRefNIL, r)
#define NcRequire(r)				NsRequire(kNewtRefNIL, r)

#define NcFileExists(r)				NsFileExists(kNewtRefNIL, r)
#define NcDirName(r)				NsDirName(kNewtRefNIL, r)
#define NcBaseName(r)				NsBaseName(kNewtRefNIL, r)
#define NcJoinPath(r1, r2)			NsJoinPath(kNewtRefNIL, r1, r2)
#define NcExpandPath(r)				NsExpandPath(kNewtRefNIL, r)


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


void *		NewtDylibInstall(const char* fname);
bool		NewtFileExists(char * path);

char		NewtGetFileSeparator(void);
char *		NewtGetHomeDir(const char * s, char ** subdir);
char *		NewtRelToAbsPath(char * s);
char *		NewtJoinPath(char * s1, char * s2, char sep);
newtRef		NewtExpandPath(const char * s);

char *		NewtBaseName(char * s, uint32_t len);

newtRef		NsCompileFile(newtRefArg rcvr, newtRefArg r);
newtRef		NsLoadLib(newtRefArg rcvr, newtRefArg r);
newtRef		NsLoad(newtRefArg rcvr, newtRefArg r);
newtRef		NcRequire0(newtRefArg r);
newtRef		NsRequire(newtRefArg rcvr, newtRefArg r);

newtRef		NsFileExists(newtRefArg rcvr, newtRefArg r);

newtRef		NsDirName(newtRefArg rcvr, newtRefArg r);
newtRef		NsBaseName(newtRefArg rcvr, newtRefArg r);
newtRef		NsJoinPath(newtRefArg rcvr, newtRefArg r1, newtRefArg r2);
newtRef		NsExpandPath(newtRefArg rcvr, newtRefArg r);

newtRef		NsLoadBinary(newtRefArg rcvr, newtRefArg r);
newtRef		NsSaveBinary(newtRefArg rcvr, newtRefArg r1, newtRefArg r2);

#ifdef __cplusplus
}
#endif


#endif /* NEWTFILE_H */


