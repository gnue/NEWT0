/// ==============================
/// @file			ObjCObjects.h
/// @author			Paul Guyot <pguyot@kallisys.net>
/// @date			2005-03-14
/// @brief			Interface to ObjC objects.
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
/// The Original Code is ObjCObjects.h.
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

#ifndef _OBJCOBJECTS_H
#define _OBJCOBJECTS_H

#include "NewtType.h"
#include <objc/objc.h>

// Prototypes
#ifdef __cplusplus
extern "C" {
#endif
newtRef CreateClassObject(Class);
newtRef GetClassFromName(const char*);
newtRef GetInstanceVariable(newtRefArg, newtRefArg);
newtRef InstanceVariableExists(newtRefArg, newtRefArg);
newtRef GetObjCClass(newtRefArg, newtRefArg);
newtRef CreateObjCClass(newtRefArg, newtRefArg);
newtRef GetInstanceFrame(id);
#ifdef __cplusplus
}
#endif

#endif
		// _OBJCOBJECTS_H

// ============================================================================== //
//         The FIELD GUIDE to NORTH AMERICAN MALES                                //
//                                                                                //
// SPECIES:        Cranial Males                                                  //
// SUBSPECIES:     The Hacker (homo computatis)                                   //
// Plumage:                                                                       //
//         All clothes have a slightly crumpled look as though they came off the  //
//         top of the laundry basket.  Style varies with status.  Hacker managers //
//         wear gray polyester slacks, pink or pastel shirts with wide collars,   //
//         and paisley ties; staff wears cinched-up baggy corduroy pants, white   //
//         or blue shirts with button-down collars, and penholder in pocket.      //
//         Both managers and staff wear running shoes to work, and a black        //
//         plastic digital watch with calculator.                                 //
// ============================================================================== //
