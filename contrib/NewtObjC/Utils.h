/// ==============================
/// @file			Utils.h
/// @author			Paul Guyot <pguyot@kallisys.net>
/// @date			2005-03-11
/// @brief			Various utility functions.
/// 
/// ***** BEGIN LICENSE BLOCK *****
/// Version: MPL 1.1
/// 
/// The contents of this file are subject to the Mozilla Public License Version
/// 1.1 (the "License"); you may not use this file except in compliance with
/// the License. You may obtain a copy of the License at
/// http://www.mozilla.org/MPL/
/// 
/// Software distributed under the License is distributed on an "AS IS" basis,
/// WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
/// for the specific language governing rights and limitations under the
/// License.
/// 
/// The Original Code is Utils.h.
/// 
/// The Initial Developer of the Original Code is Paul Guyot.
/// Portions created by the Initial Developer are Copyright (C) 2005 the
/// Initial Developer. All Rights Reserved.
/// 
/// Contributor(s):
///   Paul Guyot <pguyot@kallisys.net> (original author)
/// 
/// ***** END LICENSE BLOCK *****
/// ===========
/// @version $Id$
/// ===========

#ifndef _UTILS_H
#define _UTILS_H

// ANSI C & POSIX
#include <stdarg.h>

// ObjC & Cocoa
#include <objc/objc.h>
#include <objc/objc-runtime.h>
#include <Cocoa/Cocoa.h>

// NEWT/0
#include "NewtType.h"

// Prototypes
void* BinaryToPointer(newtRefArg);
newtRef PointerToBinary(void*);
double RefToDoubleConverting(newtRefVar);
int RefToIntConverting(newtRefVar);
char RefToCharConverting(newtRefVar);
void RefToPoint(newtRefArg, NSPoint*);
void RefToRange(newtRefArg, NSRange*);
void RefToRect(newtRefArg, NSRect*);
void RefToSize(newtRefArg, NSSize*);
newtRef PointToRef(NSPoint);
newtRef RangeToRef(NSRange);
newtRef RectToRef(NSRect);
newtRef SizeToRef(NSSize);
char* ObjCNSNameTranslation(const char*);
newtRef CastToNS(id*, const char*);
newtRef CastStructToNS(void*, const char*);
newtRef CastIdToNS(id inObjCObject);
newtRef CastParamsToNS(va_list inArgList, Method inMethod);
void CastParamToObjC(void*, int, const char*, void**, newtRefArg, int);
id CastResultToObjC(const char* inType, newtRefArg inObject);
void Lock(newtRefArg);
NSException* CastExToObjC(newtRefArg inNSException);
newtRef CastExToNS(NSException* inObjCException);

#endif
		// _UTILS_H

// ====================================================================== //
//         THE LESSER-KNOWN PROGRAMMING LANGUAGES #18: FIFTH              //
//                                                                        //
// FIFTH is a precision mathematical language in which the data types     //
// refer to quantity.  The data types range from CC, OUNCE, SHOT, and     //
// JIGGER to FIFTH (hence the name of the language), LITER, MAGNUM and    //
// BLOTTO.  Commands refer to ingredients such as CHABLIS, CHARDONNAY,    //
// CABERNET, GIN, VERMOUTH, VODKA, SCOTCH, and WHATEVERSAROUND.           //
//                                                                        //
// The many versions of the FIFTH language reflect the sophistication and //
// financial status of its users.  Commands in the ELITE dialect include  //
// VSOP and LAFITE, while commands in the GUTTER dialect include HOOTCH   //
// and RIPPLE. The latter is a favorite of frustrated FORTH programmers   //
// who end up using this language.                                        //
// ====================================================================== //
