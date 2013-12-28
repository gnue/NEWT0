/// ==============================
/// @file			Utils.m
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
/// The Original Code is Utils.m.
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

#import "Utils.h"

// ANSI & POSIX
#include <string.h>
#include <stdlib.h>

// ObjC & Cocoa
#include <objc/objc.h>
#include <objc/objc-class.h>
#include <objc/objc-runtime.h>

#if !TARGET_OS_IPHONE
# include <Cocoa/Cocoa.h>
#else
# include <UIKit/UIKit.h>
#endif

// NEWT/0
#include "NewtCore.h"

// NewtObjC
#import "Constants.h"
#import "ObjCObjects.h"

/**
 * Retrieve a pointer stored in a binary.
 *
 * @param inPointerBinary	binary holding the pointer.
 * @return the pointer stored in this binary or NULL if the parameter is not
 *			a binary.
 */
void*
BinaryToPointer(newtRefArg inPointerBinary)
{
	void* theResult = NULL;

	if (NewtRefIsBinary(inPointerBinary))
	{
		// It's a binary. Extract its value
		theResult = *((void**) NewtRefToBinary(inPointerBinary));
	}
	
	return theResult;
}

/**
 * Store a pointer to a binary.
 *
 * @param inPointer		pointer to store.
 * @return a binary holding this pointer.
 */
newtRef
PointerToBinary(void* inPointer)
{
	return NewtMakeBinary(
				kNewtRefUnbind,
				(uint8_t *)&inPointer,
				sizeof(inPointer),
				false);
}

/**
 * Convert a reference to a double.
 * If it's a real, return the real value.
 * If it's an integer, return the integer value.
 * If it's a character, return its value.
 * If it's TRUE, return 1.
 * If it's NIL, return 0.
 * Otherwise, throws kNErrNotAReal
 *
 * @param inRef		reference to the NewtonScript object
 * @return the value of the reference.
 */
double
RefToDoubleConverting(newtRefVar inRef)
{
	double theResult = 0;
	if (NewtRefIsReal(inRef))
	{
		theResult = NewtRefToReal(inRef);
	} else if (NewtRefIsInteger(inRef)) {
		theResult = (double) NewtRefToInteger(inRef);
	} else if (NewtRefIsCharacter(inRef)) {
		theResult = NewtRefToCharacter(inRef);
	} else {
		(void) NewtThrow(kNErrNotAReal, inRef);
	}
	
	return theResult;
}

/**
 * Convert a reference to an integer.
 * If it's a real, return the real value.
 * If it's an integer, return the integer value.
 * If it's a character, return its value.
 * Otherwise, throws kNErrNotAnInteger
 *
 * @param inRef		reference to the NewtonScript object
 * @return the value of the reference.
 */
int
RefToIntConverting(newtRefVar inRef)
{
	int theResult = 0;
	if (NewtRefIsReal(inRef))
	{
		theResult = (int) NewtRefToReal(inRef);
	} else if (NewtRefIsInteger(inRef)) {
		theResult = NewtRefToInteger(inRef);
	} else if (NewtRefIsCharacter(inRef)) {
		theResult = NewtRefToCharacter(inRef);
	} else {
		(void) NewtThrow(kNErrNotAnInteger, inRef);
	}
	
	return theResult;
}

/**
 * Convert a reference to a character.
 * If it's a real, return the real value.
 * If it's an integer, return the integer value.
 * If it's a character, return its value.
 * If it's TRUE, return 1.
 * If it's NIL, return 0.
 * Otherwise, throws kNErrNotACharacter
 *
 * @param inRef		reference to the NewtonScript object
 * @return the value of the reference.
 */
char
RefToCharConverting(newtRefVar inRef)
{
	char theResult = 0;
	if (NewtRefIsReal(inRef))
	{
		theResult = (char) NewtRefToReal(inRef);
	} else if (NewtRefIsInteger(inRef)) {
		theResult = (char) NewtRefToInteger(inRef);
	} else if (NewtRefIsCharacter(inRef)) {
		theResult = (char) NewtRefToCharacter(inRef);
	} else if (inRef == kNewtRefTRUE) {
		theResult = 1;
	} else if (inRef == kNewtRefNIL) {
		theResult = 0;
	} else {
		(void) NewtThrow(kNErrNotACharacter, inRef);
	}
	
	return theResult;
}

/**
 * Convert a reference (frame) to a NSRange.
 *
 * @param inRef			reference to the structure.
 * @param outRange		pointer to the range.
 */
void
RefToRange(newtRefArg inRef, NSRange* outRange)
{
	if (NewtRefIsFrame(inRef)) {
		outRange->location =
			RefToIntConverting(NcGetSlot(inRef, NSSYM(location)));
		outRange->length =
			RefToIntConverting(NcGetSlot(inRef, NSSYM(length)));
	} else {
		(void) NewtThrow(kNErrNotAFrame, inRef);
	}				
}

#if !TARGET_OS_IPHONE
/**
 * Convert a reference (frame) to a NSPoint.
 *
 * @param inRef			reference to the structure.
 * @param outPoint		pointer to the point.
 */
void
RefToNSPoint(newtRefArg inRef, NSPoint* outPoint)
{
	if (NewtRefIsFrame(inRef)) {
		outPoint->x =
    RefToDoubleConverting(NcGetSlot(inRef, NSSYM(x)));
		outPoint->y =
    RefToDoubleConverting(NcGetSlot(inRef, NSSYM(y)));
	} else {
		(void) NewtThrow(kNErrNotAFrame, inRef);
	}				
}

/**
 * Convert a reference (frame) to a NSRect.
 *
 * @param inRef			reference to the structure.
 * @param outRect		pointer to the rect.
 */
void
RefToNSRect(newtRefArg inRef, NSRect* outRect)
{
	if (NewtRefIsFrame(inRef)) {
		float top = RefToDoubleConverting(NcGetSlot(inRef, NSSYM(top)));
		float left = RefToDoubleConverting(NcGetSlot(inRef, NSSYM(left)));
		float bottom = RefToDoubleConverting(NcGetSlot(inRef, NSSYM(bottom)));
		float right = RefToDoubleConverting(NcGetSlot(inRef, NSSYM(right)));
		*outRect = NSMakeRect(left, top, right - left, bottom - top);
	} else {
		(void) NewtThrow(kNErrNotAFrame, inRef);
	}				
}

/**
 * Convert a reference (frame) to a NSSize.
 *
 * @param inRef			reference to the structure.
 * @param outSize		pointer to the size.
 */
void
RefToNSSize(newtRefArg inRef, NSSize* outSize)
{
	if (NewtRefIsFrame(inRef)) {
		outSize->width =
			RefToDoubleConverting(NcGetSlot(inRef, NSSYM(width)));
		outSize->height =
			RefToDoubleConverting(NcGetSlot(inRef, NSSYM(height)));
	} else {
		(void) NewtThrow(kNErrNotAFrame, inRef);
	}				
}
#endif

/**
 * Convert a reference (frame) to a CGPoint.
 *
 * @param inRef			reference to the structure.
 * @param outPoint		pointer to the point.
 */
void
RefToCGPoint(newtRefArg inRef, CGPoint* outPoint)
{
	if (NewtRefIsFrame(inRef)) {
		outPoint->x =
    RefToDoubleConverting(NcGetSlot(inRef, NSSYM(x)));
		outPoint->y =
    RefToDoubleConverting(NcGetSlot(inRef, NSSYM(y)));
	} else {
		(void) NewtThrow(kNErrNotAFrame, inRef);
	}				
}

/**
 * Convert a reference (frame) to a CGRect.
 *
 * @param inRef			reference to the structure.
 * @param outRect		pointer to the rect.
 */
void
RefToCGRect(newtRefArg inRef, CGRect* outRect)
{
	if (NewtRefIsFrame(inRef)) {
		float top = RefToDoubleConverting(NcGetSlot(inRef, NSSYM(top)));
		float left = RefToDoubleConverting(NcGetSlot(inRef, NSSYM(left)));
		float bottom = RefToDoubleConverting(NcGetSlot(inRef, NSSYM(bottom)));
		float right = RefToDoubleConverting(NcGetSlot(inRef, NSSYM(right)));
		*outRect = CGRectMake(left, top, right - left, bottom - top);
	} else {
		(void) NewtThrow(kNErrNotAFrame, inRef);
	}				
}

/**
 * Convert a reference (frame) to a CGSize.
 *
 * @param inRef			reference to the structure.
 * @param outSize		pointer to the size.
 */
void
RefToCGSize(newtRefArg inRef, CGSize* outSize)
{
	if (NewtRefIsFrame(inRef)) {
		outSize->width =
    RefToDoubleConverting(NcGetSlot(inRef, NSSYM(width)));
		outSize->height =
    RefToDoubleConverting(NcGetSlot(inRef, NSSYM(height)));
	} else {
		(void) NewtThrow(kNErrNotAFrame, inRef);
	}				
}

/**
 * Convert a NSRange to a frame.
 *
 * @param inRange		the range.
 * @return a frame.
 */
newtRef
RangeToRef(NSRange inRange)
{
	newtRefVar theFrame = NcMakeFrame();
	NcSetSlot(theFrame, NSSYM(location), NewtMakeInteger(inRange.location));
	NcSetSlot(theFrame, NSSYM(length), NewtMakeInteger(inRange.length));
	return theFrame;
}

#if !TARGET_OS_IPHONE
/**
 * Convert a NSPoint to a frame.
 *
 * @param inPoint		the point.
 * @return a frame.
 */
newtRef
NSPointToRef(NSPoint inPoint)
{
	newtRefVar theFrame = NcMakeFrame();
	NcSetSlot(theFrame, NSSYM(x), NewtMakeReal(inPoint.x));
	NcSetSlot(theFrame, NSSYM(y), NewtMakeReal(inPoint.y));
	return theFrame;
}

/**
 * Convert a NSRect to a frame.
 *
 * @param inRect		the rect.
 * @return a frame.
 */
newtRef
NSRectToRef(NSRect inRect)
{
	newtRefVar theFrame = NcMakeFrame();
	NcSetSlot(theFrame, NSSYM(top), NewtMakeReal(NSMinY(inRect)));
	NcSetSlot(theFrame, NSSYM(left), NewtMakeReal(NSMinX(inRect)));
	NcSetSlot(theFrame, NSSYM(bottom), NewtMakeReal(NSMaxY(inRect)));
	NcSetSlot(theFrame, NSSYM(right), NewtMakeReal(NSMaxX(inRect)));
	return theFrame;
}

/**
 * Convert a NSSize to a frame.
 *
 * @param inPoint		the point.
 * @return a frame.
 */
newtRef
NSSizeToRef(NSSize inSize)
{
	newtRefVar theFrame = NcMakeFrame();
	NcSetSlot(theFrame, NSSYM(width), NewtMakeReal(inSize.width));
	NcSetSlot(theFrame, NSSYM(height), NewtMakeReal(inSize.height));
	return theFrame;
}
#endif
/**
 * Convert a CGPoint to a frame.
 *
 * @param inPoint		the point.
 * @return a frame.
 */
newtRef
CGPointToRef(CGPoint inPoint)
{
	newtRefVar theFrame = NcMakeFrame();
	NcSetSlot(theFrame, NSSYM(x), NewtMakeReal(inPoint.x));
	NcSetSlot(theFrame, NSSYM(y), NewtMakeReal(inPoint.y));
	return theFrame;
}

/**
 * Convert a CGRect to a frame.
 *
 * @param inRect		the rect.
 * @return a frame.
 */
newtRef
CGRectToRef(CGRect inRect)
{
	newtRefVar theFrame = NcMakeFrame();
	NcSetSlot(theFrame, NSSYM(top), NewtMakeReal(CGRectGetMinY(inRect)));
	NcSetSlot(theFrame, NSSYM(left), NewtMakeReal(CGRectGetMinX(inRect)));
	NcSetSlot(theFrame, NSSYM(bottom), NewtMakeReal(CGRectGetMaxY(inRect)));
	NcSetSlot(theFrame, NSSYM(right), NewtMakeReal(CGRectGetMaxX(inRect)));
	return theFrame;
}

/**
 * Convert a CGSize to a frame.
 *
 * @param inPoint		the point.
 * @return a frame.
 */
newtRef
CGSizeToRef(CGSize inSize)
{
	newtRefVar theFrame = NcMakeFrame();
	NcSetSlot(theFrame, NSSYM(width), NewtMakeReal(inSize.width));
	NcSetSlot(theFrame, NSSYM(height), NewtMakeReal(inSize.height));
	return theFrame;
}

/**
 * From an ObjC selector name, return a NewtonScript name, and reciprocally.
 * Replace : by _ and _ by :.
 *
 * @param inName		ObjC or NS name.
 * @return a new allocated pointer to a NS or ObjC name.
 */
char*
ObjCNSNameTranslation(const char* inName)
{
	size_t theLength = strlen(inName) + 1;
	char* theResult = malloc(theLength);
	size_t index;
	for (index = 0; index < theLength; index++)
	{
		char theChar = inName[index];
		if (theChar == ':')
		{
			theChar = '_';
		} else if (theChar == '_') {
			theChar = ':';
		}
		theResult[index] = theChar;
	}
	
	return theResult;
}

/**
 * Cast an ObjectiveC value to a NewtonScript object.
 */
newtRef
CastToNS(id* inObjCValuePtr, const char* inType)
{
//	printf( "Casting result to NS (%s)\n", inType );
	newtRefVar result = kNewtRefNIL;
	switch (inType[0])
	{
		case 'r':	// const
			return CastToNS(inObjCValuePtr, &inType[1]);
			
		case '@':
		{
			id theObject = *inObjCValuePtr;
			result = CastIdToNS(theObject);
		}
		break;

		case '#':
		{
			Class theClass = (Class)(*inObjCValuePtr);
			if (theClass)
			{
				[((id) theClass) retain];
	
				// Get the class
				result =
					GetClassFromName(theClass->name);
			}
		}
		break;
		
		case ':':
		{
			// char
			SEL theSEL = *((SEL*)inObjCValuePtr);
			char* theNSName = ObjCNSNameTranslation(sel_getName(theSEL));
			result = NewtMakeSymbol(theNSName);
			free(theNSName);
		}
		break;

		case 'c':
		case 'C':
			// char
			result = NewtMakeCharacter((char)(uintptr_t)(*inObjCValuePtr));
			break;

		case 's':
		case 'S':
			// short
			result = NewtMakeInteger((short)(uintptr_t)(*inObjCValuePtr));
			break;

		case 'i':
		case 'I':
			// int
			result = NewtMakeInteger((int)(uintptr_t)(*inObjCValuePtr));
			break;

		case 'l':
		case 'L':
			// long
			result = NewtMakeInteger((long)(*inObjCValuePtr));
			break;

		case 'f':
			// float
			printf( "CastToNS: unhandled type: %s (float)\n", inType );
			break;

		case 'd':
			// double
			printf( "CastToNS: unhandled type: %s (double)\n", inType );
			break;

		case 'b':
			// BFLD
			printf( "unhandled type: %s (BFLD)\n", inType );
			break;

		case 'v':
			// void
			break;

		case '?':
			// UNDEF
			printf( "unhandled type: %s (UNDEF)\n", inType );
			break;

		case '^':	// pointer
			result = CastStructToNS((void*) *inObjCValuePtr, &inType[1]);
			break;

		case '*':	// char*
			result = NewtMakeString((const char*)(*inObjCValuePtr), false);
			break;

		default:
			printf( "CastToNS: Unhandled type %s\n", inType );
	}
	
	return result;
}

/**
 * Cast an ObjectiveC structure to a NewtonScript object.
 * This method calls the previous method on basic types.
 *
 * @param inObjCStructPtr	pointer to the structure
 * @param inType			ObjC type
 */
newtRef
CastStructToNS(void* inObjCStructPtr, const char* inType)
{
	newtRefVar result = kNewtRefNIL;
	switch (inType[0])
	{
		case 'r':	// const
			return CastStructToNS(inObjCStructPtr, &inType[1]);
		
		case 'f':
			// float
			result = NewtMakeReal(*((float*) inObjCStructPtr));
			break;

		case 'd':
			// double
			result = NewtMakeReal(*((double*) inObjCStructPtr));
			break;

		default:
			result = CastToNS((id*) inObjCStructPtr, inType);
	}
	
	return result;
}

/**
 * Cast an ObjectiveC value to a NewtonScript object.
 */
newtRef
CastIdToNS(id inObjCObject)
{
	newtRefVar result = kNewtRefNIL;
	if (inObjCObject)
	{
		if ([inObjCObject isKindOfClass: [NSString class]])
		{
			// Bridge strings.
			result = NewtMakeString([((NSString*) inObjCObject) UTF8String], false);
		} else {
			// Get the class
			newtRefVar theClass =
				GetClassFromName(inObjCObject->isa->name);
			if (NewtRefIsNIL(theClass))
			{
				return theClass;
			}
			
			// If the class is a custom class, get the _ns variable.
			if (!NewtRefIsNIL(NcGetSlot(theClass, NSSYM(_nsDefined))))
			{
				// That's a custom class.
				// We can get the single NS object associated with this
				// ObjC object.
				result = GetInstanceFrame(inObjCObject);
			} else {
				[inObjCObject retain];
			
				// Create a new NS object to represent this object.
				// (remark: several objects may exist to represent
				// the same ObjC object)
				
				// Create a new frame
				result = NcMakeFrame();
				
				// Store the id
				NcSetSlot(result, NSSYM(_self), PointerToBinary(inObjCObject));
				
				// Store the class
				NcSetSlot(result, NSSYM(_class), theClass);
	
				// Set _proto to the instance methods
				NcSetSlot(result,
					NSSYM(_proto), NcGetSlot(theClass, NSSYM(_instanceMethods)));
	
				// Add _parent
				NcSetSlot(
					result,
					NSSYM(_parent),
					NcResolveMagicPointer(NewtSymbolToMP(kInstanceParentMagicPtrKey)));
			}
		}
	}
	
	return result;
}

/**
 * Cast ObjectiveC parameters to NewtonScript objects (in an array).
 */
newtRef
CastParamsToNS(va_list inArgList, Method inMethod)
{
	// Create the array with the arguments.
	int nbArgs = method_getNumberOfArguments(inMethod) - 2;
	newtRefVar theArguments = NewtMakeArray(kNewtRefNIL, nbArgs);
	int indexArgs;
	for (indexArgs = 0; indexArgs < nbArgs; indexArgs++)
	{
		const char* theType;
		int theOffset;
		(void) method_getArgumentInfo(
							inMethod,
							indexArgs + 2,
							&theType,
							&theOffset );

		newtRefVar theParam = kNewtRefNIL;

		while (theType[0] == 'r')
		{
			// const
			theType++;
		}
		switch (theType[0])
		{
			case '@':
			{
				id theObject = va_arg(inArgList, id);
				theParam = CastIdToNS(theObject);
			}
			break;
	
			case '#':
			{
				Class theClass = va_arg(inArgList, Class);
				if (theClass)
				{
					[((id) theClass) retain];
		
					// Get the class
					theParam =
						GetClassFromName(theClass->name);
				}
			}
			break;
			
			case ':':
			{
				// char
				SEL theSEL = va_arg(inArgList, SEL);
				char* theNSName = ObjCNSNameTranslation(sel_getName(theSEL));
				theParam = NewtMakeSymbol(theNSName);
				free(theNSName);
			}
			break;
	
			case 'c':
			case 'C':
			{
				// char
				char theChar = (char) va_arg(inArgList, int /* char */);
				theParam = NewtMakeCharacter(theChar);
			}
			break;
	
			case 's':
			case 'S':
			{
				// short
				short theShort = (short) va_arg(inArgList, int /* short */);
				theParam = NewtMakeInteger(theShort);
			}
			break;
	
			case 'i':
			case 'I':
			{
				// int
				int theInt = va_arg(inArgList, int);
				theParam = NewtMakeInteger(theInt);
			}
			break;
	
			case 'l':
			case 'L':
			{
				// long
				long theLong = va_arg(inArgList, long);
				theParam = NewtMakeInteger(theLong);
			}
			break;
	
			case 'f':
			{
				// float
				float theFloat = (float) va_arg(inArgList, double /* float */);
				theParam = NewtMakeReal(theFloat);
			}
			break;
	
			case 'd':
			{
				// double
				double theDouble = va_arg(inArgList, double);
				theParam = NewtMakeReal(theDouble);
			}
			break;
	
			case 'b':
				// BFLD
				printf( "unhandled type: %s (BFLD)\n", theType );
				break;
	
			case 'v':
				// void
				break;
	
			case '?':
				// UNDEF
				printf( "unhandled type: %s (UNDEF)\n", theType );
				break;
	
			case '^':	// pointer
				printf( "unhandled type: %s (POINTER)\n", theType );
				break;
	
			case '*':	// char*
			{
				// char*
				char* theString = va_arg(inArgList, char*);
				theParam = NewtMakeString(theString, false);
			}
			break;
	
			default:
				printf( "CastParamToNS: Unhandled type %s\n", theType );
		}
	
		NewtSetArraySlot(
			theArguments,
			indexArgs,
			theParam);
	}
	
	return theArguments;
}

/**
 * Cast a NewtonScript object to an Object C value.
 */
void
CastParamToObjC(
	void* marg_list,
	int inOffset,
	const char* inType,
	void** outTempStorage,
	newtRefArg inObject,
	int inConst)
{
	switch (inType[0])
	{
		case 'r':	// const
			CastParamToObjC(marg_list, inOffset, &inType[1], outTempStorage, inObject, true);
			break;

		case '@':
			if (NewtRefIsString(inObject))
			{
				// bridge strings.
				marg_setValue(
					marg_list,
					inOffset,
					id,
					[NSString stringWithCString: NewtRefToString(inObject)]);
			} else if (NewtRefIsFrame(inObject)) {
				newtRefVar theBinary = NcGetSlot(inObject, NSSYM(_self));
				if (NewtRefIsBinary(theBinary))
				{
					marg_setValue(
						marg_list, inOffset,
						id, BinaryToPointer(theBinary));
				} else {
					(void) NewtThrow(kNErrNotABinaryObject, theBinary);
				}
			} else if (NewtRefIsSymbol(inObject)) {
				// bridge symbols.
				char* theObjCName =
					ObjCNSNameTranslation(NewtSymbolGetName(inObject));
				SEL theSEL = sel_registerName(theObjCName);
				free(theObjCName);
				marg_setValue(marg_list, inOffset, SEL, theSEL);
			} else {
				(void) NewtThrow(kNErrNotAFrame, inObject);
			}
			break;

		case '#':
			if (NewtRefIsFrame(inObject))
			{
				newtRefVar theSelfSlot = NcGetSlot(inObject, NSSYM(_self));
				if (NewtRefIsBinary(theSelfSlot))
				{
					marg_setValue(
						marg_list, inOffset,
						id, BinaryToPointer(theSelfSlot));
				} else {
					(void) NewtThrow(kNErrNotABinaryObject, theSelfSlot);
				}
			} else {
				(void) NewtThrow(kNErrNotAFrame, inObject);
			}
			break;
		
		case ':':
			// selector
			if (NewtRefIsSymbol(inObject))
			{
				char* theObjCName =
					ObjCNSNameTranslation(NewtSymbolGetName(inObject));
				SEL theSEL = sel_registerName(theObjCName);
				free(theObjCName);
				marg_setValue(marg_list, inOffset, SEL, theSEL);
			} else {
				(void) NewtThrow(kNErrNotASymbol, inObject);
			}
		break;

		case 'c':
		case 'C':
			// char
			marg_setValue(marg_list, inOffset, char, (char) RefToCharConverting(inObject));
			break;

		case 's':
		case 'S':
			// short
			marg_setValue(marg_list, inOffset, short, (short) RefToIntConverting(inObject));
			break;

		case 'i':
		case 'I':
			// int
			marg_setValue(marg_list, inOffset, int, (int) RefToIntConverting(inObject));
			break;

		case 'l':
		case 'L':
			// long
			marg_setValue(marg_list, inOffset, long, (long) RefToIntConverting(inObject));
			break;

		case 'f':
#if !defined(__ppc__) && !defined(ppc)
			// float
			marg_setValue(marg_list, inOffset, float, (float) RefToDoubleConverting(inObject));
			break;
#endif

		case 'd':
			// double
			marg_setValue(marg_list, inOffset, double, (double) RefToDoubleConverting(inObject));
			break;

		case 'b':
			// BFLD
			printf( "unhandled type: %s (BFLD)\n", inType );
			break;

		case 'v':
			// void
			break;

		case '?':
			// UNDEF
			printf( "unhandled type: %s (UNDEF)\n", inType );
			break;

		case '^':	// pointer
			// POINTER
			printf( "unhandled type: %s (POINTER)\n", inType );
			break;

		case '*':	// char*
			if (NewtRefIsString(inObject))
			{
				// We don't need to convert yet.
				char* theString = NewtRefToString(inObject);
				marg_setValue(marg_list, inOffset, const char*, (const char*) theString);
			} else {
				(void) NewtThrow(kNErrNotAString, inObject);
			}
			break;

		case '{':	// structure.
			if ((strncmp(inType, "{_NSRange}", 10) == 0)
				|| (strncmp(inType, "{_NSRange=", 10) == 0))
			{
				NSRange theRange;
				RefToRange(inObject, &theRange);
				marg_setValue(marg_list, inOffset, NSRange, theRange);
#if !TARGET_OS_IPHONE
			} else if ((strncmp(inType, "{_NSRect}", 9) == 0)
				|| (strncmp(inType, "{_NSRect=", 9) == 0)) {
				NSRect theRect;
				RefToNSRect(inObject, &theRect);
				marg_setValue(marg_list, inOffset, NSRect, theRect);
			} else if ((strncmp(inType, "{_NSPoint}", 10) == 0)
				|| (strncmp(inType, "{_NSPoint=", 10) == 0)) {
				NSPoint thePoint;
				RefToNSPoint(inObject, &thePoint);
				marg_setValue(marg_list, inOffset, NSPoint, thePoint);
			} else if ((strncmp(inType, "{_NSSize}", 9) == 0)
				|| (strncmp(inType, "{_NSSize=", 9) == 0)) {
				NSSize theSize;
				RefToNSSize(inObject, &theSize);
				marg_setValue(marg_list, inOffset, NSSize, theSize);
#endif
      } else if ((strncmp(inType, "{_CGRect}", 9) == 0)
                 || (strncmp(inType, "{_CGRect=", 9) == 0)) {
				CGRect theRect;
				RefToCGRect(inObject, &theRect);
				marg_setValue(marg_list, inOffset, CGRect, theRect);
			} else if ((strncmp(inType, "{_CGPoint}", 10) == 0)
                 || (strncmp(inType, "{_CGPoint=", 10) == 0)) {
				CGPoint thePoint;
				RefToCGPoint(inObject, &thePoint);
				marg_setValue(marg_list, inOffset, CGPoint, thePoint);
			} else if ((strncmp(inType, "{_CGSize}", 9) == 0)
                 || (strncmp(inType, "{_CGSize=", 9) == 0)) {
				CGSize theSize;
				RefToCGSize(inObject, &theSize);
				marg_setValue(marg_list, inOffset, CGSize, theSize);
			} else {
				printf( "unhandled type: %s (structure)\n", inType );
			}
			break;

		default:
			printf( "unhandled type: %s (default)\n", inType );
			break;
	}
}

/**
 * Cast a NewtonScript object to an Object C result.
 */
id
CastResultToObjC(
	const char* inType,
	newtRefArg inObject)
{
	switch (inType[0])
	{
		case 'r':	// const
			CastResultToObjC(&inType[1], inObject);
			break;

		case '@':
			if (NewtRefIsString(inObject))
			{
				// bridge strings.
				return [NSString stringWithCString: NewtRefToString(inObject)];
			} else if (NewtRefIsFrame(inObject)) {
				newtRefVar theBinary = NcGetSlot(inObject, NSSYM(_self));
				if (NewtRefIsBinary(theBinary))
				{
					return (id) BinaryToPointer(theBinary);
				} else {
					(void) NewtThrow(kNErrNotABinaryObject, theBinary);
				}
			} else if (NewtRefIsSymbol(inObject)) {
				// bridge symbols.
				char* theObjCName =
					ObjCNSNameTranslation(NewtSymbolGetName(inObject));
				SEL theSEL = sel_registerName(theObjCName);
				free(theObjCName);
				return (id) theSEL;
			} else {
				(void) NewtThrow(kNErrNotAFrame, inObject);
			}
			break;

		case '#':
			if (NewtRefIsFrame(inObject))
			{
				newtRefVar theSelfSlot = NcGetSlot(inObject, NSSYM(_self));
				if (NewtRefIsBinary(theSelfSlot))
				{
					return (id) BinaryToPointer(theSelfSlot);
				} else {
					(void) NewtThrow(kNErrNotABinaryObject, theSelfSlot);
				}
			} else {
				(void) NewtThrow(kNErrNotAFrame, inObject);
			}
			break;
		
		case ':':
			// selector
			if (NewtRefIsSymbol(inObject))
			{
				char* theObjCName =
					ObjCNSNameTranslation(NewtSymbolGetName(inObject));
				SEL theSEL = sel_registerName(theObjCName);
				free(theObjCName);
				return (id) theSEL;
			} else {
				(void) NewtThrow(kNErrNotASymbol, inObject);
			}
		break;

		case 'c':
		case 'C':
			// char
			return (id) (int) RefToCharConverting(inObject);
			break;

		case 's':
		case 'S':
			// short
			return (id) (int) (short) RefToIntConverting(inObject);
			break;

		case 'i':
		case 'I':
			// int
			return (id) (int) RefToIntConverting(inObject);
			break;

		case 'l':
		case 'L':
			// long
			return (id) (long) RefToIntConverting(inObject);
			break;

		case 'f':
			printf( "unhandled type: %s (float)\n", inType );
			break;

		case 'd':
			printf( "unhandled type: %s (double)\n", inType );
			break;

		case 'b':
			// BFLD
			printf( "unhandled type: %s (BFLD)\n", inType );
			break;

		case 'v':
			// void
			return NULL;
			break;

		case '?':
			// UNDEF
			printf( "unhandled type: %s (UNDEF)\n", inType );
			break;

		case '^':	// pointer
			// POINTER
			printf( "unhandled type: %s (POINTER)\n", inType );
			break;

		case '*':	// char*
			if (NewtRefIsString(inObject))
			{
				// We don't need to convert yet.
				char* theString = NewtRefToString(inObject);
				return (id) theString;
			} else {
				(void) NewtThrow(kNErrNotAString, inObject);
			}
			break;

		default:
			printf( "unhandled type: %s (default)\n", inType );
			break;
	}
	
	return NULL;
}

/**
 * Lock an object to prevent the garbage collector from releasing it.
 * This currently works by adding the object to an array.
 *
 * @param inObject		object to lock.
 */
void
Lock(newtRefArg inObject)
{
	newtRefVar theList = NcGetGlobalVar(NSSYM(_locked));
	if (!NewtRefIsArray(theList))
	{
		theList = NewtMakeArray(kNewtRefNIL, 0);
		NcSetGlobalVar(NSSYM(_locked), theList);
	}
	
	NcAddArraySlot(theList, inObject);
}

/**
 * Cast a NS exception to an ObjC exception.
 */
NSException*
CastExToObjC(newtRefArg inNSException)
{
	NSException* theObjCException = NULL;
	
	if (NewtRefEqual(
			NcGetSlot(inNSException, NSSYM(name)),
			kNErrObjCExceptionName))
	{
		newtRefVar theDataFrame = NcGetSlot(inNSException, NSSYM(data));
		if (NewtRefIsFrame(theDataFrame))
		{
			newtRefVar theBinary = NcGetSlot(theDataFrame, NSSYM(_obj));
			if (NewtRefIsBinary(theBinary))
			{
				theObjCException = (NSException*) BinaryToPointer(theBinary);
			}
		}
		
		if (theObjCException == NULL)
		{
			theObjCException =
				[NSException
					exceptionWithName:@"UnknownObjCException"
					reason:@"Can't find exception data"
					userInfo:nil];
		}
	} else {
		NSDictionary* theDict =
			[NSDictionary dictionaryWithObject: [
				TNewtObjCRef refWithRef: inNSException] forKey: @"NSException"];
	
		theObjCException =
			[NSException
				exceptionWithName: kNSExceptionObjCName
				reason:[
					NSString
						stringWithFormat: @"NewtonScript Exception (%s)",
						NewtSymbolGetName(NcGetSlot(inNSException, NSSYM(name)))]
				userInfo: theDict];
	}
	
	return theObjCException;
}

/**
 * Cast an ObjC exception to a NS exception.
 */
newtRef
CastExToNS(NSException* inObjCException)
{
	newtRefVar theNSException = kNewtRefNIL;

	if ([[inObjCException name] isEqualToString: kNSExceptionObjCName])
	{
		TNewtObjCRef* theNSExceptionRef =
			(TNewtObjCRef*) [
				[inObjCException userInfo] objectForKey: @"NSException"];
		theNSException = [theNSExceptionRef ref];
	} else {
		newtRefVar theExceptionDataFrame = NcMakeFrame();
		NcSetSlot(theExceptionDataFrame, NSSYM(_obj), CastIdToNS(inObjCException));
		NcSetSlot(theExceptionDataFrame, NSSYM(name), CastIdToNS([inObjCException name]));
		NcSetSlot(theExceptionDataFrame, NSSYM(reason), CastIdToNS([inObjCException reason]));
		theNSException = NcMakeFrame();
		NcSetSlot(theNSException, NSSYM(data), theExceptionDataFrame);
		NcSetSlot(theNSException, NSSYM(name), kNErrObjCExceptionName);
	}
	
	return theNSException;
}

// ========================================= //
// An algorithm must be seen to be believed. //
//                 -- D.E. Knuth             //
// ========================================= //
