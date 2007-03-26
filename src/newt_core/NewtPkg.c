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
	int8_t		verno;			///< package version 0 or 1
	uint8_t *	data;			///< package data
	uint32_t	size;			///< size of package
	pkg_header_t * header;		///< pointer to the package header
	uint8_t *	var_len_data;	///< extra data for header
	uint32_t	num_parts;		///< number of parts in package
	pkg_part_t*	part_headers;	///< pointer to first part header - can be used as an array
	pkg_part_t*	part_header;	///< header of current part
	uint8_t *	part;			///< start of current part data
	newtErr		lastErr;		///< a way to return error from deep below
#ifdef HAVE_LIBICONV
	iconv_t		from_utf16;		///< strings in compatible packages are UTF16
#endif /* HAVE_LIBICONV */
} pkg_stream_t;


/* functions */
uint32_t PkgReadU32(uint8_t *d);

newtRef PkgReadRef(pkg_stream_t *pkg, uint32_t p_obj);
newtRef PkgReadBinaryObject(pkg_stream_t *pkg, uint32_t p_obj);
newtRef PkgReadArrayObject(pkg_stream_t *pkg, uint32_t p_obj);
newtRef PkgReadFrameObject(pkg_stream_t *pkg, uint32_t p_obj);
newtRef PkgReadObject(pkg_stream_t *pkg, uint32_t p_obj);

newtRef PkgReadPart(pkg_stream_t *pkg, int32_t index);

newtRef PkgReadVardataString(pkg_stream_t *pkg, pkg_info_ref_t *info_ref);
newtRef PkgReadVardataBinary(pkg_stream_t *pkg, pkg_info_ref_t *info_ref);
newtRef PkgReadHeader(pkg_stream_t *pkg);

newtRef NewtReadPkg(uint8_t * data, size_t size);

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
/** Generate a ref from package data
 * 
 * @param pkg		[inout] the package
 * @param p_obj		[in] offset to Ref data relative to package start
 *
 * @retval	Newt object version of package Ref
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
		else if (ref==0x32) {
			result = 0x32;
			printf("*** PkgReader: PkgReadRef - unsupported ref 0x%08x - what is this?\n", ref);
		} else {
#			ifdef DEBUG_PKG
				printf("*** PkgReader: PkgReadRef - unsupported ref 0x%08x\n", ref);
#			endif
			result = ref; 
		}
		break;
	case 3: // magic pointer
#		ifdef __NAMED_MAGIC_POINTER__
			result = kNewtRefNIL; // no timplemented
#		else
			result = ref; // already a correct magic pointer
#		endif
		break;
	}

	return result;
}

/*------------------------------------------------------------------------*/
/** Generate a binary object from package data
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
				result = NewtMakeString2(buf, buflen, false);
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
/** Generate an array from package data
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
/** Generate a frame from package data
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
	newtRef ret = kNewtRefNIL;

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
	uint32_t p_obj = PkgReadU32(pkg->part+12);
	return PkgReadObject(pkg, p_obj&~3);
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
	pkg->part = pkg->data 
				+ ntohl(pkg->header->directorySize) 
				+ ntohl(pkg->part_header->offset);
	flags = ntohl(pkg->part_header->flags);

	frame = NewtMakeFrame2(sizeof(ptv) / (sizeof(newtRefVar) * 2), ptv);

	NcSetSlot(frame, NSSYM(flags), NewtMakeInt32(flags));
	NcSetSlot(frame, NSSYM(type), NewtMakeBinary(kNewtRefUnbind, 
				(uint8_t*)&pkg->part_header->type, 4, true));

	switch (flags&0x03) {
	case kNOSPart:
		if (   PkgReadU32(pkg->part)  !=0x00001041
			|| PkgReadU32(pkg->part+8)!=0x00000002) 
		{
#			ifdef DEBUG_PKG
				printf("*** PkgReader: PkgReadPart - unsupported NOS Part intro at %d\n",
					pkg->part-pkg->data);
#			endif
			break;
		}
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
		uint8_t *src = pkg->var_len_data + ntohs(info_ref->offset);
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
		char *src = pkg->var_len_data + ntohs(info_ref->offset);
		int size = ntohs(info_ref->size);
#		ifdef HAVE_LIBICONV
			size_t buflen;
			char *buf = NewtIconv(pkg->from_utf16, src, size, &buflen);
			if (buf) {
				return NewtMakeString2(buf, buflen, false);
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

    NcSetSlot(frame, NSSYM(version), NSINT(pkg->verno));
	NcSetSlot(frame, NSSYM(copyright), PkgReadVardataString(pkg, &pkg->header->copyright));
	NcSetSlot(frame, NSSYM(name), PkgReadVardataString(pkg, &pkg->header->name));
	NcSetSlot(frame, NSSYM(flags), NewtMakeInt32(ntohl(pkg->header->flags)));
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
	pkg.var_len_data = data + sizeof(pkg_header_t) + pkg.num_parts*sizeof(pkg_part_t);
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




