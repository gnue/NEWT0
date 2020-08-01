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
#include <inttypes.h>

#include "NewtPkg.h"
#include "NewtErrs.h"
#include "NewtObj.h"
#include "NewtEnv.h"
#include "NewtFns.h"
#include "NewtVM.h"
#include "NewtIconv.h"

#include "utils/endian_utils.h"

#include <time.h>

#undef DEBUG_PKG_DIS
#define DEBUG_PKG
#ifdef DEBUG_PKG
#	include "NewtPrint.h"
#endif


/* macros */

/// Test first 8 bytes for any of the package signatures
#define PkgIsPackage(data) ((strncmp((const char*) data, "package0", 8)==0) || (strncmp((const char*) data, "package1", 8)==0))

#define kRelocationFlag 0x04000000

/* types */

/// info refs are a shorthand version of pointers into a userdata area of the package
typedef struct {
	uint16_t	offset;			///< offset from the flexible size header block
	uint16_t	size;			///< size of data block
} pkg_info_ref_t;

/// relocation data
typedef struct {
	uint16_t        size;
	uint8_t*        data;
} pkg_relocation_t;

typedef struct {
	uint32_t	reserved;
	uint32_t	relocation_size;
	uint32_t	page_size;
	uint32_t	num_entries;
	uint32_t	base_address;
} relocation_header_t;

typedef struct {
	uint16_t   	page_number;
	uint16_t   	offset_count;
} relocation_set_t;

/// package header structure
typedef struct {
	uint8_t		sig[8];			///< 8 byte signature
	uint32_t	type;			///< 4 bytes describing package class
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
	uint8_t		pkg_version;	///< rw corrsponds to the last charof the signature
	uint8_t *	data;			///< rw package data
	size_t		size;			///< rw size of package
	size_t		data_size;		///< w  size of data block
	pkg_header_t *header;		///< r  pointer to the package header
	uint32_t	header_size;	///< w  size of header structure w/o var data
	uint8_t *	var_data;		///< r  extra data for header
	uint32_t	var_data_size;	///< w  size of variable length data block
	uint32_t	directory_size;	///< w  size of all headers plus var data
	uint32_t	num_parts;		///< r  number of parts in package
	pkg_part_t*	part_headers;	///< r  pointer to first part header - can be used as an array
	pkg_part_t*	part_header;	///< r  header of current part
	uint8_t *	part;			///< r  start of current part data
	uint32_t	part_offset;	///< r  offset of current part to the beginning of data
	uint32_t	part_header_offset;	///< r  offset of current part header
	newtRefVar	instances;		///< rw array holding the previously generated instance of any ref per part
	newtRefVar	precedents;		///< w  array referencing the instances array
	newtErr		lastErr;		///< r  a way to return error from deep below
#ifdef HAVE_LIBICONV
	iconv_t		from_utf16;		///< r  strings in compatible packages are UTF16
	iconv_t		to_utf16;		///< w  strings in compatible packages are UTF16
#endif /* HAVE_LIBICONV */
	size_t		code_offset;	///< rw native code block offset
	pkg_relocation_t relocations;
	uint32_t	relocation_size;
} pkg_stream_t;

// In packages, references are limited to 32 bits.
typedef uint32_t pkgNewtRef;

/* functions */

static ssize_t	PkgArraySearch(newtRefArg array, newtRefArg r);
static size_t	PkgAlign(pkg_stream_t *pkg, size_t offset);
static intptr_t	PkgGetSlotInt(newtRefArg frame, newtRefArg name, intptr_t def);

static pkgNewtRef	PkgPartGetPrecedent(pkg_stream_t *pkg, newtRefArg ref);
static void		PkgPartSetPrecedent(pkg_stream_t *pkg, newtRefArg ref, pkgNewtRef val);

static void		PkgMakeRoom(pkg_stream_t *pkg, size_t offset, size_t size);
static void		PkgWriteData(pkg_stream_t *pkg, size_t offset, void *data, size_t size);
static void		PkgWriteU32(pkg_stream_t *pkg, size_t offset, uint32_t v);
static void		PgkWriteVarData(pkg_stream_t *pkg, size_t offset, newtRefVar frame, newtRefVar sym);
static pkgNewtRef	PkgWriteFrame(pkg_stream_t *pkg, newtRefArg frame);
static pkgNewtRef	PkgWriteArray(pkg_stream_t *pkg, newtRefArg array);
static pkgNewtRef	PkgWriteBinary(pkg_stream_t *pkg, newtRefArg obj);
static pkgNewtRef	PkgWriteObject(pkg_stream_t *pkg, newtRefArg obj);
static void		PkgWritePart(pkg_stream_t *pkg, newtRefArg part);

static uint32_t	PkgReadU32(uint8_t *d) ;
static newtRef	PkgPartGetInstance(pkg_stream_t *pkg, uint32_t p_obj);
static void		PkgPartSetInstance(pkg_stream_t *pkg, uint32_t p_obj, newtRefArg r);
static newtRef	PkgReadRef(pkg_stream_t *pkg, uint32_t p_obj);
static newtRef	PkgReadBinaryObject(pkg_stream_t *pkg, uint32_t p_obj);
static newtRef	PkgReadArrayObject(pkg_stream_t *pkg, uint32_t p_obj);
static newtRef	PkgReadFrameObject(pkg_stream_t *pkg, uint32_t p_obj);
static newtRef	PkgReadObject(pkg_stream_t *pkg, uint32_t p_obj);
static newtRef	PkgReadNOSPart(pkg_stream_t *pkg);
static newtRef	PkgReadPart(pkg_stream_t *pkg, int32_t index);
static newtRef	PkgReadVardataBinary(pkg_stream_t *pkg, pkg_info_ref_t *info_ref);
static newtRef	PkgReadVardataString(pkg_stream_t *pkg, pkg_info_ref_t *info_ref);
static newtRef	PkgReadHeader(pkg_stream_t *pkg);


/*------------------------------------------------------------------------*/
/** Duplicate of same function in NSOF.
 * Maybe we should make the original file publicly accessible
 *
 * @see NewtSearchArray(newtRefArg array, newtRefArg r)
 */
ssize_t PkgArraySearch(newtRefArg array, newtRefArg r)
{
    newtRef *	slots;
	size_t		len;
	size_t		i;

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
pkgNewtRef PkgPartGetPrecedent(pkg_stream_t *pkg, newtRefArg ref)
{
	ssize_t ix = PkgArraySearch(pkg->precedents, ref);
	if (ix>=0) {
		return (pkgNewtRef) NewtGetArraySlot(pkg->instances, ix);
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
void PkgPartSetPrecedent(pkg_stream_t *pkg, newtRefArg ref, pkgNewtRef val)
{
	size_t n = NewtArrayLength(pkg->instances);
	// the code below gets horribly slow and fragments memory
	// we should consider implementing a binary search tree at some point
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
size_t PkgAlign(pkg_stream_t *pkg, size_t offset)
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
intptr_t PkgGetSlotInt(newtRefArg frame, newtRefArg name, intptr_t def)
{
	newtRef slot;
	ssize_t ix;
	
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
void PkgMakeRoom(pkg_stream_t *pkg, size_t offset, size_t size)
{
	size_t new_size = offset+size;

	if (pkg->data_size<new_size) {
		size_t os = pkg->data_size;
		size_t ns = (new_size+16383) & (~16383); // alocate in 16k blocks
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
void PkgWriteData(pkg_stream_t *pkg, size_t offset, void *data, size_t size)
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
void PkgWriteU32(pkg_stream_t *pkg, size_t offset, uint32_t v)
{
	v = htonl(v);
	PkgWriteData(pkg, offset, &v, 4);
}

/*------------------------------------------------------------------------*/
/** Write the data in the object into the vardata section and generate the info ref
 * 
 * @param pkg		[inout] the package
 * @param offset	[in] where to write the info ref
 * @param frame		[in] frame containing the data 
 * @param sym		[in] symbol of slot containing the data
 */
void PgkWriteVarData(pkg_stream_t *pkg, size_t offset, newtRefVar frame, newtRefVar sym)
{
	newtRef info;
	ssize_t ix;
	
	PkgWriteU32(pkg, offset, 0);

	ix = NewtFindSlotIndex(frame, sym);
	if (ix<0) 
		return;
	info = NewtGetFrameSlot(frame, ix);
	if (NewtRefIsBinary(info)) {
		size_t size = NewtBinaryLength(info);
		uint8_t *data = NewtRefToBinary(info);
		pkg_info_ref_t info_ref;

#		ifdef HAVE_LIBICONV
			if (NewtRefIsString(info)) {
				size_t buflen;
				char *buf = NewtIconv(pkg->to_utf16, (const char*) data, size, &buflen);
				if (buf) {
					size = (uint32_t) buflen;
					data = (uint8_t*) buf;
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
 * @retval	reference to the object (offset to the beginning of the object in the package file + 1)
 */
pkgNewtRef PkgWriteFrame(pkg_stream_t *pkg, newtRefArg frame)
{
	size_t dst, size, i, n;
	newtRef map, slot;
	newtRef map_pos, slot_pos;

	// calculate the size of this chunk
	dst = PkgAlign(pkg, pkg->size);
	n = NewtFrameLength(frame);
	size = (n+3)*4;

	// make room for the entire chunk and write the header
	PkgMakeRoom(pkg, dst, size);
	PkgWriteU32(pkg, dst, (uint32_t) ((size<<8) | 0x40 | kObjSlotted | kObjFrame)); // Possible overflow
	PkgWriteU32(pkg, dst+4, 0);

	// recursively add all arrays making up the map
	map = NewtFrameMap(frame);
	map_pos = PkgWriteObject(pkg, map);
	PkgWriteU32(pkg, dst+8, (uint32_t) map_pos);

	// now add all slots and fill in the rest of our chunk
	for (i=0; i<n; i++) {
		slot = NewtGetFrameSlot(frame, i);
		slot_pos = PkgWriteObject(pkg, slot);
		PkgWriteU32(pkg, dst+12+4*i, (uint32_t) slot_pos);
	}

	return (pkgNewtRef) NewtMakePointer(dst);
}

/*------------------------------------------------------------------------*/
/** Append an array object to the end of the Package
 * 
 * @param pkg		[inout] the package
 * @param array		[in] the array that we will write
 *
 * @retval    reference to the object (offset to the beginning of the object in the package file + 1)
 */
pkgNewtRef PkgWriteArray(pkg_stream_t *pkg, newtRefArg array)
{
	size_t dst, size, i, n;
	newtRef klass, klass_pos;
	newtRef slot, slot_pos;

	// calculate the size of this chunk
	dst = PkgAlign(pkg, pkg->size);
	n = NewtArrayLength(array);
	size = (n+3)*4;

	// make room for the entire chunk and write the header
	PkgMakeRoom(pkg, dst, size);
	PkgWriteU32(pkg, dst, (uint32_t) ((size<<8) | 0x40 | kObjSlotted)); // Possible overflow
	PkgWriteU32(pkg, dst+4, 0);

	// add the class information
	klass = NcClassOf(array);
	klass_pos = PkgWriteObject(pkg, klass);
	PkgWriteU32(pkg, dst+8, (uint32_t) klass_pos);

	// now add all slots and fill in the rest of our chunk
	for (i=0; i<n; i++) {
		slot = NewtGetArraySlot(array, i);
		slot_pos = PkgWriteObject(pkg, slot);
		PkgWriteU32(pkg, dst+12+4*i, (uint32_t) slot_pos);
	}

	return (pkgNewtRef) NewtMakePointer(dst);
}

/*------------------------------------------------------------------------*/
/** Append a binary object to the end of the Package
 * 
 * @param pkg		[inout] the package
 * @param obj		[in] the binary object that we will write
 *
 * @retval    reference to the object (offset to the beginning of the object in the package file + 1)
 */
pkgNewtRef PkgWriteBinary(pkg_stream_t *pkg, newtRefArg obj)
{
	size_t dst, size;
	uint8_t *data;
	newtRef klass, klass_ref = kNewtRefUnbind;

	// calculate the size of this chunk
	dst = PkgAlign(pkg, pkg->size);
	size = (uint32_t) NewtBinaryLength(obj);
	data = NewtRefToBinary(obj);

	// make room for the binary chunk and write the header
	if (NewtRefIsSymbol(obj)) {
		size = (uint32_t) NewtSymbolLength(obj)+5; // remember the trailing zero!
	} else if (NewtRefIsString(obj)) {
#		ifdef HAVE_LIBICONV
			size_t buflen;
			char *buf = NewtIconv(pkg->to_utf16, (const char*) data, size, &buflen);
			if (buf) {
				size = (uint32_t) buflen;
				data = (uint8_t*) buf;
			}
#		endif /* HAVE_LIBICONV */
	}
	size += 12;

	// make room for the binary chunk and write the header
	PkgMakeRoom(pkg, dst, size);
	PkgWriteU32(pkg, dst, (uint32_t) ((size<<8) | 0x40));   // Possible overflow
	PkgWriteU32(pkg, dst+4, 0);

	// symbols have special handling to avoid recursion
	if (NewtRefIsSymbol(obj)) {
		PkgWriteU32(pkg, dst+8, kNewtSymbolClass);
		PkgWriteU32(pkg, dst+12, NewtRefToHash(obj)); // make sure the hash has the right endianness
		PkgWriteData(pkg, dst+16, (uint8_t*)NewtSymbolGetName(obj), size-16);
		return (pkgNewtRef) NewtMakePointer(dst);
	}

	// add the class information
	klass = NcClassOf(obj);
	klass_ref = PkgWriteObject(pkg, klass);
	PkgWriteU32(pkg, dst+8, (uint32_t) klass_ref);

	// copy the binary data over
	if (klass==NSSYM0(int32)) {
		uint32_t *s = (uint32_t*)data;
		uint32_t  v = htonl(*s);
		PkgWriteData(pkg, dst+12, &v, sizeof(v));
	} else if (klass==NSSYM0(real)) {
		// this code fails miserably if 'double' is not an 8-byte IEEE value!
		double *s = (double*)data;
		double  v = htond(*s);
		PkgWriteData(pkg, dst+12, &v, sizeof(v));
	} else {
		PkgWriteData(pkg, dst+12, data, size-12);
	}
	if (klass==NSSYM0(code)) {
		pkg->code_offset = dst+12;
		printf("Added native code at %d, size %d\n", dst+12, size-12);
	}

	return (pkgNewtRef) NewtMakePointer(dst);
}

/*------------------------------------------------------------------------*/
/** Append the object to the end of the Package
 * 
 * @param pkg		[inout] the package
 * @param obj		[in] the object that we will write
 *
 * @retval    reference to the object (offset to the beginning of the object in the package file + 1)
 */
pkgNewtRef PkgWriteObject(pkg_stream_t *pkg, newtRefArg obj)
{
	pkgNewtRef prec;
	pkgNewtRef result;

	// FIXME add handling named magic pointers here
	if (NewtRefIsImmediate(obj)) {
		// immediates have the same form in memory as in packages
		// immediates include magic pointers
		return (uint32_t) obj;
	} 
	
	prec = PkgPartGetPrecedent(pkg, obj);
	if (prec!=kNewtRefUnbind) {
		return prec;
	}

	if (NewtRefIsFrame(obj)) {
		result = PkgWriteFrame(pkg, obj);
	} else if (NewtRefIsArray(obj)) {
		result = PkgWriteArray(pkg, obj);
	} else if (NewtRefIsBinary(obj)) {
		result = PkgWriteBinary(pkg, obj);
	} else {
#		ifdef DEBUG_PKG
			// we do not know how to write this object
			printf("*** unsupported write: ");
			NewtPrintObject(stdout, obj);
#		endif
		return kNewtRefNIL; // do not create a precedent
	}

	// make this ref available for later incarnations of the same object
	PkgPartSetPrecedent(pkg, obj, result);

	return result;
}

/*------------------------------------------------------------------------*/
/** Create a part in package format based on this object.
 * 
 * This function makes heavy use of the "pkg" structure, updating all
 * members to allow writing multiple consecutive parts by repeatedly
 * caling this function. Multiple part packages are untested.
 *
 * @param pkg		[inout] the package
 * @param part		[in] object containing part data
 *
 * @retval	ref to binary part data
 */
void PkgWritePart(pkg_stream_t *pkg, newtRefArg part)
{
	uint32_t	dst = pkg->part_offset;
	uint32_t	hdr = pkg->part_header_offset;
	ssize_t		ix;
	newtRef		data;
	size_t		part_size;

	ix = NewtFindSlotIndex(part, NSSYM(data));
	if (ix<0) 
		return;
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

	PkgMakeRoom(pkg, PkgAlign(pkg, pkg->size), 0);
	part_size = pkg->size - pkg->part_offset;
		// offset
	PkgWriteU32(pkg, hdr, pkg->part_offset - pkg->directory_size - pkg->relocation_size);
		// size
	PkgWriteU32(pkg, hdr+4, (uint32_t) part_size);
		// size2
	PkgWriteU32(pkg, hdr+8, (uint32_t) part_size);
		// type
	PkgWriteU32(pkg, hdr+12, (uint32_t) PkgGetSlotInt(part, NSSYM(type), 0x666f726d)); // "form"
		// reserved1
	PkgWriteU32(pkg, hdr+16, 0); 
		// flags
	PkgWriteU32(pkg, hdr+20, (uint32_t) PkgGetSlotInt(part, NSSYM(flags), 0));
		// reserved2
	PkgWriteU32(pkg, hdr+28, 0); 

	pkg->part_offset += part_size;
}


/*------------------------------------------------------------------------*/
/** Reserve space for relocation information
 * 
 * Add space for the relocation data after the variable data. The relocation
 * information will be updated once the location of the code block is known
 * (currently, only one code block is supported). The relocation data is assumed
 * to be sorted, allowing to estimate the number of pages which need relocation.
 *
 * @param pkg		[inout] the package
 */
void PkgReserveRelocations(newtRefArg package, pkg_stream_t *pkg)
{
	ssize_t ix = NewtFindSlotIndex(package, NSSYM(relocations));
	if (ix >= 0) {
		newtRef relocation_binary = NewtGetFrameSlot(package, ix);
		uint32_t *relocations = NewtRefToBinary(relocation_binary);
		size_t num_relocations = htonl(relocations[0]);
		uint32_t num_pages = 2 + ((htonl(relocations[num_relocations]) - htonl(relocations[1])) / 1024);
		pkg->relocation_size = PkgAlign(&pkg, sizeof(relocation_header_t) + num_pages * (sizeof(relocation_set_t) + 4)
			+ num_relocations);
		PkgMakeRoom(&pkg, pkg->directory_size, pkg->relocation_size);
	} else {
		pkg->relocation_size = 0;
	}
}

void PkgUpdateRelocations(newtRefArg package, pkg_stream_t *pkg)
{
	ssize_t ix = NewtFindSlotIndex(package, NSSYM(relocations));
	if (ix >= 0) {
		newtRef relocation_binary = NewtGetFrameSlot(package, ix);
		uint32_t *relocations = NewtRefToBinary(relocation_binary);
		size_t num_relocations = htonl(relocations[0]);
		uint32_t num_pages;
		relocation_header_t header;
		relocation_set_t reloc_set;
		uint8_t reloc_data[256];
		size_t num_reloc_data;
		uint32_t page_number;
		uint32_t reloc_sets_size;

		num_reloc_data = 0;
		reloc_sets_size = 0;
		num_pages = 0;
		memset (reloc_data, 0, sizeof(reloc_data));
		page_number = (htonl(relocations[1]) + pkg->code_offset) / 1024;
		for (size_t i = 0; i < num_relocations; i++) {
			uint32_t pkg_reloc = htonl(relocations[i + 1]) + pkg->code_offset;
			if (pkg_reloc / 1024 != page_number) {
				printf ("New page: %04x %04x %04x %d\n", page_number, pkg_reloc / 1024, pkg_reloc, reloc_sets_size);
				reloc_set.offset_count = htons(num_reloc_data);
				reloc_set.page_number = htons(page_number);
				PkgWriteData(pkg, pkg->header_size + pkg->var_data_size + sizeof(header) + reloc_sets_size,
						&reloc_set, sizeof(reloc_set));
				PkgWriteData(pkg, pkg->header_size + pkg->var_data_size + sizeof(header) + reloc_sets_size + sizeof(reloc_set),
						reloc_data, num_reloc_data);
				reloc_sets_size += sizeof(reloc_set) + num_reloc_data;
				reloc_sets_size = (reloc_sets_size + 3) & ~3;
				num_reloc_data = 0;
				memset (reloc_data, 0, sizeof(reloc_data));
				page_number = pkg_reloc / 1024;
				num_pages++;
			}
			reloc_data[num_reloc_data++] = (pkg_reloc - page_number * 1024) / 4;
		}
		if (num_reloc_data > 0) {
			printf ("Finishing page: %04x, offset %d count %d\n", page_number, reloc_sets_size, num_reloc_data);
			reloc_set.offset_count = htons(num_reloc_data);
			reloc_set.page_number = htons(page_number);
			PkgWriteData(pkg, pkg->header_size + pkg->var_data_size + sizeof(header) + reloc_sets_size,
					&reloc_set, sizeof(reloc_set));
			PkgWriteData(pkg, pkg->header_size + pkg->var_data_size + sizeof(header) + reloc_sets_size + sizeof(reloc_set),
					reloc_data, num_reloc_data);
			num_pages++;
		}
		header.reserved = 0;
		header.relocation_size = htonl(pkg->relocation_size);
		header.page_size = htonl(0x400);
		header.num_entries = htonl(num_pages);
		header.base_address = htonl(65536 - pkg->code_offset);
		PkgWriteData(pkg, pkg->header_size + pkg->var_data_size,
				&header, sizeof(header));

	}
}
/*------------------------------------------------------------------------*/
/** Create a new binary object that contains the object tree in package format.
 *
 * This function creates a binary object, containing the representaion of
 * a whole hierarchy of objects in the Newton package format. The binary
 * data can be written directly to disk to create a Newton readable .pkg file.
 * 
 * NewtWritePkg was tested on hierarchies created by NewtReadPackage, reading
 * a random bunch of .pkg files containing Newton Script applications. The 
 * packages created were identiacla to the original packages.
 *
 * @todo	NewtWritePkg does not support a relocation table yet which may 
 *			be needed to save native function of a higher complexity.
 * @todo	Error handling is not yet implemented.
 * @todo	Named magic poiners are not supported yet.
 * @todo	Only NOS parts are currently supported. We still must implement
 *			Protocol parts and Raw parts. 
 *
 * @param rpkg	[in] object hierarchy describing the package
 *
 * @retval	binary object with package
 */
newtRef NewtWritePkg(newtRefArg package)
{
	pkg_stream_t	pkg;
    size_t			num_parts, i;
	size_t			relocation_size;
    ssize_t			ix;
	newtRef			parts, result;

	// setup pkg_stream_t
	memset(&pkg, 0, sizeof(pkg));

#	ifdef HAVE_LIBICONV
	{	char *encoding = NewtDefaultEncoding();
		pkg.to_utf16 = iconv_open("UTF-16BE", encoding);
	}
#	endif /* HAVE_LIBICONV */

	// find the array of parts that we will write
	ix = NewtFindSlotIndex(package, NSSYM(parts));
	if (ix>=0) {
		parts = NewtGetFrameSlot(package, ix);
		num_parts = NewtFrameLength(parts);
		pkg.header_size = (uint32_t) (sizeof(pkg_header_t) + num_parts * sizeof(pkg_part_t));   // Abnormal number of parts is unlikely

		// start setting up the header with whatever we know
			// sig
		PkgWriteData(&pkg, 0, "package0", 8);
		pkg.data[7] = (uint8_t)('0' + PkgGetSlotInt(package, NSSYM(pkg_version), 0));
			// type
		PkgWriteU32(&pkg, 8, (uint32_t) PkgGetSlotInt(package, NSSYM(type), 0x78787878)); // "xxxx"
			// flags
		PkgWriteU32(&pkg, 12, (uint32_t) PkgGetSlotInt(package, NSSYM(flags), 0));
			// version
		PkgWriteU32(&pkg, 16, (uint32_t) PkgGetSlotInt(package, NSSYM(version), 0));
			// copyright
		PgkWriteVarData(&pkg, 20, package, NSSYM(copyright));
			// name
		PgkWriteVarData(&pkg, 24, package, NSSYM(name));
			// date
		PkgWriteU32(&pkg, 32, (uint32_t) (time(0L)+2082844800));    // Possible overflow
			// reserved2
		PkgWriteU32(&pkg, 36, 0); 
			// reserved3
		PkgWriteU32(&pkg, 40, 0); 
			// numParts
		PkgWriteU32(&pkg, 48, (uint32_t) (num_parts));

		// calculate the size of the header so we can correctly set our refs in the parts
		for (i=0; i<num_parts; i++) {
			newtRef part = NewtGetArraySlot(parts, i);
			PgkWriteVarData(&pkg, (uint32_t) (sizeof(pkg_header_t) + i*sizeof(pkg_part_t) + 24), part, NSSYM(info));
		}

		// the original file has this (c) message embedded
		{	
#ifdef _MSC_VER
			char msg[] = "Newtonª ToolKit Package © 1992-1997, Apple Computer, Inc.";
#else
			char msg[] = "Newtonï½ª ToolKit Package ï½© 1992-1997, Apple Computer, Inc.";
#endif
			PkgWriteData(&pkg, pkg.header_size + pkg.var_data_size, msg, sizeof(msg));
			pkg.var_data_size += sizeof(msg);
		}

		pkg.directory_size = (uint32_t) PkgAlign(&pkg, pkg.header_size + pkg.var_data_size);
			// directorySize
		PkgWriteU32(&pkg, 44, pkg.directory_size);
		PkgReserveRelocations(package, &pkg);

		pkg.part_offset = PkgAlign(&pkg, pkg.directory_size + pkg.relocation_size);
		// create all parts
		for (i=0; i<num_parts; i++) {
			newtRef part = NewtGetArraySlot(parts, i);
			pkg.part_header_offset = (uint32_t) (sizeof(pkg_header_t) + i*sizeof(pkg_part_t));
			PkgWritePart(&pkg, part);
		}
		PkgUpdateRelocations(package, &pkg);
	}

	// finish filling in the header
		// size
	PkgWriteU32(&pkg, 28, (uint32_t) pkg.size);

	result = NewtMakeBinary(NSSYM(package), pkg.data, pkg.size, false);

	// clean up our allocations
	if (pkg.data) 
		free(pkg.data);

#	ifdef HAVE_LIBICONV
		iconv_close(pkg.to_utf16);
#	endif /* HAVE_LIBICONV */

	return result;
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
		// special refs are immedites, encoded in the same format in
		// memory as in package files (at least for all instances I could find)
		result = ref; 
		break;
	case 3: // magic pointer
		// FIXME we must implement special code for name magic pointers here!
		result = ref; // already a correct magic pointer
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

	klass = PkgReadRef(pkg, p_obj+8);

	if (klass==kNewtSymbolClass) {
		result = NewtMakeSymbol((const char*) pkg->data + p_obj + 16);
	} else if (klass==NSSYM0(string)) {
		const char *src = (const char*) pkg->data + p_obj + 12;
		int sze = size-12;
#		ifdef HAVE_LIBICONV
			size_t buflen;
			char *buf = NewtIconv(pkg->from_utf16, src, sze, &buflen);
			if (buf) {
				result = NewtMakeString2(buf, buflen-1, true); // NewtMakeString2 appends another null
			}
#		endif /* HAVE_LIBICONV */
		if (result==kNewtRefNIL)
			result = NewtMakeString2(src, sze, true);
	} else if (klass==NSSYM0(int32)) {
		uint32_t v = PkgReadU32(pkg->data + p_obj + 12);
		result = NewtMakeInt32(v);
	} else if (klass==NSSYM0(real)) {
		double *v = (double*)(pkg->data + p_obj + 12);
		result = NewtMakeReal(ntohd(*v));
	} else if (klass==NSSYM0(instructions)) {
		result = NewtMakeBinary(klass, pkg->data + p_obj + 12, size-12, true);
#		ifdef DEBUG_PKG_DIS
			printf("*** PkgReader: PkgReadBinaryObject - dumping byte code\n");
			NVMDumpBC(stdout, result);
#		endif
	} else if (klass==NSSYM0(bits)) {
		result = NewtMakeBinary(klass, pkg->data + p_obj + 12, size-12, true);
	} else if (klass==NSSYM0(cbits)) {
		result = NewtMakeBinary(klass, pkg->data + p_obj + 12, size-12, true);
	} else if (klass==NSSYM0(nativeModule)) {
		result = NewtMakeBinary(klass, pkg->data + p_obj + 12, size-12, true);
	} else {
#		ifdef DEBUG_PKG
			// This output is helpful to find more binary classes that may need 
			// endianness fixes like the floating point class
			if (klass) {
				printf("*** PkgReader: PkgReadBinaryObject - unknown class ");
				NewtPrintObject(stdout, klass);
			}
#		endif
		result = NewtMakeBinary(klass, pkg->data + p_obj + 12, size-12, true);
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

	// verify that we have a correct lead-in 
	if (PkgReadU32(pkg->part)!=0x00001041 || PkgReadU32(pkg->part+8)!=0x00000002) {
#		ifdef DEBUG_PKG
		printf("*** PkgReader: PkgReadPart - unsupported NOS Part intro at %" PRIdPTR "\n",
			pkg->part-pkg->data);
#		endif
		return kNewtRefNIL;
	}
	
	// create an array that holds a ref to all created objects, avoiding double instantiation
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
	pkg->part_offset = ntohl(pkg->header->directorySize) + pkg->relocations.size + ntohl(pkg->part_header->offset);
	pkg->part = pkg->data + pkg->part_offset;
	flags = ntohl(pkg->part_header->flags);

	frame = NewtMakeFrame2(sizeof(ptv) / (sizeof(newtRefVar) * 2), ptv);

	NcSetSlot(frame, NSSYM(flags), NewtMakeInt32(flags));
	NcSetSlot(frame, NSSYM(type), NewtMakeInt32(ntohl(pkg->part_header->type)));

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
		char *src = (char*) pkg->var_data + ntohs(info_ref->offset);
		int size = ntohs(info_ref->size);
#		ifdef HAVE_LIBICONV
			size_t buflen;
			char *buf = NewtIconv(pkg->from_utf16, src, size, &buflen);
			if (buf) {
				return NewtMakeString2(buf, buflen-1, true);
			}
#		endif /* HAVE_LIBICONV */
		return NewtMakeString2(src, size, true);
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
                            NSSYM(type),			kNewtRefNIL,
                            NSSYM(pkg_version),		kNewtRefNIL,
                            NSSYM(version),			kNewtRefNIL,
                            NSSYM(copyright),		kNewtRefNIL,
                            NSSYM(name),			kNewtRefNIL,
                            NSSYM(flags),			kNewtRefNIL,
                            NSSYM(parts),			kNewtRefNIL
                        };
	newtRefVar	frame = NewtMakeFrame2(sizeof(fnv) / (sizeof(newtRefVar) * 2), fnv);
	newtRefVar	parts;

    NcSetSlot(frame, NSSYM(type), NewtMakeInt32(ntohl(pkg->header->type)));
    NcSetSlot(frame, NSSYM(pkg_version), NewtMakeInteger(pkg->pkg_version));
    NcSetSlot(frame, NSSYM(version), NewtMakeInt32(ntohl(pkg->header->version)));
	NcSetSlot(frame, NSSYM(copyright), PkgReadVardataString(pkg, &pkg->header->copyright));
	NcSetSlot(frame, NSSYM(name), PkgReadVardataString(pkg, &pkg->header->name));
	NcSetSlot(frame, NSSYM(flags), NewtMakeInt32(ntohl(pkg->header->flags)));

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
 * This function creates a hierarchy of Newt objects from a block of data
 * interpreted as Package data. Usually this data would be a verbatim
 * copy of a .pkg file.
 *
 * NewtReadPkg was tested on a random bunch of .pkg files containing Newton
 * Script applications. 
 *
 * The iconv library is required at compile-time to read Newton-compatible 
 * packages. 
 *
 * @todo	NewtReadPkg does not support a relocation table yet which may 
 *			be needed to load native function of a higher complexity.
 * @todo	Error handling is not yet implemented.
 * @todo	Named magic poiners are not supported yet.
 * @todo	Only NOS parts are currently supported. We still must implement
 *			Protocol parts and Raw parts. 
 * @todo	A function should be added that creates a working default 
 *			Package hierarchy.
 *
 * @param data		[in] Package data memory, can be read-only
 * @param size		[in] size of memory array
 *
 * @retval	Newt array containing some descriptions and all parts of the Package
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
	pkg.pkg_version = data[7]-'0';
	pkg.data = data;
	pkg.size = (uint32_t) size;
	pkg.header = (pkg_header_t*)data;
	pkg.num_parts = ntohl(pkg.header->numParts);
	if (pkg.header->flags & kRelocationFlag) {
		pkg.relocations.size = ntohl(*(uint32_t *) (data + ntohl(pkg.header->directorySize) + sizeof(uint32_t)));
	} else {
		pkg.relocations.size = 0;
	}
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


/*------------------------------------------------------------------------*/
/** Read a Package and create an array of parts
 *
 * @param rcvr	[in] receiver
 * @param r		[in] Package binary object
 *
 * @retval		Newt array containing some descriptions and all parts of the Package
 *
 * @note		for script call
 */

newtRef NsReadPkg(newtRefArg rcvr, newtRefArg r)
{
	size_t	len;

    if (! NewtRefIsBinary(r))
        return NewtThrow(kNErrNotABinaryObject, r);

	len = NewtBinaryLength(r);

	return NewtReadPkg(NewtRefToBinary(r), len);
}


/*------------------------------------------------------------------------*/
/** Create a new binary object that contains the object tree in package format.
 *
 * @param rcvr	[in] receiver
 * @param r		[in] object hierarchy describing the package
 *
 * @retval		binary object with package
 *
 * @note		for script call
 */

newtRef NsMakePkg(newtRefArg rcvr, newtRefArg r)
{
	return NewtWritePkg(r);
}
