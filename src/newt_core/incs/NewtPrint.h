/*------------------------------------------------------------------------*/
/**
 * @file	NewtPrint.h
 * @brief   プリント関係
 *
 * @author  M.Nukui
 * @date	2005-04-11
 *
 * Copyright (C) 2003-2005 M.Nukui All rights reserved.
 */


#ifndef	NEWTPRINT_H
#define	NEWTPRINT_H


/* ヘッダファイル */
#include <stdio.h>
#include "NewtType.h"


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif


void		NewtPrintObj(FILE * f, newtRefArg r);
void		NewtPrintObject(FILE * f, newtRefArg r);
void		NewtPrint(FILE * f, newtRefArg r);
void		NewtInfo(newtRefArg r);
void		NewtInfoGlobalFns(void);


#ifdef __cplusplus
}
#endif


#endif /* NEWTPRINT_H */

