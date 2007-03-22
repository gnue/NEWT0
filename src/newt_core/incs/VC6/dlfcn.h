 /*------------------------------------------------------------------------*/
/**
 * @file	dlfcn.h
 * @brief   共有ライブラリ（互換用）, set up for VisualC6
 *
 * @author  M.Nukui
 * @date	2004-10-21
 *
 * Copyright (C) 2004 M.Nukui All rights reserved.
*/


#ifndef	_DLFCN_H_
#define	_DLFCN_H_


/* ヘッダファイル */
#include <windows.h>


/* マクロ */
#define RTLD_LAZY       0x1
#define RTLD_NOW        0x2
#define RTLD_LOCAL      0x4
#define RTLD_GLOBAL     0x8
#define RTLD_NOLOAD     0x10
#define RTLD_NODELETE   0x80

#define RTLD_NEXT		((void *) -1)
#define RTLD_DEFAULT	((void *) -2)


#define dlopen(path, mode)		((void *)LoadLibrary(path))
#define dlsym(handle, symbol)	((void *)GetProcAddress((HINSTANCE)handle, symbol))
#define dlerror()				"DLL error"
#define dlclose(handle)			FreeLibrary((HINSTANCE)handle)


#endif /* _DLFCN_H_ */
