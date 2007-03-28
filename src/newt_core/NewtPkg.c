/*------------------------------------------------------------------------*/
/**
 * @file	NewtPkg.c
 * @brief   Newton Package Format
 *
 * @author  Matthias Melcher
 * @date	2007-03-25
 *
 * Copyright (C) 2007 Matthias Melcher, All rights reserved.
 */


/* header files */
#include <string.h>

#include "NewtPkg.h"
#include "NewtErrs.h"
#include "NewtObj.h"
#include "NewtEnv.h"
#include "NewtFns.h"
#include "NewtVM.h"
#include "NewtIconv.h"

#include "utils/endian_utils.h"

#undef DEBUG_PKG_DIS
#define DEBUG_PKG
#ifdef DEBUG_PKG
#	include "NewtPrint.h"
#endif

/* macros */

/// Test first 8 bytes for package signature
#define PkgIsPackage(data) ((strncmp(data, "package0", 8)==0) || (strncmp(data, "package1", 8)==0))


/* types */

/// info refs are a shorthand version of pointers into a userdata area of the package
typedef struct {
	uint16_t	offset;			///< offset from the flexible size header block
	uint16_t	size;			///< size of data block
} pkg_info_ref_t;

/// package header structure
typedef struct {
	uint8_t		sig[8];			///< 8 byte signature
	uint8_t		type[4];		///< 4 bytes describing package class
	uint32_t	flags;			///< describing how to store and load the package
	uint32_t	version;		///< user definable; higher numbers are newer version
	pkg_info_ref_t copyright;	///< utf16 copyright message
	pkg_info_ref_t name;		///< utf16 package name
	uint32_t	size;			///< entire size of package
	uint32_t	date;			///< creation date
	uint32_t	reserverd2;
	uint32_t	reserverd3;
	uint32_t	directorySize;	///< size in bytes of the following block
	uint32_t	numParts;		///< number of parts in this package
} pkg_header_t;

//// package part descriptor
typedef struct {
	uint32_t	offset;			///< offset to beginning of part section
	uint32_t	size;			///< size of part in bytes
	uint32_t	size2;			///< as above (maybe once intended to hold the compressed size)
	uint32_t	type;			///< type of part
	uint32_t	reserverd1;
	uint32_t	flags;			///< some more information about this part
	pkg_info_ref_t info;		///< activation data; type depends on part type
	uint32_t	reserverd2;
} pkg_part_t;

/// structure to manage the package reading process
typedef struct {
	int8_t		verno;			///< r  package version 0 or 1
	uint8_t *	data;			///< rw package data
	uint32_t	size;			///< rw size of package
	uint32_t	data_size;		///< w  size of data block
	pkg_header_t *header;		///< r  pointer to the package header
	uint32_t	header_size;	///< w  size of header structure w/o var data
	uint8_t *	var_data;		///< r  extra data for header
	uint32_t	var_data_size;	///< w  size of variable length data block
	uint32_t	num_parts;		///< r  number of parts in package
	pkg_part_t*	part_headers;	///< r  pointer to first part header - can be used as an array
	pkg_part_t*	part_header;	///< r  header of current part
	uint8_t *	part;			///< r  start of current part data
	uint32_t	part_offset;	///< r  offset of current part to the beginning of data
	newtRefVar	instances;		///< rw array holding the previously generated instance of any ref per part
	newtRefVar	precedents;		///< w  array referencing the instances array
	newtErr		lastErr;		///< r  a way to return error from deep below
#ifdef HAVE_LIBICONV
	iconv_t		from_utf16;		///< r  strings in compatible packages are UTF16
	iconv_t		to_utf16;		///< w  strings in compatible packages are UTF16
#endif /* HAVE_LIBICONV */
} pkg_stream_t;


/* functions */
static newtRef	PkgPartGetPrecedent(pkg_stream_t *pkg, newtRefArg r);
static void		PkgPartSetPrecedent(pkg_stream_t *pkg, newtRefArg r, newtRefArg val);

static newtRef	PkgWriteObject(pkg_stream_t *pkg, newtRefArg obj);

static newtRef	PkgPartGetInstance(pkg_stream_t *pkg, uint32_t p_obj);
static void		PkgPartSetInstance(pkg_stream_t *pkg, uint32_t p_obj, newtRefArg r);

static uint32_t	PkgReadU32(uint8_t *d);
static newtRef	PkgReadRef(pkg_stream_t *pkg, uint32_t p_obj);
static newtRef	PkgReadBinaryObject(pkg_stream_t *pkg, uint32_t p_obj);
static newtRef	PkgReadArrayObject(pkg_stream_t *pkg, uint32_t p_obj);
static newtRef	PkgReadFrameObject(pkg_stream_t *pkg, uint32_t p_obj);
static newtRef	PkgReadObject(pkg_stream_t *pkg, uint32_t p_obj);
static newtRef	PkgReadPart(pkg_stream_t *pkg, int32_t index);
static newtRef	PkgReadVardataString(pkg_stream_t *pkg, pkg_info_ref_t *info_ref);
static newtRef	PkgReadVardataBinary(pkg_stream_t *pkg, pkg_info_ref_t *info_ref);
static newtRef	PkgReadHeader(pkg_stream_t *pkg);

/** Duplicate of same function in NSOF.
 * Maybe we could make the original file publicly accessible
 */
int32_t PkgArraySearch(newtRefArg array, newtRefArg r)
{
    newtRef *	slots;
	uint32_t	len;
	uint32_t	i;

	len = NewtArrayLength(array);
    slots = NewtRefToSlots(array);

	for (i = 0; i < len; i++)
	{
		if (slots[i] == r)
			return i;
	}

	return -1;
}

/*------------------------------------------------------------------------*/
/** Verify if the give reference was already written to the package.
 *
 * @param pkg		[inout] the package
 * @param ref		[in] the reference that we try to find
 * 
 * @retval	ref to the previously written object
 * @retval	or kNewtRefUnbind if the object still needs to be written
 */
newtRef PkgPartGetPrecedent(pkg_stream_t *pkg, newtRefArg ref)
{
	int32_t ix = PkgArraySearch(pkg->precedents, ref);
	if (ix>=0) {
		printf("*** Multiple: ");
		NewtPrintObject(stdout, ref);
		return NewtGetArraySlot(pkg->instances, ix);
	} else {
		return kNewtRefUnbind;
	}
}

/*------------------------------------------------------------------------*/
/** Remember that a specific reference was already written.
 *
 * @param pkg		[inout] the package
 * @param ref		[in] the original reference in memory
 * @param val		[in] the reference needed to find this object in the package
 */
void PkgPartSetPrecedent(pkg_stream_t *pkg, newtRefArg ref, newtRefArg val)
{
	uint32_t n = NewtArrayLength(pkg->instances);
	NewtInsertArraySlot(pkg->instances, n, val);
	NewtInsertArraySlot(pkg->precedents, n, ref);
}

/*------------------------------------------------------------------------*/
/** Align the given value to 8 or 4 bytes.
 * 
 * @param pkg		[inout] the package
 * @param offset	[in] offset to be aligned
 *
 * @retval	new offset aligned to 8 bytes, or 4 bytes for the advance format
 *
 * @todo We shoudld add 4-byte alignment support at some point, so the 
 *       generated package is a bit smaller
 */
uint32_t PkgAlign(pkg_stream_t *pkg, uint32_t offset)
{
	return (offset+7)&(~7);
}

/*------------------------------------------------------------------------*/
/** Convenience function to get the integer value of a slot.
 * 
 * @param frame		[in] find the slot in this frame
 * @param name		[in] name of slot
 * @param def		[in] default value if slot was not found
 *
 * @retval	value read if found, or default value if not found
 */
uint32_t PkgGetSlotInt(newtRefArg frame, newtRefArg name, uint32_t def)
{
	newtRef slot;
	int32_t ix;
	
	ix = NewtFindSlotIndex(frame, name);
	if (ix<0)
		return def;
	slot = NewtGetArraySlot(frame, ix);
	if (NewtRefIsInteger(slot)) {
		return NewtRefToInteger(slot);
	} else {
		return def;
	}
}

/*------------------------------------------------------------------------*/
/** Make some room for more data in the package
 * 
 * @param pkg		[inout] the package
 * @param offset	[in] offset where we will need room
 * @param size	[in] number of bytes that we will need
 */
void PkgMakeRoom(pkg_stream_t *pkg, uint32_t offset, uint32_t size)
{
	uint32_t new_size = offset+size;

	if (pkg->data_size<new_size) {
		uint32_t os = pkg->data_size;
		uint32_t ns = (new_size+16383) & (~16383); // alocate ein 16k blocks
		pkg->data = realloc(pkg->data, ns);
		memset(pkg->data+os, 0xbf, ns-os); // filler byte
		pkg->data_size = ns;
	}

	if (pkg->size<new_size)
		pkg->size = new_size;
}

/*------------------------------------------------------------------------*/
/** Write arbitrary data into the package at any position
 * 
 * @param pkg		[inout] the package
 * @param offset	[in] offset to the beginning of data
 * @param data		[in] data to be copied to package
 * @param size		[in] size of data block
 */
void PkgWriteData(pkg_stream_t *pkg, uint32_t offset, void *data, uint32_t size)
{
	if (pkg->data_size<offset+size) 
		PkgMakeRoom(pkg, offset, size);
	if (pkg->size<offset+size)
		pkg->size = offset+size;
	memcpy(pkg->data+offset, data, size);
}

/*------------------------------------------------------------------------*/
/** Write a 32 bit integer into the package at any position
 * 
 * @param pkg		[inout] the package
 * @param offset	[in] offset to the beginning of data
 * @param v			[in] value to be written 
 */
void PkgWriteU32(pkg_stream_t *pkg, uint32_t offset, uint32_t v)
{
	v = htonl(v);
	PkgWriteData(pkg, offset, &v, 4);
}

/*------------------------------------------------------------------------*/
/** Write the data in the object into the vardata section and generate the info ref
 * 
 * @param pkg		[inout] the package
 * @param offset	[in] whgere to write the info ref
 * @param frame		[in] frame containing the data 
 * @param sym		[in] symbol of slot containing the data
 */
void PgkWriteVarData(pkg_stream_t *pkg, uint32_t offset, newtRefVar frame, newtRefVar sym)
{
	newtRef info;
	uint32_t ix;
	
	PkgWriteU32(pkg, offset, 0);

	ix = NewtFindSlotIndex(frame, sym);
	if (ix<0) 
		return;
	info = NewtGetFrameSlot(frame, ix);
	if (NewtRefIsBinary(info)) {
		uint32_t size = NewtBinaryLength(info);
		uint8_t *data = NewtRefToBinary(info);
		pkg_info_ref_t info_ref;

#		ifdef HAVE_LIBICONV
			if (NewtRefIsString(info)) {
				size_t buflen;
				char *buf = NewtIconv(pkg->to_utf16, data, size, &buflen);
				if (buf) {
					size = buflen;
					data = buf;
				}
			}
#		endif /* HAVE_LIBICONV */

		info_ref.offset = htons(pkg->var_data_size);
		info_ref.size = htons(size);

		PkgWriteData(pkg, pkg->header_size + pkg->var_data_size, data, size);
		PkgWriteData(pkg, offset, &info_ref, 4);

		pkg->var_data_size += size;		
	}
}

/*------------------------------------------------------------------------*/
/** Append a frame object to the end of the Package
 * 
 * @param pkg		[inout] the package
 * @param frame		[in] the frame that we will write
 *
 * @retval	offset to the beginning of the object in the package file
 */
newtRef PkgWriteFrame(pkg_stream_t *pkg, newtRefArg frame)
{
	uint32_t dst, size, i, n;
	newtRef map, map_pos;
	newtRef slot, slot_pos;

	// calculate the size of this chunk
	dst = PkgAlign(pkg, pkg->size);
	n = NewtFrameLength(frame);
	size = (n+3)*4;

	// make room for the entire chunk and write the header
	PkgMakeRoom(pkg, dst, size);
	PkgWriteU32(pkg, dst, (size<<8) | 0x40 | kObjSlotted | kObjFrame);
	PkgWriteU32(pkg, dst+4, 0);

	// recursively add all arrays making up the map
	map = NewtFrameMap(frame);
	map_pos = PkgWriteObject(pkg, map);
	PkgWriteU32(pkg, dst+8, map_pos);

	// now add all slots and fill in the rest of our chunk
	for (i=0; i<n; i++) {
		slot = NewtGetFrameSlot(frame, i);
		slot_pos = PkgWriteObject(pkg, slot);
		PkgWriteU32(pkg, dst+12+4*i, slot_pos);
	}

	return NewtMakePointer(dst);
}

/*------------------------------------------------------------------------*/
/** Append an array object to the end of the Package
 * 
 * @param pkg		[inout] the package
 * @param array		[in] the array that we will write
 *
 * @retval	offset to the beginning of the object in the package file
 */
newtRef PkgWriteArray(pkg_stream_t *pkg, newtRefArg array)
{
	uint32_t dst, size, i, n;
	newtRef klass, klass_pos;
	newtRef slot, slot_pos;

	// calculate the size of this chunk
	dst = PkgAlign(pkg, pkg->size);
	n = NewtArrayLength(array);
	size = (n+3)*4;

	// make room for the entire chunk and write the header
	PkgMakeRoom(pkg, dst, size);
	PkgWriteU32(pkg, dst, (size<<8) | 0x40 | kObjSlotted);
	PkgWriteU32(pkg, dst+4, 0);

	// add the class information
	klass = NcClassOf(array);
	klass_pos = PkgWriteObject(pkg, klass);
	PkgWriteU32(pkg, dst+8, klass_pos);

	// now add all slots and fill in the rest of our chunk
	for (i=0; i<n; i++) {
		slot = NewtGetArraySlot(array, i);
		slot_pos = PkgWriteObject(pkg, slot);
		PkgWriteU32(pkg, dst+12+4*i, slot_pos);
	}

	return NewtMakePointer(dst);
}

/*------------------------------------------------------------------------*/
/** Append a binary object to the end of the Package
 * 
 * @param pkg		[inout] the package
 * @param obj		[in] the binary object that we will write
 *
 * @retval	offset to the beginning of the object in the package file
 */
newtRef PkgWriteBinary(pkg_stream_t *pkg, newtRefArg obj)
{
	uint32_t dst, size;
	uint8_t *data;
	newtRef klass, klass_ref = kNewtRefUnbind;

	// calculate the size of this chunk
	dst = PkgAlign(pkg, pkg->size);
	size = NewtBinaryLength(obj);
	data = NewtRefToBinary(obj);

	// make room for the binary chunk and write the header
	if (NewtRefIsSymbol(obj)) {
		size = NewtSymbolLength(obj)+5; // remember the trailing zero!
	} else if (NewtRefIsString(obj)) {
#		ifdef HAVE_LIBICONV
			size_t buflen;
			char *buf = NewtIconv(pkg->to_utf16, data, size, &buflen);
			if (buf) {
				size = buflen;
				data = buf;
			}
#		endif /* HAVE_LIBICONV */
	}
	size += 12;

	// make room for the binary chunk and write the header
	PkgMakeRoom(pkg, dst, size);
	PkgWriteU32(pkg, dst, (size<<8) | 0x40);
	PkgWriteU32(pkg, dst+4, 0);

	// symbols have special handling to avoid recursion
	if (NewtRefIsSymbol(obj)) {
		PkgWriteU32(pkg, dst+8, kNewtSymbolClass);
		PkgWriteU32(pkg, dst+12, NewtRefToHash(obj)); // make sure the hash has the right endianness
		PkgWriteData(pkg, dst+16, (uint8_t*)NewtSymbolGetName(obj), size-16);
		return NewtMakePointer(dst);
	}

	// add the class information
	klass = NcClassOf(obj);
	klass_ref = PkgWriteObject(pkg, klass);
	PkgWriteU32(pkg, dst+8, klass_ref);

	// add a verbose copy of the binary data
	PkgWriteData(pkg, dst+12, data, size-12);

	return NewtMakePointer(dst);
}

/*------------------------------------------------------------------------*/
/** Append the object to the end of the Package
 * 
 * @param pkg		[inout] the package
 * @param obj		[in] the object that we will write
 *
 * @retval	offset to the beginning of the object in the package file
 */
newtRef PkgWriteObject(pkg_stream_t *pkg, newtRefArg obj)
{
	uint32_t dst = pkg->size;
	newtRef prec;

	if (NewtRefIsImmediate(obj)) {
		printf("*** immediate: ");
		NewtPrintObject(stdout, obj);
		return obj;
	} 
	
	prec = PkgPartGetPrecedent(pkg, obj);
	if (prec!=kNewtRefUnbind) {
		return prec;
	}

	if (NewtRefIsFrame(obj)) {
		dst = PkgWriteFrame(pkg, obj);
	} else if (NewtRefIsArray(obj)) {
		dst = PkgWriteArray(pkg, obj);
	} else if (NewtRefIsBinary(obj)) {
		dst = PkgWriteBinary(pkg, obj);
	} else {
		printf("*** unsupported write: ");
		NewtPrintObject(stdout, obj);
		PkgMakeRoom(pkg, dst, 16);
		PkgWriteU32(pkg, dst, 0xdeadbeef);
		// FIXME add all possible types...
	}

	// make this ref available for later incarnations of the same object
	PkgPartSetPrecedent(pkg, obj, dst);

	return dst;
}

/*------------------------------------------------------------------------*/
/** Create a part in package format based on this object.
 * 
 * @param pkg		[inout] the package
 * @param part		[in] object containing part data
 *
 * @retval	ref to binary part data
 */
newtRef PkgWritePart(pkg_stream_t *pkg, newtRefArg part)
{
	uint32_t	dst = pkg->part_offset;
	int32_t		ix;
	newtRef		data;

	ix = NewtFindSlotIndex(part, NSSYM(data));
	if (ix<0) 
		return kNewtRefNIL;
	data = NewtGetFrameSlot(part, ix);

	pkg->instances = NewtMakeArray(kNewtRefUnbind, 0);
	pkg->precedents = NewtMakeArray(kNewtRefUnbind, 0);

	PkgMakeRoom(pkg, dst, 16);
	PkgWriteU32(pkg, dst,    0x00001041);
	PkgWriteU32(pkg, dst+4,  0x00000000);
	PkgWriteU32(pkg, dst+8,  0x00000002);
	PkgWriteU32(pkg, dst+12, PkgWriteObject(pkg, data));

	NewtSetLength(pkg->precedents, 0);
	NewtSetLength(pkg->instances, 0);

	return kNewtRefNIL;
}

/*------------------------------------------------------------------------*/
/** Create a new binary object that conatins the object tree in package format
 * 
 * @param rpkg	[in] object tree describing the package
 *
 * @retval	binary object with package
 */
newtRef NewtWritePkg(newtRefArg package)
{
	pkg_stream_t	pkg;
	int32_t			num_parts, i, ix, directory_size;
	newtRef			parts;
	
	// setup pkg_stream_t
	memset(&pkg, 0, sizeof(pkg));

#	ifdef HAVE_LIBICONV
	{	char *encoding = NewtDefaultEncoding();
		pkg.to_utf16 = iconv_open("UTF-16BE", encoding);
	}
#	endif /* HAVE_LIBICONV */

	ix = NewtFindSlotIndex(package, NSSYM(parts));
	if (ix>=0) {
		parts = NewtGetFrameSlot(package, ix);
		num_parts = NewtFrameLength(parts);
		pkg.header_size = sizeof(pkg_header_t) + num_parts * sizeof(pkg_part_t);

		// start setting up the header with whatever we know
			// sig
		PkgWriteData(&pkg, 0, "package0", 8);
			// type
		PkgWriteData(&pkg, 8, "xxxx", 4);
			// flags
		PkgWriteU32(&pkg, 12, PkgGetSlotInt(package, NSSYM(flags), 0));
			// version
		PkgWriteU32(&pkg, 16, PkgGetSlotInt(package, NSSYM(version), 0));
			// copyright
		PgkWriteVarData(&pkg, 20, package, NSSYM(copyright));
			// name
		PgkWriteVarData(&pkg, 24, package, NSSYM(name));
			// date
		PkgWriteU32(&pkg, 32, 0xc2296aa5); // FIXME some fake creation date
			// reserved2
		PkgWriteU32(&pkg, 36, 0); 
			// reserved3
		PkgWriteU32(&pkg, 40, 0); 
			// numParts
		PkgWriteU32(&pkg, 48, num_parts);

		// calculate the size of the header so we can correctly set our refs in the parts
		for (i=0; i<num_parts; i++) {
			newtRef part = NewtGetArraySlot(parts, i);
			PgkWriteVarData(&pkg, sizeof(pkg_header_t) + i*sizeof(pkg_part_t) + 24, part, NSSYM(info));
		}

		// the original file has this (c) message embedded
		{	char msg[] = "Newtonª ToolKit Package © 1992-1997, Apple Computer, Inc.";
			PkgWriteData(&pkg, pkg.header_size + pkg.var_data_size, msg, sizeof(msg));
			pkg.var_data_size += sizeof(msg);
		}

		pkg.part_offset = directory_size = PkgAlign(&pkg, pkg.header_size + pkg.var_data_size);
			// directorySize
		PkgWriteU32(&pkg, 44, directory_size);

		// create all parts
		for (i=0; i<num_parts; i++) {
			uint32_t part_size;
			uint32_t hdr = sizeof(pkg_header_t) + i*sizeof(pkg_part_t);
			newtRef part = NewtGetArraySlot(parts, i);

			PkgWritePart(&pkg, part);
			PkgMakeRoom(&pkg, PkgAlign(&pkg, pkg.size), 0);
			part_size = pkg.size - pkg.part_offset;
				// offset
			PkgWriteU32(&pkg, hdr, pkg.part_offset - directory_size);
				// size & size2
			PkgWriteU32(&pkg, hdr+4, part_size);
			PkgWriteU32(&pkg, hdr+8, part_size);
				// type
			// FIXME the 'type' field is actually always a fourcc like 'form'
			// FIXME which Newt object should we use here? Binary? Int32? Symbol?
			PkgWriteU32(&pkg, hdr+12, PkgGetSlotInt(part, NSSYM(type), 0x666f726d)); 
				// reserved1
			PkgWriteU32(&pkg, hdr+16, 0); 
				// flags
			PkgWriteU32(&pkg, hdr+20, PkgGetSlotInt(part, NSSYM(flags), 0));
				// reserved2
			PkgWriteU32(&pkg, hdr+28, 0); 

			pkg.part_offset += part_size;
		}
	}

	// finish filling in the header
		// size
	PkgWriteU32(&pkg, 28, pkg.size);

	// convert pkg_stream_t into binary package
	// return new object

	{	FILE *f = fopen("d:/home/matt/dev/Newton/ntk2/helloWRT.pkg", "wb");
		fwrite(pkg.data, 1, pkg.size, f);
		fclose(f);
	}

#	ifdef HAVE_LIBICONV
		iconv_close(pkg.to_utf16);
#	endif /* HAVE_LIBICONV */

	return kNewtRefNIL;
}

/*------------------------------------------------------------------------*/
/** Endian-neutral conversion of four bytes into one uint32
 * 
 * @param d		[in] pointer to byte array
 *
 * @retval	uint32 assembled from four bytes
 */
uint32_t PkgReadU32(uint8_t *d) 
{
	return ((d[0]<<24)|(d[1]<<16)|(d[2]<<8)|(d[3]));
}

/*------------------------------------------------------------------------*/
/** Check if there was already an object created for the given offset.
 * 
 * @param pkg		[inout] the package
 * @param p_obj		[in] offset to object
 *
 * @retval	ref to previously create object or kNewtRefUnbind
 */
newtRef PkgPartGetInstance(pkg_stream_t *pkg, uint32_t p_obj)
{
	uint32_t ix = (p_obj - pkg->part_offset) / 4;
	return NewtGetArraySlot(pkg->instances, ix);
}

/*------------------------------------------------------------------------*/
/** Set the object ref for the given offset in the file
 * 
 * @param pkg		[inout] the package
 * @param r			[in] ref to new object
 * @param p_obj		[in] offset into data block
 */
void PkgPartSetInstance(pkg_stream_t *pkg, uint32_t p_obj, newtRefArg r)
{
	uint32_t ix = (p_obj - pkg->part_offset) / 4;
	NewtSetArraySlot(pkg->instances, ix, r);
}

/*------------------------------------------------------------------------*/
/** Interprete a ref in the Package data block.
 * 
 * @param pkg		[inout] the package
 * @param p_obj		[in] offset to Ref data relative to package start
 *
 * @retval	Newt version of Package Ref
 */
newtRef PkgReadRef(pkg_stream_t *pkg, uint32_t p_obj)
{
	uint32_t ref = PkgReadU32(pkg->data + p_obj);
	newtRef result = kNewtRefNIL;

	switch (ref&3) {
	case 0: // integer
		result = NSINT(ref>>2);
		break;
	case 1: // pointer
		result = PkgReadObject(pkg, ref&~3);
		break;
	case 2: // special
		if (ref==kNewtRefTRUE) result = kNewtRefTRUE;
		else if (ref==kNewtRefNIL) result = kNewtRefNIL;
		else if (ref&0x0f==0x0a) result = NewtMakeCharacter(ref>>4);
		else if (ref==kNewtSymbolClass) result = kNewtSymbolClass;
		else {
#			ifdef DEBUG_PKG
				printf("*** PkgReader: PkgReadRef - unknown ref 0x%08x\n", ref);
#			endif
			result = ref; 
		}
		break;
	case 3: // magic pointer
#		ifdef __NAMED_MAGIC_POINTER__
			result = kNewtRefNIL; // not implemented
#		else
			result = ref; // already a correct magic pointer
#		endif
		break;
	}

	return result;
}

/*------------------------------------------------------------------------*/
/** Generate a binary object from Package data
 * 
 * @param pkg		[inout] the package
 * @param p_obj		[in] offset to binary data object relative to package start
 *
 * @retval	Newt object version of binary object
 */
newtRef PkgReadBinaryObject(pkg_stream_t *pkg, uint32_t p_obj)
{
	uint32_t size = PkgReadU32(pkg->data + p_obj) >> 8;
	newtRef klass, result = kNewtRefNIL;
	newtRef ins = NSSYM0(instructions);

	klass = PkgReadRef(pkg, p_obj+8);

	if (klass==kNewtSymbolClass) {
		result = NewtMakeSymbol(pkg->data + p_obj + 16);
	} else if (klass==NSSYM0(string)) {
		char *src = pkg->data + p_obj + 12;
		int sze = size-12;
#		ifdef HAVE_LIBICONV
			size_t buflen;
			char *buf = NewtIconv(pkg->from_utf16, src, sze, &buflen);
			if (buf) {
				result = NewtMakeString2(buf, buflen-1, false); // NewtMakeString2 appends another null
			}
#		endif /* HAVE_LIBICONV */
		if (result==kNewtRefNIL)
			result = NewtMakeString2(src, sze, false);
	} else if (klass==NSSYM0(int32)) {
		uint32_t v = PkgReadU32(pkg->data + p_obj + 12);
		result = NewtMakeInt32(v);
	} else if (klass==NSSYM0(real)) {
		double *v = (double*)(pkg->data + p_obj + 12);
		result = NewtMakeReal(ntohd(*v));
	} else if (klass==NSSYM0(instructions)) {
		result = NewtMakeBinary(klass, pkg->data + p_obj + 12, size-12, false);
#		ifdef DEBUG_PKG_DIS
			printf("*** PkgReader: PkgReadBinaryObject - dumping byte code\n");
			NVMDumpBC(stdout, result);
#		endif
	} else {
		// bits: the bits in the icon
		// mask: the transparency mask for the icon
#		ifdef DEBUG_PKG
			if (klass) {
				printf("*** PkgReader: PkgReadBinaryObject - unknown class ");
				NewtPrintObject(stdout, klass);
			}
#		endif
		result = NewtMakeBinary(klass, pkg->data + p_obj + 12, size-12, false);
	}

	return result;
}

/*------------------------------------------------------------------------*/
/** Generate an Array Object from Package data
 * 
 * @param pkg		[inout] the package
 * @param p_obj		[in] offset to array data relative to package start
 *
 * @retval	Newt object version of package Array
 */
newtRef PkgReadArrayObject(pkg_stream_t *pkg, uint32_t p_obj)
{
	uint32_t size = PkgReadU32(pkg->data + p_obj) >> 8;
	uint32_t num_slots = size/4 - 3;
	uint32_t i;
	newtRef array, klass;

	klass = PkgReadRef(pkg, p_obj+8);
	array = NewtMakeArray(klass, num_slots);

	if (NewtRefIsNotNIL(array)) {
		for (i=0; i<num_slots; i++) {
			NewtSetArraySlot(array, i, PkgReadRef(pkg, p_obj+12 + 4*i));
		}
	}

	return array;
}


/*------------------------------------------------------------------------*/
/** Generate a Frame Object from Package data
 * 
 * @param pkg		[inout] the package
 * @param p_obj		[in] offset to frame data relative to package start
 *
 * @retval	Newt object version of package Frame
 */
newtRef PkgReadFrameObject(pkg_stream_t *pkg, uint32_t p_obj)
{
	uint32_t size = PkgReadU32(pkg->data + p_obj) >> 8;
	uint32_t i, num_slots = size/4 - 3;
	newtRef frame = kNewtRefNIL, map;

	map = PkgReadRef(pkg, p_obj+8);

	frame = NewtMakeFrame(map, num_slots);

	if (NewtRefIsNotNIL(frame)) {

        newtRef *slot = NewtRefToSlots(frame);
		for (i=0; i<num_slots; i++) {
			slot[i] = PkgReadRef(pkg, p_obj+12 + 4*i);
		}
	}

	return frame;
}

/*------------------------------------------------------------------------*/
/** Recursively read the object at the given offset
 * 
 * @param pkg		[inout] the package
 * @param p_obj		[in] offset to object relative to package start
 *
 * @retval	Newt object version of package object
 */
newtRef PkgReadObject(pkg_stream_t *pkg, uint32_t p_obj)
{
	uint32_t obj = PkgReadU32(pkg->data + p_obj);
	newtRef ret = PkgPartGetInstance(pkg, p_obj);

	// avoid generating objects twice
	if (ret!=kNewtRefUnbind)
		return ret;

	// create an object based on its low 8 bit type
	switch (obj & 0xff) {
	case 0x40: // binary object
		ret = PkgReadBinaryObject(pkg, p_obj);
		break;
	case 0x40 | kObjSlotted: // array
		ret = PkgReadArrayObject(pkg, p_obj);
		break;
	case 0x40 | kObjSlotted | kObjFrame: // frame
		ret = PkgReadFrameObject(pkg, p_obj);
		break;
	case 0x40 | kObjFrame: // not defined
	default:
#		ifdef DEBUG_PKG
			printf("*** PkgReader: PkgReadObject - unsupported object 0x%08x\n", obj);
#		endif
		break;
	}

	// remember that we created this object
	PkgPartSetInstance(pkg, p_obj, ret);

	return ret;
}

/*------------------------------------------------------------------------*/
/** Read a NOS part from the package file.
 * 
 * @param pkg		[inout] the package
 *
 * @retval	Newt object with contents of NOS part
 */
newtRef PkgReadNOSPart(pkg_stream_t *pkg)
{
	uint32_t	p_obj;
	newtRefVar	result;

	// verify that we have a correct header 
	if (PkgReadU32(pkg->part)!=0x00001041 || PkgReadU32(pkg->part+8)!=0x00000002) {
#		ifdef DEBUG_PKG
		printf("*** PkgReader: PkgReadPart - unsupported NOS Part intro at %d\n",
			pkg->part-pkg->data);
#		endif
		return kNewtRefNIL;
	}
	
	// create an array that holds a ref to all creted objects, avoiding double instantiation
	if (pkg->instances) 
		NewtSetLength(pkg->instances, ntohl(pkg->part_header->size)/4);
	else
		pkg->instances = NewtMakeArray(kNewtRefUnbind, ntohl(pkg->part_header->size)/4);

	// now recursively load all objects
	p_obj = PkgReadU32(pkg->part+12);
	result = PkgReadObject(pkg, p_obj&~3);

	// release our helper array
	NewtSetLength(pkg->instances, 0);

	return result;
}

/*------------------------------------------------------------------------*/
/** Read a part from the package file.
 * 
 * @param pkg		[inout] the package
 * @param index		[in] the index of the part to read starting at 0
 *
 * @retval	Newt object with contents of part
 */
newtRef PkgReadPart(pkg_stream_t *pkg, int32_t index)
{
	uint32_t	flags;
	newtRefVar	frame;
    newtRefVar	ptv[] = {
                            NS_CLASS,				NSSYM(PackagePart),
                            NSSYM(info),			kNewtRefNIL,
                            NSSYM(flags),			kNewtRefNIL,
                            NSSYM(type),			kNewtRefNIL,
                            NSSYM(data),			kNewtRefNIL
                        };

	pkg->part_header = pkg->part_headers + index;
	pkg->part_offset = ntohl(pkg->header->directorySize) + ntohl(pkg->part_header->offset);
	pkg->part = pkg->data + pkg->part_offset;
	flags = ntohl(pkg->part_header->flags);

	frame = NewtMakeFrame2(sizeof(ptv) / (sizeof(newtRefVar) * 2), ptv);

	NcSetSlot(frame, NSSYM(flags), NewtMakeInt32(flags));
	NcSetSlot(frame, NSSYM(type), NewtMakeBinary(kNewtRefUnbind, 
				(uint8_t*)&pkg->part_header->type, 4, true));

	switch (flags&0x03) {
	case kNOSPart:
		NcSetSlot(frame, NSSYM(info), PkgReadVardataBinary(pkg, &pkg->part_header->info));
		NcSetSlot(frame, NSSYM(data), PkgReadNOSPart(pkg));
		break;
	case kProtocolPart:
	case kRawPart:
	default:
#		ifdef DEBUG_PKG
			printf("*** PkgReader: PkgReadPart - unsupported part 0x%08x\n", flags);
#		endif
		break;
	}
	return frame;
}

/*------------------------------------------------------------------------*/
/** Read binary data from the Variable Data area
 * 
 * @param pkg		[inout] the package
 * @param info_ref	[in] pointer to the reference
 *
 * @retval	Binary object
 */
newtRef PkgReadVardataBinary(pkg_stream_t *pkg, pkg_info_ref_t *info_ref)
{
	if (info_ref->size==0) {
		return kNewtRefNIL;
	} else {
		uint8_t *src = pkg->var_data + ntohs(info_ref->offset);
		int size = ntohs(info_ref->size);
		return NewtMakeBinary(kNewtRefUnbind, src, size, true);
	}
}

/*------------------------------------------------------------------------*/
/** Read a UTF16 string from the Variable Data area
 * 
 * @param pkg		[inout] the package
 * @param info_ref	[in] pointer to the reference
 *
 * @retval	String object
 */
newtRef PkgReadVardataString(pkg_stream_t *pkg, pkg_info_ref_t *info_ref)
{
	if (info_ref->size==0) {
		return kNewtRefNIL;
	} else {
		char *src = pkg->var_data + ntohs(info_ref->offset);
		int size = ntohs(info_ref->size);
#		ifdef HAVE_LIBICONV
			size_t buflen;
			char *buf = NewtIconv(pkg->from_utf16, src, size, &buflen);
			if (buf) {
				return NewtMakeString2(buf, buflen-1, false);
			}
#		endif /* HAVE_LIBICONV */
		return NewtMakeString2(src, size, false);
	}
}

/*------------------------------------------------------------------------*/
/** Read the package header and create a Frame object describing it.
 * 
 * @param pkg		[inout] the package
 *
 * @retval	Frame describing the package
 */
newtRef PkgReadHeader(pkg_stream_t *pkg)
{
    newtRefVar	fnv[] = {
                            NS_CLASS,				NSSYM(PackageHeader),
                            NSSYM(version),			kNewtRefNIL,
                            NSSYM(copyright),		kNewtRefNIL,
                            NSSYM(name),			kNewtRefNIL,
                            NSSYM(flags),			kNewtRefNIL,
                            NSSYM(parts),			kNewtRefNIL
                        };
	newtRefVar	frame = NewtMakeFrame2(sizeof(fnv) / (sizeof(newtRefVar) * 2), fnv);
	newtRefVar	parts;

	NcSetSlot(frame, NSSYM(flags), NewtMakeInt32(ntohl(pkg->header->flags)));
    NcSetSlot(frame, NSSYM(version), NewtMakeInt32(ntohl(pkg->header->version)));
	NcSetSlot(frame, NSSYM(copyright), PkgReadVardataString(pkg, &pkg->header->copyright));
	NcSetSlot(frame, NSSYM(name), PkgReadVardataString(pkg, &pkg->header->name));
	// all other header values can be derived form the archive itself
	// or can be calculated at creation time

	parts = NewtMakeArray(kNewtRefNIL, pkg->num_parts);

	if (NewtRefIsNotNIL(parts))
	{
		int32_t	i, n = pkg->num_parts;

		NcSetSlot(frame, NSSYM(parts), parts);
		for (i = 0; i < n; i++)
		{
			NewtSetArraySlot(parts, i, PkgReadPart(pkg, i));
			if (pkg->lastErr != kNErrNone) break;
		}
	}

	return frame;
}

/*------------------------------------------------------------------------*/
/** Read a Package and create an array of parts
 *
 * @param data		[in] Package data memory, can be read-only
 * @param size		[in] size of memory array
 *
 * @retval	Newt array containing all parts of the Package
 * 
 * @todo	There is no support for relocation data yet.
 */
newtRef NewtReadPkg(uint8_t * data, size_t size)
{
	pkg_stream_t	pkg;
	newtRef			result;

	if (size<sizeof(pkg_header_t))
		return kNewtRefNIL;

	if (!PkgIsPackage(data))
		return kNewtRefNIL;

	memset(&pkg, 0, sizeof(pkg));
	pkg.verno = data[7]-'0';
	pkg.data = data;
	pkg.size = size;
	pkg.header = (pkg_header_t*)data;
	pkg.num_parts = ntohl(pkg.header->numParts);
	pkg.part_headers = (pkg_part_t*)(data + sizeof(pkg_header_t));
	pkg.var_data = data + sizeof(pkg_header_t) + pkg.num_parts*sizeof(pkg_part_t);
#	ifdef HAVE_LIBICONV
	{	char *encoding = NewtDefaultEncoding();
		pkg.from_utf16 = iconv_open(encoding, "UTF-16BE");
	}
#	endif /* HAVE_LIBICONV */

	result = PkgReadHeader(&pkg);

#	ifdef HAVE_LIBICONV
		iconv_close(pkg.from_utf16);
#	endif /* HAVE_LIBICONV */

	return result;
}




