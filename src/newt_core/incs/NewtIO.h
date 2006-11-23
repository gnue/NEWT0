/*------------------------------------------------------------------------*/
/**
 * @file	NewtIO.h
 * @brief   入出力処理
 *
 * @author  M.Nukui
 * @date	2004-06-05
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTIO_H
#define	NEWTIO_H

/* ヘッダファイル */
#include <stdio.h>
#include <stdarg.h>

#include "NewtType.h"


/* マクロ */
#define NcGets()				NsGets(kNewtRefNIL)
#define NcGetc()				NsGetc(kNewtRefNIL)
#define NcGetch()				NsGetch(kNewtRefNIL)


/// 入出力ストリーム
typedef struct {
    FILE *		file;		///< ファイルポインタ
	newtRefVar	obj;		///< オブジェクト
} newtStream_t;


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


void		NIOSetFile(newtStream_t * stream, FILE * f);
int			NIOFprintf(newtStream_t * stream, const char * format, ...);
int			NIOVfprintf(newtStream_t * stream, const char * format, va_list ap);
int			NIOFputc(int c, newtStream_t * stream);
int			NIOFputs(const char *str, newtStream_t * stream);

int			NewtFprintf(FILE * f, const char * format, ...);
int			NewtFputc(int c, FILE * f);
int			NewtFputs(const char *str, FILE * f);

int			NewtDebugMsg(const char * title, const char * format, ...);

newtRef		NewtFgets(FILE * stream);
newtRef		NsGets(newtRefArg rcvr);
newtRef		NewtFgetc(FILE * stream);
newtRef		NsGetc(newtRefArg rcvr);
newtRef		NsGetch(newtRefArg rcvr);


#ifdef __cplusplus
}
#endif


#endif /* NEWTIO_H */
