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

// NEWT/0
#include "NewtCore.h"
#include "NewtVM.h"

// NewtObjC
#import "Constants.h"
#import "TNewtObjCRef.h"
#import "Utils.h"

// As an alternative to the ASM glue, we use more portable NSInvocation.
// However, we need this private function.
// Cf: https://www.mikeash.com/pyblog/friday-qa-2011-10-28-generic-block-proxying.html
@interface NSInvocation (PrivateHack)
- (void)invokeUsingIMP: (IMP)imp;
@end

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
						object_getClass(theObjCObject),
						NewtRefToString(inName));
	if (theVarDef)
	{
		void* theVariable = (void*) (((uintptr_t) theObjCObject) + ivar_getOffset(theVarDef));
		theResult = CastStructToNS(theVariable, ivar_getTypeEncoding(theVarDef));
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
						object_getClass(theObjCObject),
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

	Method theObjCMethod = class_getInstanceMethod(object_getClass(inSelf), _cmd);
	const char* theObjCMethodName = sel_getName(_cmd);

	// Get the NewtonScript method.
	char* theNSMethodName = ObjCNSNameTranslation(theObjCMethodName);
	newtRefVar theMethodSym = NewtMakeSymbol(theNSMethodName);
	free(theNSMethodName);
	newtRefVar theIMPL = theInstance;
	newtRefVar theNSMethod = kNewtRefNIL;
	while (NewtRefIsFrame(theIMPL))
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
		objCResult = CastResultToObjC(method_getTypeEncoding(theObjCMethod), theResult);
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
	superCall.super_class = class_getSuperclass(object_getClass(inSelf));
    void* (*objc_msgSendSuperTyped)(struct objc_super *self, SEL _cmd, NSZone*) = (void*)objc_msgSendSuper;
	id theResult = objc_msgSendSuperTyped(&superCall, _cmd, inZone);
	
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
	superCall.super_class = class_getSuperclass(object_getClass(inSelf));
    void* (*objc_msgSendSuperTyped)(struct objc_super *self, SEL _cmd) = (void*)objc_msgSendSuper;
    id theResult = objc_msgSendSuperTyped(&superCall, _cmd);
	
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
    // Create proto frame shared by all instances of the class.
	newtRefVar theProtoFrame = NcMakeFrame();

	// Grab the class frame.
	newtRefVar theClassFrame = GetClassFromName(class_getName(((Class) inSelf)));

    // Get the instance methods.
    newtRefVar instanceMethods =
        NcGetSlot(theClassFrame, NSSYM(_instanceMethods));
    // Set _proto slot to be the instance methods frame.
    NcSetSlot(theProtoFrame, NSSYM(_proto), instanceMethods);

    // Store the class in the _class frame.
    NcSetSlot(theProtoFrame, NSSYM(_class), theClassFrame);

	// Set it as index Ivar
	TNewtObjCRef** indexedIVars = (TNewtObjCRef**) object_getIndexedIvars(inSelf);
	*indexedIVars = [TNewtObjCRef refWithRef: theProtoFrame];
}

newtRef
GetClassProtoFrame(Class inClass)
{
	TNewtObjCRef** indexedIVars = (TNewtObjCRef**) object_getIndexedIvars((id) inClass);
	return [*indexedIVars ref];
}

/**
 * Create the instance frame and set the variable on the object accordingly.
 * This is done by getting the class proto frame.
 * An instance frame is made of:
 * {
 *  _proto: {
 *	  _proto: instance methods frame
 *    _class: class frame
 *  },
 *  _parent: utility methods
 *  _self: pointer to the object
 * }
 */
void
CreateInstanceFrame(id inObjCObject)
{
	// Get a pointer to the _ns variable of the objc object.
	Ivar theVarDef = class_getInstanceVariable(object_getClass(inObjCObject), kObjCNSFrameVarName);
	TNewtObjCRef** theHandle =
		((TNewtObjCRef**) ((uintptr_t) inObjCObject + ivar_getOffset(theVarDef)));

	if (*theHandle == NULL)
	{
		// Create a new frame.
		newtRefVar theObjectFrame = NcMakeFrame();

	    newtRefVar theClassProtoFrame = GetClassProtoFrame(object_getClass(inObjCObject));
		NcSetSlot(theObjectFrame, NSSYM(_proto), theClassProtoFrame);
		
		// Store the id in the _self frame.
		NcSetSlot(theObjectFrame, NSSYM(_self), PointerToBinary(inObjCObject));

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
GetInstanceFrame(id inObjCObject)
{
	Ivar theVarDef = class_getInstanceVariable(object_getClass(inObjCObject), kObjCNSFrameVarName);
	TNewtObjCRef** theHandle =
		((TNewtObjCRef**) ((uintptr_t) inObjCObject + ivar_getOffset(theVarDef)));
	return [*theHandle ref];
}

/**
 * Generic method with any number of argument.
 */
newtRef
GenericMethod(newtRef inRcvr, newtRef inArgs)
{
	// Retrieve the method
	id objcSelf;
	unsigned nbOfArguments;
	int indexArgs, nbArgsNS;
	struct objc_method* theMethod;
	newtRefVar theResultObj = kNewtRefNIL;
	newtRefVar theCurrentFunc = NVMCurrentFunction();
	theMethod = (struct objc_method*)
		BinaryToPointer(NcGetSlot(theCurrentFunc, NSSYM(_method)));

	nbOfArguments = method_getNumberOfArguments(theMethod) - 2;
	nbArgsNS = (int) NewtArrayLength(inArgs);
	if (nbOfArguments != nbArgsNS)
	{
		// Throw an error
		return NewtThrow(
					kNErrWrongNumberOfArgs,
					NcGetSlot(theCurrentFunc, NSSYM(docString)));
	}

    const char* methodTypes = method_getTypeEncoding(theMethod);
    NSMethodSignature* methodSig = [NSMethodSignature signatureWithObjCTypes: methodTypes];
    NSInvocation* invocation = [NSInvocation invocationWithMethodSignature: methodSig];
	objcSelf = (id) BinaryToPointer(NcGetSlot(inRcvr, NSSYM(_self)));

	// Add both the id and the selector
    invocation.target = objcSelf;
    invocation.selector = method_getName(theMethod);

	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	// Convert and stuff the arguments
	for (indexArgs = 0; indexArgs < nbOfArguments; indexArgs++)
	{
        char* theType = method_copyArgumentType(theMethod, indexArgs + 2);
        CastParamToObjC(theMethod, invocation, indexArgs + 2, NewtGetArraySlot(inArgs, indexArgs));
        free(theType);
    }
	
	// Catch exceptions (we'll cast them)
	NS_DURING

    // We need to use a private API call here to call super without a lookup for
    // the imp. Otherwise, we'll run into infinite loops for NewtonScript usage
    // of inheritance.
	[invocation invokeUsingIMP: method_getImplementation(theMethod)];
	NSUInteger methodReturnLength = [methodSig methodReturnLength];

	// Determine if the return type is a structure
	if (methodTypes[0] == '{') {
		if ((strncmp(methodTypes, "{_NSRange}", 10) == 0)
			|| (strncmp(methodTypes, "{_NSRange=", 10) == 0)) {
			NSRange theRange;
			if (methodReturnLength != sizeof(theRange)) {
				printf("Return type size mismatch, got %d, expected %d (NSRange)\n", (int) methodReturnLength, (int) sizeof(theRange));
				theResultObj = kNewtRefNIL;
			} else {
				[invocation getReturnValue:&theRange];
				theResultObj = NSRangeToRef(theRange);
			}
		} else if ((strncmp(methodTypes, "{_NSRect}", 9) == 0)
			|| (strncmp(methodTypes, "{_NSRect=", 9) == 0)) {
            NSRect theRect;
            if (methodReturnLength != sizeof(theRect)) {
				printf("Return type size mismatch, got %d, expected %d (NSRect)\n", (int) methodReturnLength, (int) sizeof(theRect));
				theResultObj = kNewtRefNIL;
            } else {
				[invocation getReturnValue:&theRect];
				theResultObj = NSRectToRef(theRect);
            }
		} else if ((strncmp(methodTypes, "{_NSPoint}", 10) == 0)
			|| (strncmp(methodTypes, "{_NSPoint=", 10) == 0)) {
			NSPoint thePoint;
			if (methodReturnLength != sizeof(thePoint)) {
				printf("Return type size mismatch, got %d, expected %d (NSPoint)\n", (int) methodReturnLength, (int) sizeof(thePoint));
				theResultObj = kNewtRefNIL;
			} else {
				[invocation getReturnValue:&thePoint];
				theResultObj = NSPointToRef(thePoint);
			}
		} else if ((strncmp(methodTypes, "{_NSSize}", 9) == 0)
			|| (strncmp(methodTypes, "{_NSSize=", 9) == 0)) {
			NSSize theSize;
			if (methodReturnLength != sizeof(theSize)) {
				printf("Return type size mismatch, got %d, expected %d (NSSize)\n", (int) methodReturnLength, (int) sizeof(theSize));
				theResultObj = kNewtRefNIL;
			} else {
				[invocation getReturnValue:&theSize];
				theResultObj = NSSizeToRef(theSize);
			}
		} else if ((strncmp(methodTypes, "{_CGRect}", 9) == 0)
             || (strncmp(methodTypes, "{_CGRect=", 9) == 0)) {
			CGRect theRect;
			if (methodReturnLength != sizeof(theRect)) {
				printf("Return type size mismatch, got %d, expected %d (CGRect)\n", (int) methodReturnLength, (int) sizeof(theRect));
				theResultObj = kNewtRefNIL;
			} else {
				[invocation getReturnValue:&theRect];
				theResultObj = CGRectToRef(theRect);
			}
		} else if ((strncmp(methodTypes, "{_CGPoint}", 10) == 0)
               || (strncmp(methodTypes, "{_CGPoint=", 10) == 0)) {
			CGPoint thePoint;
			if (methodReturnLength != sizeof(thePoint)) {
				printf("Return type size mismatch, got %d, expected %d (CGPoint)\n", (int) methodReturnLength, (int) sizeof(thePoint));
				theResultObj = kNewtRefNIL;
			} else {
				[invocation getReturnValue:&thePoint];
				theResultObj = CGPointToRef(thePoint);
			}
		} else if ((strncmp(methodTypes, "{_CGSize}", 9) == 0)
               || (strncmp(methodTypes, "{_CGSize=", 9) == 0)) {
			CGSize theSize;
			if (methodReturnLength != sizeof(theSize)) {
				printf("Return type size mismatch, got %d, expected %d (CGPoint)\n", (int) methodReturnLength, (int) sizeof(theSize));
				theResultObj = kNewtRefNIL;
			} else {
				[invocation getReturnValue:&theSize];
				theResultObj = CGSizeToRef(theSize);
			}
		} else {
			printf("Unhandled structure %s\n", methodTypes);
			theResultObj = kNewtRefNIL;
		}
	} else if (methodTypes[0] == 'f') {
        float theFloat;
        if (methodReturnLength != sizeof(theFloat)) {
            printf("Return type size mismatch, got %d, expected %d (float)\n", (int) methodReturnLength, (int) sizeof(theFloat));
            theResultObj = kNewtRefNIL;
        } else {
            [invocation getReturnValue:&theFloat];
            // Cast the result
            theResultObj = CastToNS((id*) &theFloat, methodTypes);
        }
	} else if (methodTypes[0] == 'd') {
        double theDouble;
        if (methodReturnLength != sizeof(theDouble)) {
            printf("Return type size mismatch, got %d, expected %d (double)\n", (int) methodReturnLength, (int) sizeof(theDouble));
            theResultObj = kNewtRefNIL;
        } else {
            [invocation getReturnValue:&theDouble];
            // Cast the result
            theResultObj = CastToNS((id*) &theDouble, methodTypes);
        }
    } else if (methodReturnLength == sizeof(id)) {
        id theResult = nil;
        [invocation getReturnValue:&theResult];
		// Cast the result
		theResultObj = CastToNS(&theResult, methodTypes);
    } else if (methodReturnLength == 0) {
        theResultObj = kNewtRefNIL;
    } else {
        printf("Return type size mismatch, got %d, expected %d (%s)\n", (int) methodReturnLength, (int) sizeof(id), methodTypes);
        theResultObj = kNewtRefNIL;
    }

	NS_HANDLER
		// Convert the exception and throw it as a NS exception.
		newtRefVar theExceptionFrame = CastExToNS(localException);
		NVMThrowData(
			NcGetSlot(theExceptionFrame, NSSYM(name)), theExceptionFrame);
		theResultObj = kNewtRefNIL;
	NS_ENDHANDLER

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
	
    Class theClass = inObjCClass;
	while(theClass != NULL)
	{
        unsigned int nbMethods;
        Method* methods = class_copyMethodList(theClass, &nbMethods);

		int indexMethods;
		for (indexMethods = 0; indexMethods < nbMethods; indexMethods++)
		{
            Method theMethod = methods[indexMethods];
            const char* theObjCName = sel_getName(method_getName(theMethod));
			char* theNSName = ObjCNSNameTranslation(theObjCName);
            newtRef theNSNameSymbol = NewtMakeSymbol(theNSName);

            // Do not add methods we already have (from subclasses)
            if (!NewtHasSlot(result, theNSNameSymbol)) {
				newtRefVar nsMethod;

				// create the (ns) native method
				nsMethod =
					NewtMakeNativeFunc0(GenericMethod, 0, true, (char*) theNSName);

				// add the objc method ptr itself so the glue will be able to call
				// it
				NcSetSlot(nsMethod, NSSYM(_method), PointerToBinary(theMethod));

				// add the method to the frame
				NcSetSlot(result, NewtMakeSymbol(theNSName), nsMethod);
            }
			
			free(theNSName);
		}
        free(methods);
        theClass = class_getSuperclass(theClass);
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
	newtRefVar classMethodsFrame = CreateClassObjectMethods(object_getClass(inObjCClass));

	// Link it with super class class methods frame.
	Class super_class = class_getSuperclass(inObjCClass);
	newtRefVar superClassFrame = kNewtRefNIL;
	if (super_class)
	{
		superClassFrame = GetClassFromName(class_getName(super_class));
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
 * @param ioClass		class we're defining
 * @param inVariables	list of the variables to define (array).
 */
void
CreateObjCVariables(
	Class ioClass,
	newtRefArg inVariables)
{
	// Set the instance variables.
	size_t nbVars = 0;
	if (!NewtRefIsNIL(inVariables))
	{
		nbVars = NewtArrayLength(inVariables);
	}

	// Create the variables from the list.
	size_t indexVars;
	for (indexVars = 0; indexVars < nbVars; indexVars++)
	{
		newtRefVar theNSVarDef = NewtSlotsGetSlot( inVariables, indexVars );
		if (!NewtRefIsFrame(theNSVarDef))
		{
			(void) NewtThrow(kNErrNotAFrame, theNSVarDef);
			break;
		}
		newtRefVar theName = NcGetSlot(theNSVarDef, NSSYM(name));
		if (!NewtRefIsString(theName))
		{
			(void) NewtThrow(kNErrNotAString, theName);
			break;
		}
		const char* theNameCStr = NewtRefToString(theName);

		newtRefVar theType = NcGetSlot(theNSVarDef, NSSYM(type));
		if (!NewtRefIsString(theType))
		{
			(void) NewtThrow(kNErrNotAString, theType);
			break;
		}
		const char* theTypeCStr = NewtRefToString(theType);
		
		if (strcmp(theTypeCStr, kObjCOutletTypeStr) == 0)
		{
			// Copy the name.
            class_addIvar(ioClass, theNameCStr, sizeof(id), log2(_Alignof(id)), kObjCOutletTypeStr);
		} else {
			fprintf(stderr, "I don't handle types other than kObjCOutletType yet");
			break;
		}
	}

	// Finish with variable is _ns
    class_addIvar(ioClass, kObjCNSFrameVarName, sizeof(newtRefVar), log2(_Alignof(newtRefVar)), kObjCNSFrameVarType);
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
	Class ioClass,
	newtRefArg inMethods,
	int isMetaClass)
{
	size_t nbNSMethods = 0;
	size_t indexMethods;

	if (!NewtRefIsNIL(inMethods))
	{
		nbNSMethods = NewtFrameLength(inMethods);
	}
	
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
    Class superClass = class_getSuperclass(ioClass);

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
				continue;
			}
		}
		
		// Get the method of super class.
		theOriginalMethod =
			class_getInstanceMethod(superClass, theMethodSEL);
		
		if (theOriginalMethod)
		{
			int nbArgsObjC = method_getNumberOfArguments(theOriginalMethod) - 2;
			if (!NVMFuncCheckNumArgs(function, nbArgsObjC))
			{
				(void) NewtThrow( kNErrWrongNumberOfArgs, function );
				// XXX Cleanup
				break;
			}
			
            class_addMethod(ioClass, theMethodSEL, (IMP) GenericObjCMethod, method_getTypeEncoding(theOriginalMethod));
		} else {
			// I need the type (from the function definition).
			newtRef value = NewtGetFrameSlot(inMethods, indexMethods);
			newtRef theType = NcGetSlot(value, NSSYM(objc_type));
			if (NewtRefIsString(theType))
			{
				const char* theTypeString = NewtRefToString(theType);
				class_addMethod(ioClass, theMethodSEL, (IMP) GenericObjCMethod, theTypeString);
			} else {
				// Use the default type.
				const char* theTypeString;
				if (NVMFuncCheckNumArgs(function, 0))
				{
					theTypeString = kObjCDefaultFunc0TypeStr;
				} else if (NVMFuncCheckNumArgs(function, 1)) {
					theTypeString = kObjCDefaultFunc1TypeStr;
				} else if (NVMFuncCheckNumArgs(function, 2)) {
					theTypeString = kObjCDefaultFunc2TypeStr;
				} else if (NVMFuncCheckNumArgs(function, 3)) {
					theTypeString = kObjCDefaultFunc3TypeStr;
				} else {
					(void) NewtThrow( kNErrWrongNumberOfArgs, function );
					// XXX Cleanup
					break;
				}
				class_addMethod(ioClass, theMethodSEL, (IMP) GenericObjCMethod, theTypeString);
			}
		}
	}

	// Add the initialize, alloc & allocWithZone class method
	if (isMetaClass)
	{
		theOriginalMethod =
			class_getInstanceMethod(superClass, theInitMethodSEL);
        class_addMethod(ioClass, theInitMethodSEL, (IMP) ObjCInitializeMethod, method_getTypeEncoding(theOriginalMethod));

		theOriginalMethod =
			class_getInstanceMethod(superClass, theAllocMethodSEL);
        class_addMethod(ioClass, theAllocMethodSEL, (IMP) ObjCAllocMethod, method_getTypeEncoding(theOriginalMethod));

		theOriginalMethod =
			class_getInstanceMethod(superClass, theAllocWithZoneMethodSEL);
        class_addMethod(ioClass, theAllocWithZoneMethodSEL, (IMP) ObjCAllocWithZoneMethod, method_getTypeEncoding(theOriginalMethod));
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

	//
	// Ensure that the superclass exists and that someone
	// hasn't already implemented a class with the same name
	//
	Class super_class = (Class) objc_lookUpClass(superclassName);
	if (super_class == nil)
	{
		return kNewtRefNIL;
	}
	if (objc_lookUpClass(name) != nil) 
	{
		return kNewtRefNIL;
	}

	// Allocate space for the class and its metaclass
    Class new_class = objc_allocateClassPair(super_class, name, sizeof(TNewtObjCRef*));
    Class meta_class = object_getClass(new_class);

    // Set the instance variables lists.
	CreateObjCVariables(new_class, nsInstanceVariables);

	// Define the ObjC methods (those that will call ns methods and those
	// that will initialize the object).
	CreateObjCMethods(new_class, nsInstanceMethods, 0);
	CreateObjCMethods(meta_class, nsClassMethods, 1);

	// Finally, register the class with the runtime.
	objc_registerClassPair(new_class);

	// Get the class frame as if it was an ObjC class.
	newtRefVar theClassFrame = GetClassFromName(class_getName(new_class));

	// The result frame includes native methods (instead of ns methods).
	// I have to modify the methods frame to include the ns methods.
	size_t nbMethods;
	size_t indexMethods;
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
