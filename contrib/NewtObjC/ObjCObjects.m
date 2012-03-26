/// ==============================
/// @file			ObjCObjects.m
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
/// The Original Code is ObjCObjects.m.
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

#import "ObjCObjects.h"

// ANSI C & POSIX
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>

// ObjC & Cocoa
#include <objc/objc.h>
#include <objc/objc-class.h>
#include <objc/objc-runtime.h>
#include <Foundation/Foundation.h>
#include "objc-runtime-x/objc-runtime-x.h"

// NEWT/0
#include "NewtCore.h"
#include "NewtVM.h"

// NewtObjC
#import "Constants.h"
#import "TNewtObjCRef.h"
#import "Utils.h"

// Additional prototypes
newtRef GenericMethod(newtRef, newtRef);
void CreateInstanceFrame(id);
void ObjCInitializeMethod(id, SEL);
id ObjCAllocMethod(id, SEL);
id ObjCAllocWithZoneMethod(id, SEL, NSZone*);
newtRef CreateClassObjectMethods(Class inObjCClass);

/**
 * Accessor to an instance variable.
 *
 * @param inRcvr	self
 * @param inName	name of the variable to retrieve.
 * @return the value of the instance variable or nil if it doesn't exist.
 */
newtRef
GetInstanceVariable(newtRef inRcvr, newtRef inName)
{
	if (!NewtRefIsFrame(inRcvr))
	{
		return NewtThrow(kNErrNotAFrame, inRcvr);
	}
	if (!NewtRefIsString(inName))
	{
		return NewtThrow(kNErrNotAString, inName);
	}

	newtRefVar theResult = kNewtRefNIL;
	id theObjCObject = (id) BinaryToPointer(NcGetSlot(inRcvr, NSSYM(_self)));
	Ivar theVarDef = class_getInstanceVariable(
						theObjCObject->isa,
						NewtRefToString(inName));
	if (theVarDef)
	{
		void* theVariable = (void*) (((uintptr_t) theObjCObject) + theVarDef->ivar_offset);
		theResult = CastStructToNS(theVariable, theVarDef->ivar_type);
	}
	
	return theResult;
}

/**
 * Determine if an instance variable exists.
 *
 * @param inRcvr	self
 * @param inName	name of the variable to retrieve.
 * @return true if the variable exists, false otherwise.
 */
newtRef
InstanceVariableExists(newtRef inRcvr, newtRef inName)
{
	if (!NewtRefIsFrame(inRcvr))
	{
		return NewtThrow(kNErrNotAFrame, inRcvr);
	}
	if (!NewtRefIsString(inName))
	{
		return NewtThrow(kNErrNotAString, inName);
	}
	
	newtRefVar theResult = kNewtRefNIL;
	id theObjCObject = (id) BinaryToPointer(NcGetSlot(inRcvr, NSSYM(_self)));
	Ivar theVarDef = class_getInstanceVariable(
						theObjCObject->isa,
						NewtRefToString(inName));
	if (theVarDef)
	{
		theResult = kNewtRefTRUE;
	}
	
	return theResult;
}

/**
 * Method of the ObjC object to call a NewtonScript-defined method.
 */
id
GenericObjCMethod(id inSelf, SEL _cmd, ...)
{
	newtRefVar theInstance = GetInstanceFrame(inSelf);

	Method theObjCMethod = class_getInstanceMethod(inSelf->isa, _cmd);
	const char* theObjCMethodName = sel_getName(_cmd);

	// Get the NewtonScript method.
	char* theNSMethodName = ObjCNSNameTranslation(theObjCMethodName);
	newtRefVar theMethodSym = NewtMakeSymbol(theNSMethodName);
	free(theNSMethodName);
	newtRefVar theIMPL = theInstance;
	newtRefVar theNSMethod = kNewtRefNIL;
	while (NewtRefIsNotNIL(theIMPL))
	{
		if (NewtHasSlot(theIMPL, theMethodSym))
		{
			theNSMethod = NcGetSlot(theIMPL, theMethodSym);
			break;
		}
		
		theIMPL = NcGetSlot(theIMPL, NSSYM(_proto));
	}

	id objCResult = NULL;
	if (NewtRefIsNotNIL(theNSMethod))
	{
		va_list theStackArgs;	
		va_start(theStackArgs, _cmd);
		newtRefVar theArguments = CastParamsToNS(theStackArgs, theObjCMethod);
		va_end(theStackArgs);

		// Call the function.
		newtRefVar theResult = NVMMessageSendWithArgArray(
			theIMPL, theInstance,
			theNSMethod, theArguments);
		
		// Handle exceptions.
		newtRefVar theNSException = NVMCurrentException();
		NVMClearException();
		if (NewtRefIsNotNIL(theNSException))
		{
			// Throw it as a ObjC exception.
			NSException* theObjCException = CastExToObjC(theNSException);
			[theObjCException raise];	// ciao.
		}
		
		// Cast the result.
		objCResult = CastResultToObjC(theObjCMethod->method_types, theResult);
	} else {
		(void) NewtThrow(kNErrUndefinedMethod, theMethodSym);
		newtRefVar theEx = NVMCurrentException();
		NVMClearException();
		NSException* theException = CastExToObjC(theEx);
		[theException raise];	// ciao.
	}
	
	return objCResult;
}

/**
 * ObjC allocWithZone: method.
 *
 * @param inSelf	pointer to self.
 * @param _cmd		pointer to the command.
 * @param inZone	the zone.
 */
id
ObjCAllocWithZoneMethod(id inSelf, SEL _cmd, NSZone* inZone)
{
	// Call super's allocWithZone:
	struct objc_super superCall;
	superCall.receiver = inSelf;
	superCall.class = inSelf->isa->super_class;
	id theResult = objc_msgSendSuper(&superCall, _cmd, inZone);
	
	// Create the instance frame.
	CreateInstanceFrame(theResult);
	
	return theResult;
}

/**
 * ObjC alloc method.
 *
 * @param inSelf	pointer to self.
 * @param _cmd		pointer to the command.
 */
id
ObjCAllocMethod(id inSelf, SEL _cmd)
{
	// Call super's alloc
	struct objc_super superCall;
	superCall.receiver = inSelf;
	superCall.class = inSelf->isa->super_class;
	id theResult = objc_msgSendSuper(&superCall, _cmd);
	
	// Set the instance frame.
	CreateInstanceFrame(theResult);
	
	return theResult;
}

/**
 * ObjC initialize method.
 *
 * @param inSelf	pointer to self.
 * @param _cmd		pointer to the command.
 */
void
ObjCInitializeMethod(id inSelf, SEL _cmd)
{
	// Grab the class frame.
	newtRefVar theClassFrame = GetClassFromName(((Class) inSelf)->name);
	
	// Set it as the _ns variable the objc object.
	Ivar theVarDef = class_getInstanceVariable(inSelf->isa, kObjCNSFrameVarName);
	*((TNewtObjCRef**) ((uintptr_t) inSelf + theVarDef->ivar_offset)) =
		[TNewtObjCRef refWithRef: theClassFrame];
}

/**
 * Create the instance frame and set the variable on the object accordingly.
 * This is done by getting the class and getting the methods from there.
 * An instance frame is made of:
 * {
 *	_proto: instance methods frame
 *  _parent: utility methods
 *  _self: pointer to the object
 *  _class: class frame
 * }
 */
void
CreateInstanceFrame(id inObjCObject)
{
	// Get a pointer to the _ns variable of the objc object.
	Ivar theVarDef = class_getInstanceVariable(inObjCObject->isa, kObjCNSFrameVarName);
	TNewtObjCRef** theHandle =
		((TNewtObjCRef**) ((uintptr_t) inObjCObject + theVarDef->ivar_offset));

	if (*theHandle == NULL)
	{
		// Create a new frame.
		newtRefVar theObjectFrame = NcMakeFrame();
		
		// Get the class frame.
		newtRefVar theClassFrame = GetClassFromName(inObjCObject->isa->name);
		
		// Get the instance methods.
		newtRefVar instanceMethods =
			NcGetSlot(theClassFrame, NSSYM(_instanceMethods));
		// Set _proto slot to be the instance methods frame.
		NcSetSlot(theObjectFrame, NSSYM(_proto), instanceMethods);
		
		// Store the id in the _self frame.
		NcSetSlot(theObjectFrame, NSSYM(_self), PointerToBinary(inObjCObject));

		// Store the class in the _class frame.
		NcSetSlot(theObjectFrame, NSSYM(_class), theClassFrame);

		// Set _parent to the utility methods.
		NcSetSlot(theObjectFrame, NSSYM(_parent),
			NcResolveMagicPointer(NewtSymbolToMP(kInstanceParentMagicPtrKey)));

		// Save it to the variable.
		*theHandle = [TNewtObjCRef refWithRef: theObjectFrame];
	}
}

/**
 * Get the instance frame from a given object.
 */
newtRef
GetInstanceFrame(id inObject)
{
	// Get a pointer to the _ns variable of the objc object.
	Ivar theVarDef = class_getInstanceVariable(inObject->isa, kObjCNSFrameVarName);
	TNewtObjCRef** theHandle = ((TNewtObjCRef**) ((uintptr_t) inObject + theVarDef->ivar_offset));
	return [*theHandle ref];
}

/**
 * Generic method with any number of argument.
 */
newtRef
GenericMethod(newtRef inRcvr, newtRef inArgs)
{
	// Retrieve the method
	marg_list arguments;
	id objcSelf;
	unsigned nbOfArguments, sizeOfArguments;
	int indexArgs, nbArgsNS;
	struct objc_method* theMethod;
	const char* theType;
	newtRefVar theResultObj = kNewtRefNIL;
	int theOffset;
	id result;
	void** storage;
	newtRefVar theCurrentFunc = NVMCurrentFunction();
	theMethod = (struct objc_method*)
		BinaryToPointer(NcGetSlot(theCurrentFunc, NSSYM(_method)));

	sizeOfArguments = method_getSizeOfArguments(theMethod);
	nbOfArguments = method_getNumberOfArguments(theMethod) - 2;
	nbArgsNS = NewtArrayLength(inArgs);
	if (nbOfArguments != nbArgsNS)
	{
		// Throw an error
		return NewtThrow(
					kNErrWrongNumberOfArgs,
					NcGetSlot(theCurrentFunc, NSSYM(docString)));
	}

	marg_malloc(arguments, theMethod);
	objcSelf = (id) BinaryToPointer(NcGetSlot(inRcvr, NSSYM(_self)));

	// Add both the id and the selector
	(void) method_getArgumentInfo(
							theMethod,
							0,
							&theType,
							&theOffset );
	marg_setValue(arguments, theOffset, id, objcSelf);
	(void) method_getArgumentInfo(
							theMethod,
							1,
							&theType,
							&theOffset );
	marg_setValue(arguments, theOffset, SEL, theMethod->method_name);

	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	// Convert and stuff the arguments
	storage = (void**) malloc( nbOfArguments * sizeof( void * ) );
#if defined(__ppc__) || defined(ppc)
	int indexDoubles = 0;
#endif
	for (indexArgs = 0; indexArgs < nbOfArguments; indexArgs++)
	{
		storage[indexArgs] = NULL;
		(void) method_getArgumentInfo(
							theMethod,
							indexArgs + 2,
							&theType,
							&theOffset );
#if defined(__ppc__) || defined(ppc)
		if (theOffset > 0)
		{
			// Fix offset for float & double parameters.
			if ((theType[0] == 'f') || (theType[0] == 'd'))
			{
				// It's always a double.
				theOffset = (indexDoubles * sizeof(double)) - marg_prearg_size;
				indexDoubles++;
			}
		}
#endif

		CastParamToObjC(
			arguments,
			theOffset,
			theType,
			&storage[indexArgs],
			NewtGetArraySlot(inArgs, indexArgs),
			false);
	}
	
	// Catch exceptions (we'll cast them)
	NS_DURING

	// Determine if the return type is a structure
	theType = theMethod->method_types;
//	printf( "calling %s (type = %s)\n", sel_getName(theMethod->method_name), theType );
	if (theType[0] == '{')
	{
		if ((strncmp(theType, "{_NSRange}", 10) == 0)
			|| (strncmp(theType, "{_NSRange=", 10) == 0))
		{
			NSRange theRange;
			objc_methodCallv_stret(
								&theRange,
								objcSelf,
								theMethod->method_name,
								theMethod->method_imp,
								sizeOfArguments,
								arguments);
			theResultObj = NSRangeToRef(theRange);
		} 
#if !TARGET_OS_IPHONE
    else if ((strncmp(theType, "{_NSRect}", 9) == 0)
			|| (strncmp(theType, "{_NSRect=", 9) == 0)) {
			NSRect theRect;
			objc_methodCallv_stret(
								&theRect,
								objcSelf,
								theMethod->method_name,
								theMethod->method_imp,
								sizeOfArguments,
								arguments);
			theResultObj = NSRectToRef(theRect);
		} else if ((strncmp(theType, "{_NSPoint}", 10) == 0)
			|| (strncmp(theType, "{_NSPoint=", 10) == 0)) {
			NSPoint thePoint;
			objc_methodCallv_stret(
								&thePoint,
								objcSelf,
								theMethod->method_name,
								theMethod->method_imp,
								sizeOfArguments,
								arguments);
			theResultObj = NSPointToRef(thePoint);
		} else if ((strncmp(theType, "{_NSSize}", 9) == 0)
			|| (strncmp(theType, "{_NSSize=", 9) == 0)) {
			NSSize theSize;
			objc_methodCallv_stret(
								&theSize,
								objcSelf,
								theMethod->method_name,
								theMethod->method_imp,
								sizeOfArguments,
								arguments);
			theResultObj = NSSizeToRef(theSize);
		} 
#endif
    else if ((strncmp(theType, "{_CGRect}", 9) == 0)
             || (strncmp(theType, "{_CGRect=", 9) == 0)) {
			CGRect theRect;
			objc_methodCallv_stret(
                             &theRect,
                             objcSelf,
                             theMethod->method_name,
                             theMethod->method_imp,
                             sizeOfArguments,
                             arguments);
			theResultObj = CGRectToRef(theRect);
		} else if ((strncmp(theType, "{_CGPoint}", 10) == 0)
               || (strncmp(theType, "{_CGPoint=", 10) == 0)) {
			CGPoint thePoint;
			objc_methodCallv_stret(
                             &thePoint,
                             objcSelf,
                             theMethod->method_name,
                             theMethod->method_imp,
                             sizeOfArguments,
                             arguments);
			theResultObj = CGPointToRef(thePoint);
		} else if ((strncmp(theType, "{_CGSize}", 9) == 0)
               || (strncmp(theType, "{_CGSize=", 9) == 0)) {
			CGSize theSize;
			objc_methodCallv_stret(
                             &theSize,
                             objcSelf,
                             theMethod->method_name,
                             theMethod->method_imp,
                             sizeOfArguments,
                             arguments);
			theResultObj = CGSizeToRef(theSize);
		} 
    else {
			printf("Unhandled structure %s\n", theType);
			theResultObj = kNewtRefNIL;
		}
	} else if (theType[0] == 'f') {
		float theFloat =
			(*(float(*)(id, SEL, IMP, unsigned, marg_list))objc_methodCallv)(
							objcSelf,
							theMethod->method_name,
							theMethod->method_imp,
							sizeOfArguments,
							arguments);
		// Cast the result
		theResultObj = CastStructToNS(&theFloat, theType);
	} else if (theType[0] == 'd') {
		double theDouble =
			(*(double(*)(id, SEL, IMP, unsigned, marg_list))objc_methodCallv)(
							objcSelf,
							theMethod->method_name,
							theMethod->method_imp,
							sizeOfArguments,
							arguments);
		// Cast the result
		theResultObj = CastStructToNS(&theDouble, theType);		
	} else {
		// Send the message (actually call the method)
		result = objc_methodCallv(
							objcSelf,
							theMethod->method_name,
							theMethod->method_imp,
							sizeOfArguments,
							arguments);
	
		// Cast the result
		theResultObj = CastToNS(&result, theType);
	}

	NS_HANDLER
		// Convert the exception and throw it as a NS exception.
		newtRefVar theExceptionFrame = CastExToNS(localException);
		NVMThrowData(
			NcGetSlot(theExceptionFrame, NSSYM(name)), theExceptionFrame);
		theResultObj = kNewtRefNIL;
	NS_ENDHANDLER

	// Release the temporary storage
	for (indexArgs = 0; indexArgs < nbOfArguments; indexArgs++)
	{
		if (storage[indexArgs])
		{
			free( storage[indexArgs] );
		}
	}
	free( storage );
		
	[pool release];

	return theResultObj;
}

/**
 * Create the NS instance or class methods frame that call the ObjC methods.
 *
 * @param inObjCClass	ObjC class.
 * @return a new frame with the methods.
 */
newtRef
CreateClassObjectMethods(Class inObjCClass)
{
	newtRef result = NcMakeFrame();
	
	void* iterator = 0;
    struct objc_method_list* mlist;

	// Iterate on the methods of the class
	mlist = class_nextMethodList( inObjCClass, &iterator );
	while( mlist != NULL )
	{
		int indexMethods;
		int nbMethods;
		
		nbMethods = mlist->method_count;
		for (indexMethods = 0; indexMethods < nbMethods; indexMethods++)
		{
			newtRefVar nsMethod;
			struct objc_method* theMethod = &mlist->method_list[indexMethods];
			const char* theObjCName = sel_getName(theMethod->method_name);
			char* theNSName = ObjCNSNameTranslation(theObjCName);

			// create the (ns) native method
			nsMethod =
				NewtMakeNativeFunc0(GenericMethod, 0, true, (char*) theNSName);

			// add the objc method ptr itself so the glue will be able to call
			// it
			NcSetSlot(nsMethod, NSSYM(_method), PointerToBinary(theMethod));
			
			// add the method to the frame
			NcSetSlot(result, NewtMakeSymbol(theNSName), nsMethod);
			
			free(theNSName);
		}
		
		// On 10.4, the following lines need to be commented.
		// Not using them creates problem on 10.3
//		if (inObjCClass->info & CLS_METHOD_ARRAY)
//		{
			mlist = class_nextMethodList( inObjCClass, &iterator );
//		} else {
//			break;
//		}
	}
	
	return result;
}

/**
 * Create an object that will respond to class methods.
 * It is built as follows:
 * {
 *	_proto: methods frame
 *	_parent: instance variable access methods
 *  _self: binary to the ObjC object
 *  _instanceMethods: instance methods frame
 * }
 *
 * @param inObjCClass	ObjC class.
 * @return a frame
 */
newtRef
CreateClassObject(Class inObjCClass)
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	// Create the frame
	newtRefVar result = NcMakeFrame();
		
	// Save the self pointer
	NcSetSlot(result, NSSYM(_self), PointerToBinary(inObjCClass));

	// Create the class methods frame.
	newtRefVar classMethodsFrame = CreateClassObjectMethods( inObjCClass->isa );

	// Link it with super class class methods frame.
	struct objc_class* super_class = inObjCClass->super_class;
	newtRefVar superClassFrame = kNewtRefNIL;
	if (super_class)
	{
		superClassFrame = GetClassFromName(super_class->name);
		NcSetSlot(
			classMethodsFrame,
			NSSYM(_proto),
			NcGetSlot(superClassFrame, NSSYM(_proto)));
	}

	// Save it as the _proto slot.
	NcSetSlot(result, NSSYM(_proto), classMethodsFrame);
	
	// Create the instance methods frame.
	newtRefVar instanceMethodsFrame = CreateClassObjectMethods( inObjCClass );

	// Link it with super class instance methods frame.
	if (super_class)
	{
		NcSetSlot(
			instanceMethodsFrame,
			NSSYM(_proto),
			NcGetSlot(superClassFrame, NSSYM(_instanceMethods)));
	}
	
	// Save it as _instanceMethods.
	NcSetSlot(result, NSSYM(_instanceMethods), instanceMethodsFrame);
	
	// Add the utility methods (as _parent slot).
	NcSetSlot(result, NSSYM(_parent),
		NcResolveMagicPointer(NewtSymbolToMP(kInstanceParentMagicPtrKey)));

	[pool release];

	return result;
}

/**
 * Get a reference to a class.
 *
 * @param inName	name of the class (it's a string)
 * @return a reference to a class object.
 */
newtRef
GetClassFromName(const char* inName)
{
	newtRefVar  classes;
	newtRefVar	sym;
	newtRefVar  result = kNewtRefNIL;

	// Check the class isn't known yet.
	classes = NcGetGlobalVar(NSSYM(ObjCClasses));
	
	sym = NewtMakeSymbol(inName);

	if (NewtRefIsNIL(classes))
	{
		classes = NcMakeFrame();
		NcSetGlobalVar(NSSYM(ObjCClasses), classes);
	}
	
	if (NewtHasSlot(classes, sym))
	{
		// It's already known.
		result = NcGetSlot(classes, sym);
	} else {
		// try to load it
		id theObjCClass = objc_getClass(inName);
		if (theObjCClass)
		{
			// Create a new frame
			result = CreateClassObject(theObjCClass);
			// Save it to the frame
			NcSetSlot(classes, sym, result);
		} else {
			result = NewtThrow(kNErrObjCRuntimeErr, NewtMakeString("objc_getClass failed", true));
		}
	}
	
	return result;
}

/**
 * Create the list of variables.
 *
 * @param ioClass		class we're defining (or the metaclass).
 * @param inVariables	list of the variables to define (array).
 */
void
CreateObjCVariables(
	struct objc_class* ioClass,
	newtRefArg inVariables)
{
	// Set the instance variables.
	int nbVars = 0;
	if (!NewtRefIsNIL(inVariables))
	{
		nbVars = NewtArrayLength(inVariables);
	}
	
	struct objc_ivar_list* theIvarlist =
		(struct objc_ivar_list*) malloc( sizeof(struct objc_ivar_list)
			+ ((nbVars /* +1 -1*/) * sizeof(struct objc_ivar)));
	theIvarlist->ivar_count = nbVars + 1;
	
	int ivar_offset = ioClass->super_class->instance_size;

	// Create the variables from the list.
	int indexVars;
	for (indexVars = 0; indexVars < nbVars; indexVars++)
	{
		newtRefVar theNSVarDef = NewtSlotsGetSlot( inVariables, indexVars );
		if (!NewtRefIsFrame(theNSVarDef))
		{
			(void) NewtThrow(kNErrNotAFrame, theNSVarDef);
			theIvarlist->ivar_count = 1;
			nbVars = 0;
			break;
		}
		newtRefVar theName = NcGetSlot(theNSVarDef, NSSYM(name));
		if (!NewtRefIsString(theName))
		{
			(void) NewtThrow(kNErrNotAString, theName);
			theIvarlist->ivar_count = 1;
			nbVars = 0;
			break;
		}
		const char* theNameCStr = NewtRefToString(theName);

		newtRefVar theType = NcGetSlot(theNSVarDef, NSSYM(type));
		if (!NewtRefIsString(theType))
		{
			(void) NewtThrow(kNErrNotAString, theType);
			theIvarlist->ivar_count = 1;
			nbVars = 0;
			break;
		}
		const char* theTypeCStr = NewtRefToString(theType);
		
		if (strcmp(theTypeCStr, kObjCOutletTypeStr) == 0)
		{
			// Copy the name.
			theIvarlist->ivar_list[indexVars].ivar_name = strdup(theNameCStr);
			theIvarlist->ivar_list[indexVars].ivar_type = kObjCOutletTypeStr;
			theIvarlist->ivar_list[indexVars].ivar_offset = ivar_offset;
			ivar_offset += sizeof(id);
		} else {
			fprintf(stderr, "I don't handle types other than kObjCOutletType yet");
			theIvarlist->ivar_count = 1;
			nbVars = 0;
			break;
		}
	}

	// Finish with variable is _ns
	theIvarlist->ivar_list[nbVars].ivar_name = kObjCNSFrameVarName;
	theIvarlist->ivar_list[nbVars].ivar_type = kObjCNSFrameVarType;
	theIvarlist->ivar_list[nbVars].ivar_offset = ivar_offset;
	ivar_offset += sizeof(newtRefVar);

	// Save the pointer.
	ioClass->ivars = theIvarlist;

	// Set the instance size.
	ioClass->instance_size = ivar_offset;
}

/**
 * Create the list of methods.
 *
 * @param ioClass			class we're defining (or the metaclass).
 * @param inMethods			set of the methods to define (frame).
 * @param outSuperMethods	set of super methods that can be called.
 * @param isMetaClass		if we're definining the meta class.
 */
void
CreateObjCMethods(
	struct objc_class* ioClass,
	newtRefArg inMethods,
	int isMetaClass)
{
	ioClass->methodLists =
		(struct objc_method_list**)
			calloc( 2, sizeof(struct objc_method_list*) );
	// Fix for a bug in objc runtime (CLS_METHOD_ARRAY isn't checked).
	ioClass->methodLists[1] = (struct objc_method_list*) -1;

	uint32_t nbMethods = 0;
	uint32_t nbNSMethods = 0;
	uint32_t indexMethods;

	if (!NewtRefIsNIL(inMethods))
	{
		nbNSMethods = NewtFrameLength(inMethods);
	}
	
	if (isMetaClass)
	{
		// I add 3 class methods (+initialize, +alloc and +allocWithZone)
		nbMethods = nbNSMethods + 3;
	} else {
		nbMethods = nbNSMethods;
	}
	
	ioClass->methodLists[0] =
		(struct objc_method_list*)
			malloc( sizeof(struct objc_method_list)
				+ ((nbMethods - 1) * sizeof(struct objc_method)) );
	ioClass->methodLists[0]->obsolete = NULL;
	ioClass->methodLists[0]->method_count = nbMethods;
	struct objc_method* methodsCrsr = (*ioClass->methodLists)->method_list;
	
	// Get the initialize/init method
	SEL theInitMethodSEL = NULL;
	SEL theAllocMethodSEL = NULL;
	SEL theAllocWithZoneMethodSEL = NULL;
	if (isMetaClass)
	{
		theInitMethodSEL = sel_registerName("initialize");
		theAllocMethodSEL = sel_registerName("alloc");
		theAllocWithZoneMethodSEL = sel_registerName("allocWithZone:");
	}
	Method theOriginalMethod;

	for (indexMethods = 0; indexMethods < nbNSMethods; indexMethods++)
	{
		newtRef key = NewtGetFrameKey(inMethods, indexMethods);
		newtRef function = NewtGetFrameSlot(inMethods, indexMethods);
		
		const char* theNSMethodName = NewtSymbolGetName(key);
		char* theObjCMethodName = ObjCNSNameTranslation(theNSMethodName);
		
		// Get the selector
		SEL theMethodSEL = sel_registerName(theObjCMethodName);
		
		free(theObjCMethodName);
		
		if (isMetaClass)
		{
			if ((theMethodSEL == theInitMethodSEL)
				|| (theMethodSEL == theAllocMethodSEL)
				|| (theMethodSEL == theAllocWithZoneMethodSEL))
			{
				// Skip this one.
				(*ioClass->methodLists)->method_count--;
				continue;
			}
		}
		
		// Get the method of super class.
		theOriginalMethod =
			class_getInstanceMethod(ioClass->super_class, theMethodSEL);
		
		if (theOriginalMethod)
		{
			int nbArgsObjC = method_getNumberOfArguments(theOriginalMethod) - 2;
			if (!NVMFuncCheckNumArgs(function, nbArgsObjC))
			{
				(void) NewtThrow( kNErrWrongNumberOfArgs, function );
				// XXX Cleanup
				break;
			}
			
			// I suppose the original method won't go away and therefore
			// I don't need to copy the type.
			methodsCrsr->method_name = theMethodSEL;
			methodsCrsr->method_types = theOriginalMethod->method_types;
			methodsCrsr->method_imp = GenericObjCMethod;
		} else {
			// I need the type (from the function definition).
			newtRef value = NewtGetFrameSlot(inMethods, indexMethods);
			newtRef theType = NcGetSlot(value, NSSYM(objc_type));
			if (NewtRefIsString(theType))
			{
				const char* theTypeString = NewtRefToString(theType);

				methodsCrsr->method_name = theMethodSEL;
				methodsCrsr->method_types = strdup(theTypeString);
				methodsCrsr->method_imp = GenericObjCMethod;
			} else {
				// Use the default type.
				if (NVMFuncCheckNumArgs(function, 0))
				{
					methodsCrsr->method_types = kObjCDefaultFunc0TypeStr;
				} else if (NVMFuncCheckNumArgs(function, 1)) {
					methodsCrsr->method_types = kObjCDefaultFunc1TypeStr;
				} else if (NVMFuncCheckNumArgs(function, 2)) {
					methodsCrsr->method_types = kObjCDefaultFunc2TypeStr;
				} else if (NVMFuncCheckNumArgs(function, 3)) {
					methodsCrsr->method_types = kObjCDefaultFunc3TypeStr;
				} else {
					(void) NewtThrow( kNErrWrongNumberOfArgs, function );
					// XXX Cleanup
					break;
				}
				methodsCrsr->method_name = theMethodSEL;
				methodsCrsr->method_imp = GenericObjCMethod;
			}
		}
		
		methodsCrsr++;
	}

	// Add the initialize, alloc & allocWithZone class method
	if (isMetaClass)
	{
		theOriginalMethod =
			class_getInstanceMethod(ioClass->super_class, theInitMethodSEL);
		methodsCrsr->method_name = theInitMethodSEL;
		methodsCrsr->method_types = theOriginalMethod->method_types;
		methodsCrsr->method_imp = (IMP) ObjCInitializeMethod;
		methodsCrsr++;

		theOriginalMethod =
			class_getInstanceMethod(ioClass->super_class, theAllocMethodSEL);
		methodsCrsr->method_name = theAllocMethodSEL;
		methodsCrsr->method_types = theOriginalMethod->method_types;
		methodsCrsr->method_imp = (IMP) ObjCAllocMethod;
		methodsCrsr++;

		theOriginalMethod =
			class_getInstanceMethod(ioClass->super_class, theAllocWithZoneMethodSEL);
		methodsCrsr->method_name = theAllocWithZoneMethodSEL;
		methodsCrsr->method_types = theOriginalMethod->method_types;
		methodsCrsr->method_imp = (IMP) ObjCAllocWithZoneMethod;
	}
}

/**
 * Native function: GetObjCClass.
 * Get a reference to a class.
 *
 * @param inRcvr	self.
 * @param inName	name of the class (it's a string)
 * @return a reference to a class object.
 */
newtRef
GetObjCClass(newtRefArg inRcvr, newtRefArg inName)
{
	(void) inRcvr;

	if (!NewtRefIsString(inName))
	{
		return NewtThrow(kNErrNotAString, inName);
	}
	
	return GetClassFromName(NewtRefToString(inName));
}

/**
 * Native function: CreateObjCClass.
 * Create a class inherited from another class.
 *
 * @param inRcvr			self.
 * @param inDef				definition of the class.
 * @return a reference to a class object or nil if we couldn't create the
 * the class (the super class doesn't exist or the class already exists).
 */
newtRef
CreateObjCClass(newtRefArg inRcvr, newtRefArg inDef)
{
	(void) inRcvr;

	if (!NewtRefIsFrame(inDef))
	{
		return NewtThrow(kNErrNotAFrame, inDef);
	}

	newtRefVar nsNameString = NcGetSlot(inDef, NSSYM(name));
	newtRefVar nsSuperString = NcGetSlot(inDef, NSSYM(super));
	newtRefVar nsInstanceMethods = NcGetSlot(inDef, NSSYM(instanceMethods));
	newtRefVar nsClassMethods = NcGetSlot(inDef, NSSYM(classMethods));
	newtRefVar nsInstanceVariables = NcGetSlot(inDef, NSSYM(instanceVariables));
	newtRefVar nsClassVariables = NcGetSlot(inDef, NSSYM(classVariables));

	if (!NewtRefIsString(nsNameString))
	{
		return NewtThrow(kNErrNotAString, nsNameString);
	}
	if (!NewtRefIsString(nsSuperString))
	{
		return NewtThrow(kNErrNotAString, nsSuperString);
	}

	const char* name = NewtRefToString( nsNameString );
	const char* superclassName = NewtRefToString( nsSuperString );

	// (code below is directly copied from Apple documentation).
	
	struct objc_class* meta_class;
	struct objc_class* super_class;
	struct objc_class* new_class;
	struct objc_class* root_class;

	//
	// Ensure that the superclass exists and that someone
	// hasn't already implemented a class with the same name
	//
	super_class = (struct objc_class*) objc_lookUpClass(superclassName);
	if (super_class == nil)
	{
		return kNewtRefNIL;
	}
	if (objc_lookUpClass(name) != nil) 
	{
		return kNewtRefNIL;
	}

	// Find the root class
	root_class = super_class;
	while( root_class->super_class != nil )
	{
		root_class = root_class->super_class;
	}
	
	// Allocate space for the class and its metaclass
	new_class = calloc( 2, sizeof(struct objc_class) );
	meta_class = &new_class[1];
	
	// setup class
	new_class->isa      = meta_class;
	new_class->info     = CLS_CLASS;
	meta_class->info    = CLS_META;
	
	//
	// Create a copy of the class name.
	// For efficiency, we have the metaclass and the class itself 
	// to share this copy of the name, but this is not a requirement
	// imposed by the runtime.
	//
	new_class->name = strdup(name);
	meta_class->name = new_class->name;
	
	//
	// Connect the class definition to the class hierarchy:
	// Connect the class to the superclass.
	// Connect the metaclass to the metaclass of the superclass.
	// Connect the metaclass of the metaclass to
	//      the metaclass of the root class.
	new_class->super_class  = super_class;
	meta_class->super_class = super_class->isa;
	meta_class->isa         = (void*) root_class->isa;

	// Set the version.
	new_class->version = 0;
	meta_class->version = 0;

	// Set the cache.
	new_class->cache = NULL;
	meta_class->cache = NULL;

	// Set the protocols.
	new_class->protocols = NULL;
	meta_class->protocols = NULL;
	
	// Set the instance variables lists.
	CreateObjCVariables(new_class, nsInstanceVariables);
	CreateObjCVariables(meta_class, nsClassVariables);
	
	// Define the ObjC methods (those that will call ns methods and those
	// that will initialize the object).
	CreateObjCMethods(new_class, nsInstanceMethods, 0);
	CreateObjCMethods(meta_class, nsClassMethods, 1);
    
	// Finally, register the class with the runtime.
	objc_addClass( new_class );

	// Get the class frame as if it was an ObjC class.
	newtRefVar theClassFrame = GetClassFromName( new_class->name );

	// The result frame includes native methods (instead of ns methods).
	// I have to modify the methods frame to include the ns methods.
	int nbMethods;
	int indexMethods;
	if (NewtRefIsNotNIL(nsClassMethods))
	{
		newtRefVar originalMethods = NcGetSlot(theClassFrame, NSSYM(_proto));
		nbMethods = NewtFrameLength(nsClassMethods);
		for (indexMethods = 0; indexMethods < nbMethods; indexMethods++)
		{
			newtRef key = NewtGetFrameKey(nsClassMethods, indexMethods);
			newtRef function = NewtGetFrameSlot(nsClassMethods, indexMethods);
	
			NcSetSlot(originalMethods, key, function);
		}
	}

	if (NewtRefIsNotNIL(nsInstanceMethods))
	{
		newtRefVar originalMethods = NcGetSlot(theClassFrame, NSSYM(_instanceMethods));
		nbMethods = NewtFrameLength(nsInstanceMethods);
		for (indexMethods = 0; indexMethods < nbMethods; indexMethods++)
		{
			newtRef key = NewtGetFrameKey(nsInstanceMethods, indexMethods);
			newtRef function = NewtGetFrameSlot(nsInstanceMethods, indexMethods);
	
			NcSetSlot(originalMethods, key, function);
		}
	}
	
	// Note that this class is defined by us (i.e. instances have a _ns slot).
	NcSetSlot(theClassFrame, NSSYM(_nsDefined), kNewtRefTRUE);

	// Return the class frame.
	return theClassFrame;
}

// ======================================================================= //
//         How many seconds are there in a year?  If I tell you there  are //
// 3.155  x  10^7, you won't even try to remember it.  On the other hand,  //
// who could forget that, to within half a percent, pi seconds is a        //
// nanocentury.                                                            //
//                 -- Tom Duff, Bell Labs                                  //
// ======================================================================= //
