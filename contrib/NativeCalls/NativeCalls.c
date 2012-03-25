//--------------------------------------------------------------------------
/**
 * @file  NativeCalls.c
 * @brief Interface for all native code available through dlopen & libffi.
 *
 * @author Paul Guyot <pguyot@kallisys.net>
 * @date 2005-01-24
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is NativeCalls.c
 *
 * The Initial Developer of the Original Code is Paul Guyot. Portions created
 * by the Initial Developers are
 * Copyright (C) 2005 the Initial Developers. All Rights Reserved.
 *
 * Contributor(s):
 *   Paul Guyot <pguyot@kallisys.net> (original author)
 *
 * ***** END LICENSE BLOCK *****
 */


/* Includes */
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <dlfcn.h>
#include <ffi/ffi.h>
#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include "NewtLib.h"
#include "NewtCore.h"
#include "NewtVM.h"

#define kNErrNative					(kNErrMiscBase - 2)	///< OK. Maybe change this.
#define kLibParentMagicPtrKey		NSSYM(parentNativeLib)

/* Storage for non pointer arguments */
typedef union SStorage {
	long double	fLongDouble;
	double		fDouble;
	float		fFloat;
	uint64_t	fInt64;
	uint32_t	fInt32;
	uint16_t	fInt16;
	uint8_t		fInt8;
	void*		fPointer;
} SStorage;

typedef struct SBufferZone {
	int			fNumberBuffers;
	void**		fBufferTable;
} SBufferZone;

/**
 * Allocate an empty storage buffer.
 *
 * @param outBuffer	on output, allocated buffer.
 */
void
NativeCalls_AllocateBuffer(SBufferZone* outBuffer)
{
	outBuffer->fNumberBuffers = 0;
	outBuffer->fBufferTable = (void**) malloc(0);
}

/**
 * Release a storage buffer.
 *
 * @param ioBuffer	buffer to release.
 */
void
NativeCalls_ReleaseBuffer(SBufferZone* ioBuffer)
{
	/* Iterate on all buffers */
	int indexBuffers;
	int nbBuffers = ioBuffer->fNumberBuffers;
	void** cursor = ioBuffer->fBufferTable;
	
	for (indexBuffers = 0; indexBuffers < nbBuffers; indexBuffers++)
	{
		free(*cursor);
		*cursor++;
	}
	
	free(ioBuffer->fBufferTable);
	ioBuffer->fNumberBuffers = 0;
	ioBuffer->fBufferTable = NULL;
}

/**
 * Allocate a segment in a buffer.
 *
 * @param ioBuffer	buffer in which to allocate the segment.
 * @param inSize	segment size.
 */
void*
NativeCalls_AllocateBufferSegment(SBufferZone* ioBuffer, int inSize)
{
	void* theResult = malloc(inSize);
	int newBufferCount = ++ioBuffer->fNumberBuffers;
	ioBuffer->fBufferTable =
		realloc(ioBuffer->fBufferTable, newBufferCount * sizeof(void*));
	ioBuffer->fBufferTable[newBufferCount - 1] = theResult;
	
	return theResult;
}

/**
 * Retrieve a pointer stored in a binary.
 *
 * @param inPointerBinary	binary holding the pointer.
 * @return the pointer stored in this binary or NULL if the parameter is not
 *			a binary.
 */
void*
NativeCalls_BinaryToPointer(newtRefArg inPointerBinary)
{
	void* theResult = NULL;

	if (NewtRefIsBinary(inPointerBinary))
	{
		/* It's a binary. Extract its value */
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
NativeCalls_PointerToBinary(void* inPointer)
{
	return NewtMakeBinary(
				kNewtRefUnbind,
				(uint8_t *)&inPointer,
				sizeof(inPointer),
				false);
}

/**
 * Perform the cleanup for a library frame.
 * Basically, closes the library and set the handle to NIL.
 *
 * @param inLib	a reference to the library frame.
 */
void
NativeCalls_CloseLib(newtRefArg inPointerBinary)
{
	void* theHandle;

	/* Get the handle */
	theHandle = NativeCalls_BinaryToPointer(inPointerBinary);
	
	if (theHandle)
	{
		/* close the library, ignoring errors */
		(void) dlclose(theHandle);
	}
}

/**
 * Actually open a library. Return NULL if the library couldn't be found.
 * Tries: path, path + suffix, path + suffix + anything.
 *
 * @param inPath	path of the library to open.
 * @return a handle or NULL
 */
void*
NativeCalls_DoOpenLib(const char* inPath)
{
	void* theResult = NULL;
	char* theDirNameBuf = NULL;
	char* theBaseNameBuf = NULL;
	do {
		char thePath[PATH_MAX];
		char* theDirName;
		char* theBaseName;
		int baseNameLen;
		int pathLen;
		DIR* theDir;

		/* try to open the library without any suffix */
		theResult = dlopen(inPath, RTLD_LAZY);
		if (theResult) break;
		
		/* add the suffix */
		pathLen = strlen(inPath);
		if (pathLen >= (PATH_MAX - 1)) break;

		(void) memcpy(
					thePath,
					inPath,
					pathLen);

		thePath[pathLen++] = '.';
		(void) strncpy(
					&thePath[pathLen],
					DYLIBSUFFIX,
					PATH_MAX - pathLen);
		thePath[PATH_MAX - 1] = 0;
		theResult = dlopen(thePath, RTLD_LAZY);
		if (theResult) break;

		/* try to look at path + anything */
		theDirNameBuf = strdup(inPath);
		theDirName = dirname(theDirNameBuf);
		
		/* iterate on the directory */
		theDir = opendir(theDirName);
		if (theDir != NULL)
		{
			theBaseNameBuf = strdup(thePath);
			theBaseName = basename(theBaseNameBuf);
			baseNameLen = strlen(theBaseName);
			pathLen = strlen(thePath);

			struct dirent* theEntry;
			do {
				theEntry = readdir(theDir);
				if (theEntry == NULL)
				{
					break;
				}
				if (memcmp(theEntry->d_name, theBaseName, baseNameLen) == 0)
				{
					/* the name begins with what we have */
					strncpy(
						&thePath[pathLen],
						&(theEntry->d_name)[baseNameLen],
						PATH_MAX - pathLen);
					thePath[PATH_MAX - 1] = 0;
					theResult = dlopen(thePath, RTLD_LAZY);
					if (theResult) break;
				}
			} while (true);
			
			closedir(theDir);
		}
	} while (false);
	
	if (theDirNameBuf != NULL)
	{
		free(theDirNameBuf);
	}
	if (theBaseNameBuf != NULL)
	{
		free(theBaseNameBuf);
	}
	
	return theResult;
}

/**
 * Open a library. Return a reference to the binary holding the handle.
 * Algorithm:
 * -> if the path is absolute, check 
 *
 * @param inPath	path of the library to open.
 * @return a reference to a library object.
 */
newtRef
NativeCalls_OpenLib(const char* inPath)
{
	newtRefVar result;
	char thePath[PATH_MAX];
	void* theHandle = NULL;
	
	do {
		if (inPath[0] != '/')
		{
			/* try with /lib/<inPath> */
			(void) snprintf(
						thePath,
						sizeof(thePath),
						"/lib/%s",
						inPath );
			thePath[PATH_MAX - 1] = 0;
			theHandle = NativeCalls_DoOpenLib(thePath);
			if (theHandle) break;
	
			/* try with /usr/lib/<inPath> */
			(void) snprintf(
						thePath,
						sizeof(thePath),
						"/usr/lib/%s",
						inPath );
			thePath[PATH_MAX - 1] = 0;
			theHandle = NativeCalls_DoOpenLib(thePath);
			if (theHandle) break;
		}

		/* try directly */
		theHandle = NativeCalls_DoOpenLib(inPath);
		if (theHandle) break;
	} while (false);

	if (theHandle)
	{
		/* Create a new binary */
		result = NativeCalls_PointerToBinary(theHandle);
	} else {
		/* dlopen failed, let's throw the error string */
		result = NewtThrow(kNErrNative, NewtMakeString(dlerror(), false));
	}
	
	return result;
}

/**
 * Cast the result using the NS type.
 *
 * @param inNSType		ns type.
 * @param inFFIValue	ffi value.
 * @return the NS value, or nil if it couldn't be cast (or if the type is void).
 */
newtRef
NativeCalls_CastResult(newtRefArg inNSType, SStorage* inValue)
{
	newtRefVar theResult;
	
	if (NewtSymbolEqual(inNSType, NSSYM(uint8))) {
		theResult = NewtMakeInteger( inValue->fInt8 );
	} else if (NewtSymbolEqual(inNSType, NSSYM(sint8))) {
		theResult = NewtMakeInteger( (int32_t) inValue->fInt8 );
	} else if (NewtSymbolEqual(inNSType, NSSYM(uint16))) {
		theResult = NewtMakeInteger( inValue->fInt16 );
	} else if (NewtSymbolEqual(inNSType, NSSYM(sint16))) {
		theResult = NewtMakeInteger( (int32_t) inValue->fInt16 );
	} else if (NewtSymbolEqual(inNSType, NSSYM(uint32))) {
		theResult = NewtMakeInteger( inValue->fInt32 );
	} else if (NewtSymbolEqual(inNSType, NSSYM(sint32))) {
		theResult = NewtMakeInteger( (int32_t) inValue->fInt32 );
	} else if (NewtSymbolEqual(inNSType, NSSYM(uint64))) {
		theResult = NewtMakeInteger( inValue->fInt64 );
	} else if (NewtSymbolEqual(inNSType, NSSYM(sint64))) {
		theResult = NewtMakeInteger( (int32_t) inValue->fInt64 );
	} else if (NewtSymbolEqual(inNSType, NSSYM(float))) {
		theResult = NewtMakeReal( inValue->fFloat );
	} else if (NewtSymbolEqual(inNSType, NSSYM(double))) {
		theResult = NewtMakeReal( inValue->fDouble );
	} else if (NewtSymbolEqual(inNSType, NSSYM(longdouble))) {
		theResult = NewtMakeReal( inValue->fLongDouble );
	} else if (NewtSymbolEqual(inNSType, NSSYM(string))) {
		if (inValue->fPointer) {
			theResult = NewtMakeString((char*) inValue->fPointer, false);
		} else {
			theResult = kNewtRefNIL;
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(pointer))) {
		if (inValue->fPointer) {
			theResult = NativeCalls_PointerToBinary(inValue->fPointer);
		} else {
			theResult = kNewtRefNIL;
		}
	} else {
		theResult = kNewtRefNIL;
	}

	return theResult;
}

/**
 * From a type (in NS format), set the ffi type.
 *
 * @param inNSType		ns type.
 * @param outFFIType	ffi type.
 * @return \c true if the type was set, \c false otherwise, if the type is
 * unknown (the exception would have already been thrown then).
 */
bool
NativeCalls_CastType(newtRefArg inNSType, ffi_type** outFFIType)
{
	bool theResult = true;
		
	if (NewtSymbolEqual(inNSType, NSSYM(void)))
	{
		*outFFIType = &ffi_type_void;
	} else if (NewtSymbolEqual(inNSType, NSSYM(uint8))) {
		*outFFIType = &ffi_type_uint8;
	} else if (NewtSymbolEqual(inNSType, NSSYM(sint8))) {
		*outFFIType = &ffi_type_sint8;
	} else if (NewtSymbolEqual(inNSType, NSSYM(uint16))) {
		*outFFIType = &ffi_type_uint16;
	} else if (NewtSymbolEqual(inNSType, NSSYM(sint16))) {
		*outFFIType = &ffi_type_sint16;
	} else if (NewtSymbolEqual(inNSType, NSSYM(uint32))) {
		*outFFIType = &ffi_type_uint32;
	} else if (NewtSymbolEqual(inNSType, NSSYM(sint32))) {
		*outFFIType = &ffi_type_sint32;
	} else if (NewtSymbolEqual(inNSType, NSSYM(uint64))) {
		*outFFIType = &ffi_type_uint64;
	} else if (NewtSymbolEqual(inNSType, NSSYM(sint64))) {
		*outFFIType = &ffi_type_sint64;
	} else if (NewtSymbolEqual(inNSType, NSSYM(float))) {
		*outFFIType = &ffi_type_float;
	} else if (NewtSymbolEqual(inNSType, NSSYM(double))) {
		*outFFIType = &ffi_type_double;
	} else if (NewtSymbolEqual(inNSType, NSSYM(longdouble))) {
		*outFFIType = &ffi_type_longdouble;
	} else if (NewtSymbolEqual(inNSType, NSSYM(string))) {
		*outFFIType = &ffi_type_pointer;
	} else if (NewtSymbolEqual(inNSType, NSSYM(pointer))) {
		*outFFIType = &ffi_type_pointer;
	} else {
		(void) NewtThrow(kNErrNotASymbol, inNSType);
		theResult = false;
	}

	return theResult;
}

/* forward declaration */
bool
NativeCalls_CastTypeAndValue(
			newtRefArg inNSType,
			newtRefArg inNSValue,
			ffi_type** outFFIType,
			void* outFFIValue,
			size_t* outValueSize,
			SBufferZone* ioStorage);

/**
 * From an array of types (in NS format), and an array of values, set the ffi
 * types and cast the values.
 *
 * @param inNbArgs		number of args.
 * @param inNSTypes		ns types.
 * @param inNSValues	ns values.
 * @param outFFITypes	ffi types (allocated in the buffer).
 * @param outFFIValues	ffi values (allocated in the buffer).
 * @param ioStorage		storage buffer.
 * @return \c true if the type and value were cast, \c false otherwise, if
 * there was a typing error (the exception would have already been thrown then).
 */
bool
NativeCalls_CastTypesAndValues(
		int inNbArgs,
		newtRefVar inNSTypes,
		newtRefVar inNSValues,
		ffi_type*** outFFITypes,
		void*** outFFIValues,
		void** outStorage,
		SBufferZone* ioStorage)
{
	ffi_type**	ffiArgsTypes;
	void**		argsValues;
	SStorage*	argsStorage;
	void*		cursorStorage;
	int			indexArgs;
	int			ffitypes_size = (inNbArgs + 1) * sizeof(ffi_type*);
	int			storage_size = inNbArgs * sizeof(SStorage);
	bool 		typeError = false;	

	/* build the ffi lists */
	ffiArgsTypes = (ffi_type**) NativeCalls_AllocateBufferSegment(ioStorage, ffitypes_size);
	bzero(ffiArgsTypes, ffitypes_size);
	argsValues = (void**) NativeCalls_AllocateBufferSegment(ioStorage, sizeof(void*) * inNbArgs);
	argsStorage = (SStorage*) NativeCalls_AllocateBufferSegment(ioStorage, storage_size);
	bzero(argsStorage, storage_size);
	cursorStorage = argsStorage;
	
	for (indexArgs = 0; indexArgs < inNbArgs; indexArgs++)
	{
		newtRefVar theType, theValue;
		size_t theSize;
		
		theType = NewtGetArraySlot(inNSTypes, indexArgs);
		theValue = NewtGetArraySlot(inNSValues, indexArgs);
		/* use the storage */
		argsValues[indexArgs] = cursorStorage;
		if (!NativeCalls_CastTypeAndValue(
					theType,
					theValue,
					&ffiArgsTypes[indexArgs],
					cursorStorage,
					&theSize,
					ioStorage))
		{
			typeError = true;
			break;
		}
		cursorStorage = (void*) (((char*) cursorStorage) + theSize);
	}
	
	*outFFITypes = ffiArgsTypes;
	if (outFFIValues)
	{
		*outFFIValues = argsValues;
	}
	if (outStorage)
	{
		*outStorage = argsStorage;
	}

	return !typeError;
}

/**
 * From a type (in NS format), and a value, set the ffi type and cast the
 * value.
 *
 * @param inNSType		ns type.
 * @param inNSValue		ns value.
 * @param outFFIType	ffi type.
 * @param outFFIValue	ffi value.
 * @param ioStorage		storage buffer.
 * @return \c true if the type and value were cast, \c false otherwise, if
 * there was a typing error (the exception would have already been thrown then).
 */
bool
NativeCalls_CastTypeAndValue(
			newtRefArg inNSType,
			newtRefArg inNSValue,
			ffi_type** outFFIType,
			void* outFFIValue,
			size_t* outValueSize,
			SBufferZone* ioStorage)
{
	size_t theSize = 0;
	bool theResult = true;
	
	if (NewtRefIsArray(inNSType))
	{
		// Structure.
		int nbVals = NewtArrayLength(inNSType);
		if (nbVals != NewtArrayLength(inNSValue))
		{
			theResult = false;
		} else {
			ffi_type** theTypes;
			void* theValues;
			theResult = NativeCalls_CastTypesAndValues(
				nbVals,
				inNSType,
				inNSValue,
				&theTypes,
				NULL,
				&theValues,
				ioStorage);
			if (theResult) {
				// Make the record.
				ffi_type* theType = (ffi_type*)
					NativeCalls_AllocateBufferSegment(ioStorage, sizeof(ffi_type));
				*outFFIType = theType;
				theType->size = 0;
				theType->alignment = 0;
				theType->type = FFI_TYPE_STRUCT;
				theType->elements = theTypes;
				*((void**) outFFIValue) = theValues;
				theSize = sizeof(void*);
			}
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(uint8))) {
		*outFFIType = &ffi_type_uint8;
		if (NewtRefIsInteger(inNSValue))
		{
			*((uint8_t*) outFFIValue) =
				(uint8_t) NewtRefToInteger(inNSValue);
			theSize = sizeof(uint8_t);
		} else {
			(void) NewtThrow(kNErrNotAnInteger, inNSValue);
			theResult = false;
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(sint8))) {
		*outFFIType = &ffi_type_sint8;
		if (NewtRefIsInteger(inNSValue))
		{
			*((uint8_t*) outFFIValue) =
				(uint8_t) (int8_t) NewtRefToInteger(inNSValue);
			theSize = sizeof(uint8_t);
		} else {
			(void) NewtThrow(kNErrNotAnInteger, inNSValue);
			theResult = false;
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(uint16))) {
		*outFFIType = &ffi_type_uint16;
		if (NewtRefIsInteger(inNSValue))
		{
			*((uint16_t*) outFFIValue) =
				(uint16_t) NewtRefToInteger(inNSValue);
			theSize = sizeof(uint16_t);
		} else {
			(void) NewtThrow(kNErrNotAnInteger, inNSValue);
			theResult = false;
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(sint16))) {
		*outFFIType = &ffi_type_sint16;
		if (NewtRefIsInteger(inNSValue))
		{
			*((uint16_t*) outFFIValue) =
				(uint16_t) (int16_t) NewtRefToInteger(inNSValue);
			theSize = sizeof(uint16_t);
		} else {
			(void) NewtThrow(kNErrNotAnInteger, inNSValue);
			theResult = false;
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(uint32))) {
		*outFFIType = &ffi_type_uint32;
		if (NewtRefIsInteger(inNSValue))
		{
			*((uint32_t*) outFFIValue) =
				(uint32_t) NewtRefToInteger(inNSValue);
			theSize = sizeof(uint32_t);
		} else {
			(void) NewtThrow(kNErrNotAnInteger, inNSValue);
			theResult = false;
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(sint32))) {
		*outFFIType = &ffi_type_sint32;
		if (NewtRefIsInteger(inNSValue))
		{
			*((uint32_t*) outFFIValue) =
				(uint32_t) (int32_t) NewtRefToInteger(inNSValue);
			theSize = sizeof(uint32_t);
		} else {
			(void) NewtThrow(kNErrNotAnInteger, inNSValue);
			theResult = false;
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(uint64))) {
		*outFFIType = &ffi_type_uint64;
		if (NewtRefIsInteger(inNSValue))
		{
			*((uint64_t*) outFFIValue) =
				(uint64_t) NewtRefToInteger(inNSValue);
			theSize = sizeof(uint64_t);
		} else {
			(void) NewtThrow(kNErrNotAnInteger, inNSValue);
			theResult = false;
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(sint64))) {
		*outFFIType = &ffi_type_sint64;
		if (NewtRefIsInteger(inNSValue))
		{
			*((uint64_t*) outFFIValue) =
				(uint64_t) (int64_t) NewtRefToInteger(inNSValue);
			theSize = sizeof(uint64_t);
		} else {
			(void) NewtThrow(kNErrNotAnInteger, inNSValue);
			theResult = false;
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(float))) {
		*outFFIType = &ffi_type_float;
		if (NewtRefIsReal(inNSValue))
		{
			*((float*) outFFIValue) =
				(float) NewtRefToReal(inNSValue);
			theSize = sizeof(float);
		} else {
			(void) NewtThrow(kNErrNotAReal, inNSValue);
			theResult = false;
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(double))) {
		*outFFIType = &ffi_type_double;
		if (NewtRefIsReal(inNSValue))
		{
			*((double*) outFFIValue) =
				(double) NewtRefToReal(inNSValue);
			theSize = sizeof(double);
		} else {
			(void) NewtThrow(kNErrNotAReal, inNSValue);
			theResult = false;
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(longdouble))) {
		*outFFIType = &ffi_type_longdouble;
		if (NewtRefIsReal(inNSValue))
		{
			*((long double*) outFFIValue) =
				(long double) NewtRefToReal(inNSValue);
			theSize = sizeof(long double);
		} else {
			(void) NewtThrow(kNErrNotAReal, inNSValue);
			theResult = false;
		}
	} else if ((NewtSymbolEqual(inNSType, NSSYM(string)))
		|| (NewtSymbolEqual(inNSType, NSSYM(iostring)))) {
		*outFFIType = &ffi_type_pointer;
		if (NewtRefIsString(inNSValue))
		{
			*((void**) outFFIValue) =
				(void*) NewtRefToString(inNSValue);
			theSize = sizeof(void*);
		} else {
			(void) NewtThrow(kNErrNotAString, inNSValue);
			theResult = false;
		}
	} else if ((NewtSymbolEqual(inNSType, NSSYM(binary)))
		|| (NewtSymbolEqual(inNSType, NSSYM(iobinary)))) {
		*outFFIType = &ffi_type_pointer;
		if (NewtRefIsBinary(inNSValue))
		{
			*((void**) outFFIValue) =
				(void*) NewtRefToBinary(inNSValue);
			theSize = sizeof(void*);
		} else {
			(void) NewtThrow(kNErrNotABinaryObject, inNSValue);
			theResult = false;
		}
	} else if (NewtSymbolEqual(inNSType, NSSYM(pointer))) {
		*outFFIType = &ffi_type_pointer;
		if (NewtRefIsInteger(inNSValue))
		{
			*((void**) outFFIValue) =
				(void*) NewtRefToInteger(inNSValue);
			theSize = sizeof(void*);
		} else if (NewtRefIsBinary(inNSValue)) {
			*((void**) outFFIValue) =
				NativeCalls_BinaryToPointer(inNSValue);
			theSize = sizeof(void*);
		} else {
			(void) NewtThrow(kNErrNotABinaryObject, inNSValue);
			theResult = false;
		}
	} else {
		(void) NewtThrow(kNErrNotASymbol, inNSType);
		theResult = false;
	}
	
	if (outValueSize)
	{
		*outValueSize = theSize;
	}
	
	return theResult;
}

/**
 * Generic (global) function with any number of arguments.
 */
newtRef
NativeCalls_GenericFunction(newtRef inRcvr, newtRef inArgs)
{
	ffi_cif		cif;
	ffi_type**	ffiArgsTypes;
	void**		argsValues;
	SBufferZone	storage;
	int			nbArgs;
	SStorage	result;
	newtRefVar	resultRef = kNewtRefNIL;
	ffi_type*	ffiResultType;
	void*		theHandle;
	newtRefVar	theHandleRef;
	void*		theSymbolPtr;
	char*		theSymbolCStr;
	bool 		typeError = false;	

	/* Retrieve informations on the function */
	newtRefVar theCurrentFunc = NVMCurrentFunction();
	newtRefVar argTypes = NcGetSlot(theCurrentFunc, NSSYM(_argTypes));
	newtRefVar resultType = NcGetSlot(theCurrentFunc, NSSYM(_resultType));
	newtRefVar nativeName = NcGetSlot(theCurrentFunc, NSSYM(_nativeName));
	newtRefVar nativeLib = NcGetSlot(theCurrentFunc, NSSYM(_lib));
	
	/* check the parameters */
	if (!NewtRefIsString(nativeName))
	{
		return NewtThrow(kNErrNotAString, nativeName);
	}
	if (!NewtRefIsArray(argTypes))
	{
		return NewtThrow(kNErrNotAnArray, argTypes);
	}
	if (!NewtRefIsSymbol(resultType))
	{
		return NewtThrow(kNErrNotASymbol, resultType);
	}

	/* check the number of arguments */	
	nbArgs = NewtArrayLength(argTypes);
	if (nbArgs != NewtArrayLength(inArgs))
	{
		return NewtThrow(kNErrWrongNumberOfArgs, nativeName);
	}

	/* get the library */
	if (!NewtRefIsFrame(nativeLib))
	{
		return NewtThrow(kNErrNotAFrame, nativeLib);
	}
	theHandleRef = NcGetSlot(nativeLib, NSSYM(_handle));
	theHandle = NativeCalls_BinaryToPointer(theHandleRef);
	if (theHandle == NULL)
	{
		/* some problem occurred */
		return NewtThrow(kNErrNative, NewtMakeString(dlerror(), false));
	}

	/* resolve the symbol */
	theSymbolCStr = NewtRefToString(nativeName);
	theSymbolPtr = dlsym(theHandle, theSymbolCStr);
	
	if (theSymbolPtr == NULL)
	{
		/* dlsym failed, let's throw the error string */
		return NewtThrow(kNErrNative, NewtMakeString(dlerror(), false));
	}

	/* build the ffi lists */
	NativeCalls_AllocateBuffer(&storage);
	NativeCalls_CastTypesAndValues(nbArgs, argTypes, inArgs, &ffiArgsTypes, &argsValues, NULL, &storage);
	
	if (!typeError)
	{
		if (!NativeCalls_CastType( resultType, &ffiResultType ))
		{
			typeError = true;
		}
	}
	
	if (!typeError)
	{
		if (ffi_prep_cif(
				&cif, FFI_DEFAULT_ABI, nbArgs,
				ffiResultType, ffiArgsTypes) == FFI_OK)
		{
			ffi_call(&cif, theSymbolPtr, &result, argsValues);
			
			/* Cast the result. */
			resultRef = NativeCalls_CastResult(resultType, &result);
		} else {
			resultRef = NewtThrow(
						kNErrNative,
						NewtMakeString("ffi_prep_cif failed", true));
		}
	}

	NativeCalls_ReleaseBuffer(&storage);

	return resultRef;
}

#pragma mark -

/**
 * Native function OpenNativeLibrary(name)
 *
 * name is a string representing the library name.
 *
 * @param inRcvr	self (ignored)
 * @param inName	library name
 * @return NIL
 */
newtRef
NativeCalls_OpenNativeLibrary(
	newtRefArg inRcvr,
	newtRefArg inName)
{
	newtRefVar theHandleRef;
	newtRefVar result = kNewtRefNIL;
	
	(void) inRcvr;
	
	/* check parameters */
	if (!NewtRefIsString(inName))
	{
		return NewtThrow(kNErrNotAString, inName);
	}

	/* grab the handle on the lib */
	theHandleRef = NativeCalls_OpenLib(NewtRefToString(inName));
	if (NewtRefIsNotNIL(theHandleRef))
	{
		/* create the frame */
		result = NcMakeFrame();
		NcSetSlot(result, NSSYM(_handle), theHandleRef);
		NcSetSlot(result, NSSYM(_parent),
			NcResolveMagicPointer(NewtSymbolToMP(kLibParentMagicPtrKey)));
	}
	
	return result;
}

/**
 * Native function lib:GetFunction(specs)
 *
 * specs is a frame defining the native function. It includes the following
 * slots:
 *
 * name			(string) name of the native function to call
 * args			(array of symbols) types of the arguments.
 * result		(symbol) type of the result.
 * 
 * Hopefully you know what you're doing with types being correct.
 * When calling the function, values will be cast to the proper type.
 * The table below summarizes what Newton types can be used to perform the cast.
 *
 * Type			Newton type			ffi type
 *  'void		nil					ffi_type_void			not available for params
 *  'uint8		int					ffi_type_uint8
 *  'sint8		int					ffi_type_sint8
 *  'uint16		int					ffi_type_uint16
 *  'sint16		int					ffi_type_sint16
 *  'uint32		int (or binary)		ffi_type_uint32
 *  'sint32		int (or binary)		ffi_type_sint32
 *  'uint64		int (or binary)		ffi_type_uint64
 *  'sint64		int (or binary)		ffi_type_sint64
 *  'float		real				ffi_type_float
 *  'double		real				ffi_type_double
 *  'longdouble	real				ffi_type_longdouble
 *  'string		string				ffi_type_pointer
 *  'iostring	string				ffi_type_pointer		not available for result
 *	'binary		binary				ffi_type_pointer		not available for result
 *	'iobinary	binary				ffi_type_pointer		not available for result
 *	'pointer	int	or binary		ffi_type_pointer
 *  [array]		array				ffi_type_struct			not available for result
 *
 * string and iostring are identical (strings are I/O). Same with binary and
 * iobinary. This may change in the future.
 *
 * @param inRcvr	self
 * @param inSpec	specification frame
 * @return NIL
 */
newtRef
NativeCalls_GetFunction(
	newtRefArg inRcvr,
	newtRefArg inSpec)
{
	newtRefVar argTypes;
	newtRefVar resultType;
	newtRefVar functionName;
	newtRefVar functionObject;

	/* check self */
	if (!NewtRefIsFrame(inRcvr))
	{
		return NewtThrow(kNErrNotAFrame, inRcvr);
	}
	/* check that the library wasn't closed */
	if (!NewtRefIsBinary(NcGetSlot(inRcvr, NSSYM(_handle))))
	{
		return NewtThrow(kNErrNotABinaryObject, NcGetSlot(inRcvr, NSSYM(_handle)));
	}
	/* check parameters */
	if (!NewtRefIsFrame(inSpec))
	{
		return NewtThrow(kNErrNotAFrame, inSpec);
	}
	argTypes = NcGetSlot(inSpec, NSSYM(args));
	if (!NewtRefIsArray(argTypes))
	{
		return NewtThrow(kNErrNotAnArray, argTypes);
	}
	resultType = NcGetSlot(inSpec, NSSYM(result));
	if (!NewtRefIsSymbol(resultType))
	{
		return NewtThrow(kNErrNotASymbol, resultType);
	}
	functionName = NcGetSlot(inSpec, NSSYM(name));
	if (!NewtRefIsString(functionName))
	{
		return NewtThrow(kNErrNotAString, functionName);
	}
	
	/* create the function object */
	functionObject = 
		NewtMakeNativeFunc0(NativeCalls_GenericFunction, 0, true, (char*) NewtRefToString(functionName));
	
	/* stuff information to call the native function */
	NcSetSlot(functionObject, NSSYM(_lib), inRcvr);
	NcSetSlot(functionObject, NSSYM(_nativeName), functionName);
	NcSetSlot(functionObject, NSSYM(_resultType), resultType);
	NcSetSlot(functionObject, NSSYM(_argTypes), argTypes);
	
	return functionObject;
}

/**
 * Native function lib:DefGlobalFn(symbol, specs)
 *
 * symbol is the symbol of the function to define.
 * specs is a frame defining the native function. It is passed to GetFunction.
 *
 * The symbol of the function is added to a list of the library object to allow
 * Close to undefine the function.
 *
 * @param inRcvr	self
 * @param inSymbol	function symbol
 * @param inSpec	specification frame
 * @return NIL
 */
newtRef
NativeCalls_DefGlobalFn(
	newtRefArg inRcvr,
	newtRefArg inSymbol,
	newtRefArg inSpec)
{
	newtRefVar functionObject;
	newtRefVar libList;
	
	/* check the function doesn't exist yet */
	if (NewtHasGlobalFn(inSymbol))
	{
		return NewtThrow(
				kNErrNative,
				NewtMakeString("Global function already exists", true));
	}
	
	/* Get the object */
	functionObject = NativeCalls_GetFunction(inRcvr, inSpec);
	
	/* add the function to the list */
	libList = NcGetSlot(inRcvr, NSSYM(_globalFns));
	if (NewtRefIsNIL(libList))
	{
		libList = NewtMakeArray(kNewtRefNIL, 0);
		NcSetSlot(inRcvr, NSSYM(_globalFns), libList);
	}
	NcAddArraySlot(libList, inSymbol);
	
	/* define the global function */
	NcDefGlobalFn(inSymbol, functionObject);
	
	return inSymbol;
}

/**
 * Native function GetErrno()
 *
 * @param inRcvr	self.
 */
newtRef
NativeCalls_GetErrno(newtRefArg inRcvr)
{
	(void) inRcvr;

	return NewtMakeInteger(errno);
}

/**
 * Native function lib:Close()
 *
 * Cleans up the references to an open library.
 *
 * @param inRcvr	self.
 * @return nil
 */
newtRef
NativeCalls_Close(newtRefArg inRcvr)
{
	newtRefVar theHandleRef;
	newtRefVar theGlobalFnsList;
	
	/* undef all the global functions from this library */
	theGlobalFnsList = NcGetSlot(inRcvr, NSSYM(_globalFns));
	if (NewtRefIsNotNIL(theGlobalFnsList))
	{
		int indexFns, nbFns;

		nbFns = NewtArrayLength(theGlobalFnsList);
		for (indexFns = 0; indexFns < nbFns; indexFns++)
		{
			newtRefVar theFnSym;
			
			theFnSym = NewtGetArraySlot(theGlobalFnsList, indexFns);
			NcUndefGlobalFn(theFnSym);
		}
	}
	
	NcRemoveSlot(inRcvr, NSSYM(_globalFns));

	theHandleRef = NcGetSlot(inRcvr, NSSYM(_handle));
	/* close the library */
	if (NewtRefIsNotNIL(theHandleRef))
	{
		void* theHandle;
	
		/* Get the handle */
		theHandle = NativeCalls_BinaryToPointer(theHandleRef);
		
		if (theHandle)
		{
			/* close the library, ignoring errors */
			(void) dlclose(theHandle);
		}
		
		NcRemoveSlot(inRcvr, NSSYM(_handle));
	}

	return kNewtRefNIL;
}

/*------------------------------------------------------------------------*/
/**
 * Install the global functions and the global variable.
 */
void NativeCalls_install(void) 
{
	newtRefVar magicPtr;
  
	// Add the OpenNativeLibrary global function.
	NewtDefGlobalFunc(
                    NSSYM(OpenNativeLibrary),
                    NativeCalls_OpenNativeLibrary,
                    1,
                    "OpenNativeLibrary(lib)");
  
	// Add the GetErrno global function.
	NewtDefGlobalFunc(
                    NSSYM(GetErrno),
                    NativeCalls_GetErrno,
                    0,
                    "GetErrno()");
  
	// Create magic pointer (for libraries methods).
	magicPtr = NcMakeFrame();
  
	NcSetSlot(magicPtr,
            NSSYM(Close),
            NewtMakeNativeFunc(
                               NativeCalls_Close, 0, "Close()"));
	NcSetSlot(magicPtr,
            NSSYM(GetFunction),
            NewtMakeNativeFunc(
                               NativeCalls_GetFunction, 1, "GetFunction(spec)"));
	NcSetSlot(magicPtr,
            NSSYM(DefGlobalFn),
            NewtMakeNativeFunc(
                               NativeCalls_DefGlobalFn, 2, "DefGlobalFn(symbol, spec)"));
  
	NcDefMagicPointer(kLibParentMagicPtrKey, magicPtr);
}

#if !TARGET_OS_IPHONE
void newt_install(void)
{
  NativeCalls_install();
}
#endif