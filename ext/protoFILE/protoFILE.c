//--------------------------------------------------------------------------
/**
 * @file  protoFILE.c
 * @brief 拡張ライブラリ
 *
 * @author M.Nukui
 * @date 2004-06-10
 *
 * Copyright (C) 2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <stdio.h>

#include "NewtLib.h"
#include "NewtObj.h"
#include "NewtIO.h"
#include "NewtCore.h"
#include "NewtVM.h"
#include "NewtPrint.h"


#define NewtRefToFILE(r)		((FILE *)NewtRefToAddress(r))   ///< オブジェクト参照をファイル参照に変換


/*------------------------------------------------------------------------*/
/** ファイルをオープンする
 *
 * @param rcvr		[in] レシーバ
 * @param path		[in] パス
 * @param mode		[in] オープンモード
 *
 * @return			ファイル参照
 */

newtRef MyFopen(newtRefArg rcvr, newtRefArg path, newtRefArg mode)
{
	FILE *  stream;

    if (! NewtRefIsString(path))
        return NewtThrow(kNErrNotAString, path);

    if (! NewtRefIsString(mode))
        return NewtThrow(kNErrNotAString, mode);

	stream = fopen(NewtRefToString(path), NewtRefToString(mode));

	if (stream != NULL)
		return NewtMakeAddress(stream);
	else
		return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** ファイルをクローズする
 *
 * @param rcvr		[in] レシーバ
 * @param stream	[in] ファイル参照
 *
 * @return			エラーコード
 */

newtRef MyFclose(newtRefArg rcvr, newtRefArg stream)
{
	int		result;

    if (! NewtRefIsInteger(stream))
        return NewtThrow(kNErrNotAnInteger, stream);

	result = fclose(NewtRefToFILE(stream));

	return NewtMakeInteger(result);
}


newtRef MyFeof(newtRefArg stream)
{
    if (! NewtRefIsInteger(stream))
        return NewtThrow(kNErrNotAnInteger, stream);

	if (feof(NewtRefToFILE(stream)) == 0)
		return kNewtRefNIL;
	else
		return kNewtRefTRUE;
}


newtRef MyFseek(newtRefArg stream, newtRefArg offset, newtRefArg whence)
{
	int		result;
	int		wh;

    if (! NewtRefIsInteger(stream))
        return NewtThrow(kNErrNotAnInteger, stream);

    if (! NewtRefIsInteger(offset))
        return NewtThrow(kNErrNotAnInteger, offset);

    if (! NewtRefIsSymbol(whence))
        return NewtThrow(kNErrNotASymbol, whence);

	if (NewtRefEqual(whence, NSSYM(seek_set)))
		wh = SEEK_SET;
	else if (NewtRefEqual(whence, NSSYM(seek_cur)))
		wh = SEEK_CUR;
	else if (NewtRefEqual(whence, NSSYM(seek_end)))
		wh = SEEK_END;
	else
		wh = SEEK_SET;

	result = fseek(NewtRefToFILE(stream), NewtRefToInteger(offset), wh);

	return NewtMakeInteger(result);
}


newtRef MyFtell(newtRefArg stream)
{
	long  result;

    if (! NewtRefIsInteger(stream))
        return NewtThrow(kNErrNotAnInteger, stream);

	result = ftell(NewtRefToFILE(stream));

	return NewtMakeInteger(result);
}


newtRef MyFrewind(newtRefArg stream)
{
    if (! NewtRefIsInteger(stream))
        return NewtThrow(kNErrNotAnInteger, stream);

	rewind(NewtRefToFILE(stream));

	return kNewtRefNIL;
}


newtRef MyFread(newtRefArg rcvr, newtRefArg stream, newtRefArg len)
{
	newtRefVar  r;
	FILE *  f;
	void *  data;
	size_t  dataSize;
	size_t  readSize;

    if (! NewtRefIsInteger(stream))
        return NewtThrow(kNErrNotAnInteger, stream);

    if (! NewtRefIsInteger(len))
        return NewtThrow(kNErrNotAnInteger, len);

	f = NewtRefToFILE(stream);
	dataSize = NewtRefToInteger(len);

	r = NewtMakeBinary(kNewtRefUnbind, NULL, dataSize, false);

	if (NewtRefIsNIL(r))
        return r;

	data = NewtRefToBinary(r);
	readSize = fread(data, 1, dataSize, f);

	if (readSize == 0)
		r = kNewtRefNIL;
	else if (readSize != dataSize)
		NewtSetLength(r, readSize);

	return r;
}


/*
size_t MyFwriteObj(FILE * f, newtRefArg r)
{
	size_t  result = 0;

    switch (NewtGetRefType(r, true))
    {
        case kNewtNil:
            break;

        case kNewtTrue:
            break;

        case kNewtUnbind:
            break;

        case kNewtSpecial:
//            NewtPrintSpecial(f, r);
            break;

        case kNewtInt30:
        case kNewtInt32:
//            NewtPrintInteger(f, r);
            break;

        case kNewtReal:
//            NewtPrintReal(f, r);
            break;

        case kNewtCharacter:
			{
				int		c;

				c = NewtRefToCharacter(r);
				fputc(c, f);
			}
            break;

        case kNewtMagicPointer:
//            NewtPrintMagicPointer(f, r);
            break;

        case kNewtBinary:
//            NewtPrintBinary(f, r);
            break;

        case kNewtArray:
//            NewtPrintArray(f, r, depth, literal);
            break;

        case kNewtFrame:
//            NewtPrintFrame(f, r, depth, literal);
            break;

        case kNewtSymbol:
			{
				newtSymDataRef	sym;

				sym = NewtRefToSymbol(r);
				result = fwrite(sym->name, 1, NewtSymbolLength(r), f);
			}
            break;

        case kNewtString:
			result = fwrite(NewtRefToString(r), 1, NewtStringLength(r), f);
            break;

        default:
            break;
    }

	return result;
}

newtRef MyFwrite(newtRefArg rcvr, newtRefArg stream, newtRefArg r)
{
	size_t  result;

    if (! NewtRefIsInteger(stream))
        return NewtThrow(kNErrNotAnInteger, stream);

	result = MyFwriteObj(NewtRefToFILE(stream), r);

	return NewtMakeInteger(result);
}

*/

newtRef MyFprint(newtRefArg stream, newtRefArg r)
{
    if (! NewtRefIsInteger(stream))
        return NewtThrow(kNErrNotAnInteger, stream);

	NewtPrint(NewtRefToFILE(stream), r);

	return kNewtRefNIL;
}


#pragma mark -
newtRef MyOpen(newtRefArg rcvr, newtRefArg path, newtRefArg mode)
{
	newtRefVar  stream;

	if (NewtRefIsNIL(rcvr))
		return kNewtRefUnbind;

	stream = MyFopen(rcvr, path, mode);
	NcSetSlot(rcvr, NSSYM(_stream), stream);

	return rcvr;
}


newtRef MyClose(newtRefArg rcvr)
{
	newtRefVar  result;
	newtRefVar  stream;

	if (NewtRefIsNIL(rcvr))
		return kNewtRefUnbind;

	stream = NcGetSlot(rcvr, NSSYM(_stream));

	if (NewtRefIsNIL(stream))
		return kNewtRefUnbind;

	result = MyFclose(rcvr, stream);
	NcSetSlot(rcvr, NSSYM(_stream), kNewtRefNIL);
	NcSetSlot(rcvr, NSSYM(_lineno), kNewtRefNIL);

	return result;
}


newtRef MyEof(newtRefArg rcvr)
{
	if (NewtRefIsNIL(rcvr))
		return kNewtRefUnbind;

	return MyFeof(NcGetSlot(rcvr, NSSYM(_stream)));
}


newtRef MySeek(newtRefArg rcvr, newtRefArg offset, newtRefArg whence)
{
	if (NewtRefIsNIL(rcvr))
		return kNewtRefUnbind;

	return MyFseek(NcGetSlot(rcvr, NSSYM(_stream)), offset, whence);
}


newtRef MyTell(newtRefArg rcvr)
{
	if (NewtRefIsNIL(rcvr))
		return kNewtRefUnbind;

	return MyFtell(NcGetSlot(rcvr, NSSYM(_stream)));
}


newtRef MyRewind(newtRefArg rcvr)
{
	if (NewtRefIsNIL(rcvr))
		return kNewtRefUnbind;

	return MyFrewind(NcGetSlot(rcvr, NSSYM(_stream)));
}


newtRef MyRead(newtRefArg rcvr, newtRefArg len)
{
	if (NewtRefIsNIL(rcvr))
		return kNewtRefUnbind;

	return MyFread(rcvr, NcGetSlot(rcvr, NSSYM(_stream)), len);
}


newtRef MyPrint(newtRefArg rcvr, newtRefArg r)
{
	if (NewtRefIsNIL(rcvr))
		return kNewtRefUnbind;

	return MyFprint(NcGetSlot(rcvr, NSSYM(_stream)), r);
}


newtRef MyGets(newtRefArg rcvr)
{
	newtRefVar  stream;
	newtRefVar  r;

	if (NewtRefIsNIL(rcvr))
		return kNewtRefUnbind;

	stream = NcGetSlot(rcvr, NSSYM(_stream));

    if (! NewtRefIsInteger(stream))
        return NewtThrow(kNErrNotAnInteger, stream);

	r = NewtFgets(NewtRefToFILE(stream));

	if (NewtRefIsNotNIL(r))
	{
		newtRefVar  lineno;
		int32_t n = 0;

		lineno = NcGetSlot(rcvr, NSSYM(_lineno));

		if (NewtRefIsNotNIL(lineno))
			n = NewtRefToInteger(lineno);

		NcSetSlot(rcvr, NSSYM(_lineno), NewtMakeInteger(n + 1));
	}

	return r;
}

newtRef MyGetc(newtRefArg rcvr)
{
	newtRefVar stream;
	FILE* theFile;
	int theResult;
	
	stream = NcGetSlot(rcvr, NSSYM(_stream));
	if (NewtRefIsNIL(stream))
		return kNewtRefUnbind;

	theFile = NewtRefToFILE(stream);
	theResult = fgetc(theFile);
	return NewtMakeInteger(theResult);
}

newtRef MyPutc(newtRefArg rcvr, newtRefArg byte)
{
	newtRefVar stream;
	FILE* theFile;
	char theByte;
	int theResult;
	
	stream = NcGetSlot(rcvr, NSSYM(_stream));
	if (NewtRefIsNIL(stream))
		return kNewtRefUnbind;

	theFile = NewtRefToFILE(stream);
	theByte = NewtRefToInteger(byte);
	theResult = fputc(theByte, theFile);
	return NewtMakeInteger(theResult);
}

newtRef MyWrite(newtRefArg rcvr, newtRefArg binary)
{
	newtRefVar stream;
	FILE* theFile;
	void* data;
	int len;
	int theResult;
	
	stream = NcGetSlot(rcvr, NSSYM(_stream));
	if (NewtRefIsNIL(stream))
		return kNewtRefUnbind;

	theFile = NewtRefToFILE(stream);
	data = NewtRefToBinary(binary);
	len = NewtBinaryLength(binary);
	theResult = fwrite(data, len, 1, theFile);

	return NewtMakeInteger(theResult);
}

/*------------------------------------------------------------------------*/
/** 拡張ライブラリのインストール
 *
 * @return			なし
 */

void newt_install(void)
{
	newtRefVar  r;

/*
    NewtDefGlobalFunc(NSSYM(fopen),		MyFopen,	2, "fopen(path, mode)");
    NewtDefGlobalFunc(NSSYM(fclose),	MyFclose,   1, "fclose(stream)");
    NewtDefGlobalFunc(NSSYM(fread),		MyFread,	1, "fread(stream, len)");
    NewtDefGlobalFunc(NSSYM(fwriteObj),	MyFwrite,	2, "fwriteObj(stream, obj)");
*/

	r = NcMakeFrame();

	NcSetSlot(r, NSSYM(Class),		NSSYM(File));

//	NcSetSlot(r, NSSYM(_gcScript),	NewtMakeNativeFunc(MyClose,		0, "_gcScript()"));
	NcSetSlot(r, NSSYM(Open),		NewtMakeNativeFunc(MyOpen,		2, "Open(path, mode)"));
	NcSetSlot(r, NSSYM(Close),		NewtMakeNativeFunc(MyClose,		0, "Close()"));
	NcSetSlot(r, NSSYM(Eof),		NewtMakeNativeFunc(MyEof,		0, "Eof()"));
	NcSetSlot(r, NSSYM(Seek),		NewtMakeNativeFunc(MySeek,		2, "Seek(offset, whence)"));
	NcSetSlot(r, NSSYM(Tell),		NewtMakeNativeFunc(MyTell,		0, "Tell()"));
	NcSetSlot(r, NSSYM(Rewind),		NewtMakeNativeFunc(MyRewind,	0, "Rewind()"));
	NcSetSlot(r, NSSYM(Read),		NewtMakeNativeFunc(MyRead,		1, "Read(len)"));
	NcSetSlot(r, NSSYM(Print),		NewtMakeNativeFunc(MyPrint,		1, "Print(str)"));
	NcSetSlot(r, NSSYM(Gets),		NewtMakeNativeFunc(MyGets,		0, "Gets()"));
	NcSetSlot(r, NSSYM(Getc),		NewtMakeNativeFunc(MyGetc,		0, "Getc()"));
	NcSetSlot(r, NSSYM(Putc),		NewtMakeNativeFunc(MyPutc,		1, "Putc(byte)"));
	NcSetSlot(r, NSSYM(Write),		NewtMakeNativeFunc(MyWrite,		1, "Write(binary)"));

	NcSetSlot(r, NSSYM(_stream),	kNewtRefNIL);
	NcSetSlot(r, NSSYM(_lineno),	kNewtRefNIL);

	NcDefMagicPointer(NSSYM(protoFILE), r);
}
