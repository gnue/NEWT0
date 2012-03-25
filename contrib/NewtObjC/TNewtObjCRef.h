// ==============================
// Fichier:			TNewtObjCRef.h
// Projet:			NewtObjC
// Ecrit par:		Paul Guyot (pguyot@kallisys.net)
// 
// Créé le:			25/3/2005
// Tabulation:		4 espaces
// 
// ***** BEGIN LICENSE BLOCK *****
// Version: MPL 1.1
// 
// The contents of this file are subject to the Mozilla Public License Version
// 1.1 (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at
// http://www.mozilla.org/MPL/
// 
// Software distributed under the License is distributed on an "AS IS" basis,
// WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
// for the specific language governing rights and limitations under the
// License.
// 
// The Original Code is TNewtObjCRef.h.
// 
// The Initial Developer of the Original Code is Paul Guyot.
// Portions created by the Initial Developer are Copyright (C) 2005 the
// Initial Developer. All Rights Reserved.
// 
// Contributor(s):
//   Paul Guyot <pguyot@kallisys.net> (original author)
// 
// ***** END LICENSE BLOCK *****
// ===========
// $Id$
// ===========

#ifndef _TNEWTOBJCREF_H
#define _TNEWTOBJCREF_H

// ObjC & Cocoa
#import <objc/objc.h>
#import <Foundation/Foundation.h>

// NEWT/0
#include "NewtCore.h"

///
/// Class for newtRef in the ObjC world.
///
/// \author Paul Guyot <pguyot@kallisys.net>
/// \version $Revision$
///
/// \test	aucun test défini.
///
@interface TNewtObjCRef : NSObject
{
	newtRefVar	mRef;
}

+ (id) refWithRef: (newtRefArg) inRef;
- (void) dealloc;
- (newtRef) ref;

@end

#endif
		// _TNEWTOBJCREF_H

// ================= //
// Byte your tongue. //
// ================= //
