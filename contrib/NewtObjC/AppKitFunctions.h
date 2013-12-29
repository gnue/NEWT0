/// ==============================
/// @file			AppKitFunctions.h
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
/// The Original Code is AppKitFunctions.h.
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

#ifndef _APPKITFUNCTIONS_H
#define _APPKITFUNCTIONS_H

#include "NewtType.h"

// Prototypes
#if !TARGET_OS_IPHONE
newtRef NewtNSApplicationMain(newtRefArg, newtRefArg);
#else
newtRef NewtNSApplicationMain(newtRefArg, newtRefArg);
#endif

#endif
		// _APPKITFUNCTIONS_H

// ============================================================= //
// Basic is a high level languish.  APL is a high level anguish. //
// ============================================================= //
