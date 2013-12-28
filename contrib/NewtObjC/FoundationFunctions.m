/// ==============================
/// @file			FoundationFunctions.m
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
/// The Original Code is FoundationFunctions.m.
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

#import "FoundationFunctions.h"

// NEWT/0
#include "NewtCore.h"
#include "NewtVM.h"

// Foundation
#include <Foundation/Foundation.h>

// NewtObjC
#include "Utils.h"

/**
 * Native function: NewtNSLog.
 * Bridge to NSLog
 *
 * @param inRcvr			self.
 * @param inFormat			Newt or ObjC string.
 * @param additionalArgs	Additional arguments.
 * @return nil
 */
newtRef
NewtNSLog(newtRefArg inRcvr, newtRefArg inFormat, newtRefArg additionalArgs)
{
	// Translate the arg
	if (!NewtRefIsString(inFormat))
	{
		return NewtThrow(kNErrNotAString, inFormat);
	}
	
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	
	NS_DURING
	
	NSLog(@"%@", [NSString stringWithCString: NewtRefToString(inFormat)]);
	
	NS_HANDLER
		// Convert the exception and throw it as a NS exception.
		newtRefVar theExceptionFrame = CastExToNS(localException);
		NVMThrowData(
			NcGetSlot(theExceptionFrame, NSSYM(name)), theExceptionFrame);
	NS_ENDHANDLER

	[pool release];

	return kNewtRefNIL;
}

// ========================================================================== //
// `Lasu' Releases SAG 0.3 -- Freeware Book Takes Paves For New World Order   //
// by staff writers                                                           //
//                                                                            //
//         ...                                                                //
//         The SAG is one of the major products developed via the Information //
// Superhighway, the brain child of Al Gore, US Vice President.  The ISHW     //
// is being developed with massive govenment funding, since studies show      //
// that it already has more than four hundred users, three years before       //
// the first prototypes are ready.  Asked whether he was worried about the    //
// foreign influence in an expensive American Dream, the vice president       //
// said, ``Finland?  Oh, we've already bought them, but we haven't told       //
// anyone yet.  They're great at building model airplanes as well.  And _I   //
// can spell potato.''  House representatives are not mollified, however,     //
// wanting to see the terms of the deal first, fearing another Alaska.        //
//         Rumors about the SAG release have imbalanced the American stock    //
// market for weeks.  Several major publishing houses reached an all time     //
// low in the New York Stock Exchange, while publicly competing for the       //
// publishing agreement with Mr. Wirzenius.  The negotiations did not work    //
// out, tough.  ``Not enough dough,'' says the author, although spokesmen     //
// at both Prentice-Hall and Playboy, Inc., claim the author was incapable    //
// of expressing his wishes in a coherent form during face to face talks,     //
// preferring to communicate via e-mail.  ``He kept muttering something       //
// about jiffies and pegs,'' they say.                                        //
//         ...                                                                //
//                 -- Lars Wirzenius <wirzeniu@cs.helsinki.fi>                //
//                    [comp.os.linux.announce]                                //
// ========================================================================== //
