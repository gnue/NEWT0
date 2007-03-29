/*------------------------------------------------------------------------*/
/**
 * @file	NewtPkg.h
 * @brief   Newton Package Format
 *
 * @author  Matthias Melcher
 * @date	2007-03-25
 *
 * Copyright (C) 2007 Matthias Melcher, All rights reserved.
 */


#ifndef	NEWTPKG_H
#define	NEWTPKG_H

#include "NewtType.h"


#ifdef __cplusplus
extern "C" {
#endif


newtRef		NewtReadPkg(uint8_t * data, size_t size);
newtRef		NewtWritePkg(newtRefArg pkg);

newtRef		NsReadPkg(newtRefArg rcvr, newtRefArg r);
newtRef		NsMakePkg(newtRefArg rcvr, newtRefArg r);


#ifdef __cplusplus
}
#endif


#endif /* NEWTPKG_H */
