/// ==============================
/// @file			AppKitFunctions.m
/// @author			Paul Guyot <pguyot@kallisys.net>
/// @date			2005-03-14
/// @brief			Interface for Foundation functions.
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
/// The Original Code is AppKitFunctions.m.
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

#import "AppKitFunctions.h"

// ANSI C & POSIX
#include <stdlib.h>

// NEWT/0
#include "NewtCore.h"
#include "NewtVM.h"

// AppKit
#if !TARGET_OS_IPHONE
# include <AppKit/AppKit.h>
#else
# include <UIKit/UIKit.h>
#endif

// NewtObjC
#include "Utils.h"


#if !TARGET_OS_IPHONE
/**
 * Native function: NewtNSApplicationMain.
 * Bridge to NSApplicationMain
 *
 * @param inRcvr		self.
 * @param inArgv		array of arguments (strings) to pass
 *						to NSApplicationMain.
 * @return the result of NSApplicationMain (an int).
 */
newtRef
NewtNSApplicationMain(newtRefArg inRcvr, newtRefArg inArgv)
{
	newtRefVar theResultObj;

	// Translate the args
	if (!NewtRefIsArray(inArgv))
	{
		return NewtThrow(kNErrNotAnArray, inArgv);
	}

	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	int nbArgs = NewtArrayLength(inArgv);
	const char** argv = (const char**) malloc(sizeof(const char*) * (nbArgs + 1));
	int indexArgs;
	for (indexArgs = 0; indexArgs < nbArgs; indexArgs++)
	{
		newtRefVar someArg = NewtSlotsGetSlot( inArgv, indexArgs );
		if (!NewtRefIsString(someArg))
		{
			free(argv);
			return NewtThrow(kNErrNotAString, someArg);
		}
		
		argv[indexArgs] = NewtRefToString(someArg);
	}
	argv[nbArgs] = NULL;
	
	NS_DURING
	
	int theResult = NSApplicationMain(nbArgs, argv);
	theResultObj = NewtMakeInteger(theResult);
	
	NS_HANDLER
		// Convert the exception and throw it as a NS exception.
		newtRefVar theExceptionFrame = CastExToNS(localException);
		NVMThrowData(
			NcGetSlot(theExceptionFrame, NSSYM(name)), theExceptionFrame);
		theResultObj = kNewtRefNIL;
	NS_ENDHANDLER
	
	[pool release];

	free(argv);
	
	return theResultObj;
}
#else // !TARGET_OS_IPHONE
/**
 * Native function: NewtUIApplicationMain.
 * Bridge to UIApplicationMain
 *
 * @param inRcvr		self.
 * @param inArgv		array of arguments (strings) to pass
 *						to UIApplicationMain.
 * @return the result of UIApplicationMain (an int).
 */
newtRef
NewtUIApplicationMain(newtRefArg inRcvr, newtRefArg inArgv)
{
	newtRefVar theResultObj;
  
	// Translate the args
	if (!NewtRefIsArray(inArgv))
	{
		return NewtThrow(kNErrNotAnArray, inArgv);
	}
  
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  
	int nbArgs = NewtArrayLength(inArgv);
	const char** argv = (const char**) malloc(sizeof(const char*) * (nbArgs + 1));
	int indexArgs;
	for (indexArgs = 0; indexArgs < nbArgs; indexArgs++)
	{
		newtRefVar someArg = NewtSlotsGetSlot( inArgv, indexArgs );
		if (!NewtRefIsString(someArg))
		{
			free(argv);
			return NewtThrow(kNErrNotAString, someArg);
		}
		
		argv[indexArgs] = NewtRefToString(someArg);
	}
	argv[nbArgs] = NULL;
	
	NS_DURING
	
	int theResult = UIApplicationMain(nbArgs, argv, NULL, NULL);
	theResultObj = NewtMakeInteger(theResult);
	
	NS_HANDLER
  // Convert the exception and throw it as a NS exception.
  newtRefVar theExceptionFrame = CastExToNS(localException);
  NVMThrowData(
               NcGetSlot(theExceptionFrame, NSSYM(name)), theExceptionFrame);
  theResultObj = kNewtRefNIL;
	NS_ENDHANDLER
	
	[pool release];
  
	free(argv);
	
	return theResultObj;
}
#endif

// ============================================================= //
// Memory fault -- core...uh...um...core... Oh dammit, I forget! //
// ============================================================= //
