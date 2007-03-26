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


/* ヘッダファイル  includes */
#include <string.h>

#include "NewtPkg.h"
#include "NewtErrs.h"
#include "NewtObj.h"
//#include "NewtEnv.h"
//#include "NewtFns.h"
//#include "NewtVM.h"
//#include "NewtIconv.h"

#include "utils/endian_utils.h"


/* マクロ  defines */

/// Test first 8 bytes for package signature
#define PkgIsPackage(data) ((strncmp(data, "package0", 8)==0) || (strncmp(data, "package0", 8)==0))


/* 型宣言  types */

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
	uint32_t	data;			///< creation data
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
	pkg_header_t *header;		///< pointer to the package header
	pkg_part_t	*part_headers;	///< pointer to first part header - can be used as an array
	newtErr		lastErr;		///< a way to return error from deep below
} pkg_stream_t;


/* 関数プロトタイプ  functions */
newtRef PkgReadPart(pkg_stream_t *pkg, int32_t index);
newtRef NewtReadPkg(uint8_t * data, size_t size);

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
	newtRefVar	r = kNewtRefNIL;
	// FIXME prepare pkg with offsets and pointer for reading this Part
	// FIXME store 8/4 byte alignment flag (no so important for reading though)
	// FIXME call PkgReadPart2 to recursively scan all Refs in the part
	return r;
}

/*------------------------------------------------------------------------*/
/** Read a Package and create an array of parts
 *
 * @param data		[in] Package data memory
 * @param size		[in] size of memory array
 *
 * @retval	Newt array containing all parts of the Package
 * 
 * @todo	There is no support for relocation data yet.
 */

newtRef NewtReadPkg(uint8_t * data, size_t size)
{
	pkg_stream_t	pkg;
	newtRefVar		result;

	if (size<sizeof(pkg_header_t))
		return 0; // FIXME return a meaningful error

	if (!PkgIsPackage(data))
		return 0; // FIXME return a meaningful error

	memset(&pkg, 0, sizeof(pkg));
	pkg.verno = data[7]-'0';
	pkg.data = data;
	pkg.size = size;
	pkg.header = (pkg_header_t*)data;

	result = NewtMakeArray(kNewtRefNIL, pkg.header->numParts+1);

	if (NewtRefIsNotNIL(result))
	{
		// FIXME the first slot of the array will contain information form the package
		// FIXME header arranged in a frame
		newtRef *slots;
		int32_t	i, n;

		slots = NewtRefToSlots(result);
		n = pkg.header->numParts;

		for (i = 0; i < n; i++)
		{
			slots[i+1] = PkgReadPart(&pkg, i);
			if (pkg.lastErr != kNErrNone) break;
		}
	}

	return result;
}




