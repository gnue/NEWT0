/// ==============================
/// @file			NewtObjC.m
/// @author			Paul Guyot <pguyot@kallisys.net>
/// @date			2005-03-11
/// @brief			Interface for all native code available through ObjC
///					runtime architecture.
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
/// The Original Code is NewtObjC.m.
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


// ANSI C & POSIX
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <mach-o/dyld.h>

// NEWT/0
#include "NewtCore.h"
#include "config.h"

// NewtObjC
#import "AppKitFunctions.h"
#import "Constants.h"
#import "FoundationFunctions.h"
#import "ObjCObjects.h"
#import "Utils.h"

// Prototypes.
void newt_install(void);

/*------------------------------------------------------------------------*/
/**
 * Install the global function and the global variables & constants.
 */

void newt_install(void)
{
#if !TARGET_OS_IPHONE
	// Load the objc-runtime extension dynamic library.
	if (!NSAddLibraryWithSearching("@loader_path/objc-runtime-x.dylib")	// Tiger and higher.
		&& !NSAddLibraryWithSearching("@executable_path/objc-runtime-x.dylib") // In-place.
		&& !NSAddLibraryWithSearching(__LIBDIR__ "/objc-runtime-x.dylib")) // Installed path.
	{
		(void) NewtThrow(kNErrObjCRuntimeErr, NewtMakeString("Couldn't find objc-runtime-x.dylib", true));
    return;
	}
#endif
  
  // Add the GetObjCClass global function.
  NewtDefGlobalFunc(
                    NSSYM(GetObjCClass),
                    GetObjCClass,
                    1,
                    "GetObjCClass(name)");
	
  // Add the CreateObjCClass global function.
  NewtDefGlobalFunc(
                    NSSYM(CreateObjCClass),
                    CreateObjCClass,
                    1,
                    "CreateObjCClass(def)");
	
  // Add the list of known classes (it's actually a frame).
  NcSetGlobalVar(NSSYM(ObjCClasses), kNewtRefNIL);
	
  // Create kObjCTrue and kObjCFalse (chr(0) and chr(1)).
  NcSetGlobalVar(NSSYM(kObjCTrue), NewtMakeCharacter(1));
  NcSetGlobalVar(NSSYM(kObjCFalse), NewtMakeCharacter(0));
  
  // Create kObjCActionFuncType
  NcSetGlobalVar(NSSYM(kObjCActionFuncType),
                 NewtMakeString(kObjCActionFuncTypeStr, true));
	
  // Create kObjCOutletType
  NcSetGlobalVar(NSSYM(kObjCOutletType),
                 NewtMakeString(kObjCOutletTypeStr, true));
  
  // Create magic pointer (for instance variable methods)
  newtRefVar magicPtr = NcMakeFrame();
  
  NcSetSlot(magicPtr,
            NSSYM(InstanceVariableExists),
            NewtMakeNativeFunc(
                               InstanceVariableExists, 1, "InstanceVariableExists(name)"));
  NcSetSlot(magicPtr,
            NSSYM(GetInstanceVariable),
            NewtMakeNativeFunc(
                               GetInstanceVariable, 1, "GetInstanceVariable(name)"));
  
  NcDefMagicPointer(kInstanceParentMagicPtrKey, magicPtr);
	
#if !TARGET_OS_IPHONE
  // Add the NSApplicationMain global function.
  NewtDefGlobalFunc(
                    NSSYM(NSApplicationMain),
                    NewtNSApplicationMain,
                    1,
                    "NSApplicationMain(argv)");
#else
  // Add the UIApplicationMain global function.
  NewtDefGlobalFunc(
                    NSSYM(UIApplicationMain),
                    NewtUIApplicationMain,
                    1,
                    "UIApplicationMain(argv)");
#endif
  
  // Add the NSLog global function.
  NewtDefGlobalFunc0(
                     NSSYM(NSLog),
                     NewtNSLog,
                     1,
                     true,
                     "NSLog(format, ...)");
}

// =================================================== //
// If this is timesharing, give me my share right now. //
// =================================================== //
