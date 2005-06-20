/// ==============================
/// @file			Constants.h
/// @author			Paul Guyot <pguyot@kallisys.net>
/// @date			2005-03-11
/// @brief			Shared constants.
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
/// The Original Code is Constants.h.
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

#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#import "TNewtObjCRef.h"

#define kNErrObjCRuntimeErr			(kNErrMiscBase - 3)	///< OK. Maybe change this.
#define kNErrObjCExceptionName		NSSYM(evt.ex.objc;type.ref.frame.objcex)
#define kNSExceptionObjCName		@"NewtonScriptException"
#define kObjCActionFuncTypeStr		"v12@0:4@8"
#define kObjCDefaultFunc0TypeStr	"@8@0:4"
#define kObjCDefaultFunc1TypeStr	kObjCActionFuncTypeStr
#define kObjCDefaultFunc2TypeStr	"@16@0:4@8@12"
#define kObjCDefaultFunc3TypeStr	"@24@0:4@8@12@16"
#define kObjCNSFrameVarName			"_ns"
#define kObjCNSFrameVarType			@encode(TNewtObjCRef)
#define kObjCOutletTypeStr			@encode(id)
#define kInstanceParentMagicPtrKey	NSSYM(parentObjCInstance)

#endif
		// _CONSTANTS_H

// ==================================================================== //
// Computers can figure out all kinds of problems, except the things in //
// the world that just don't add up.                                    //
// ==================================================================== //
