//--------------------------------------------------------------------------
/**
 * @file  protoREGEX.c
 * @brief 拡張ライブラリ
 *
 * @author M.Nukui
 * @date 2004-06-10
 *
 * Copyright (C) 2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>

#include "NewtLib.h"
#include "NewtCore.h"
#include "NewtVM.h"


bool protoREGEX_refIsPreg(newtRefArg r)
{
    if (! NewtRefIsBinary(r))
	{
        NewtThrow(kNErrNotABinaryObject, r);
		return false;
	}

    if (NewtLength(r) != sizeof(regex_t))
	{
        NewtThrow(kNErrNegativeLength, r);
		return false;
	}

	return true;
}

newtRef protoREGEX_regcomp(newtRefArg pattern, newtRefArg opt)
{
	regex_t	preg;
	int		cflags = REG_EXTENDED;
	int		err;

    if (! NewtRefIsString(pattern))
        return NewtThrow(kNErrNotAString, pattern);

    if (NewtRefIsString(opt))
	{
		char *		optstr;
		uint32_t	len;
		uint32_t	i;

		optstr = NewtRefToString(opt);
		len = NewtLength(opt);

		for (i = 0; i < len && optstr[i]; i++)
		{
			switch (optstr[i])
			{
/*
				case 'e':
					cflags |= REG_EXTENDED;
					break;
*/
				case 'i':
					cflags |= REG_ICASE;
					break;

				case 'm':
					cflags |= REG_NEWLINE;
					break;
			}
		}
	}

	err = regcomp(&preg, NewtRefToString(pattern), cflags);

	if (err != 0)
	{
        return NewtThrow(kNErrRegcomp, pattern);
	}

	return NewtMakeBinary(kNewtRefUnbind, (uint8_t *)&preg, sizeof(preg), false);
}


newtRef protoREGEX_regexec(newtRefArg preg, newtRefArg str)
{
	newtRefVar	substr;
	newtRefVar	r;
	regmatch_t	pmatch[10];
	size_t	nmatch;
	char *	src;
	int		eflags = 0;
	int		err;
	int		i;

    if (NewtRefIsNIL(str))
        return kNewtRefNIL;

    if (! protoREGEX_refIsPreg(preg))
        return kNewtRefUnbind;

    if (! NewtRefIsString(str))
        return NewtThrow(kNErrNotAString, str);

	nmatch = sizeof(pmatch) / sizeof(regmatch_t);

	src = NewtRefToString(str);
    err = regexec((regex_t*)NewtRefToBinary(preg), src, nmatch, pmatch, eflags);

	if (err != 0)
		return kNewtRefNIL;

	r = NewtMakeArray(kNewtRefUnbind, nmatch);

	for (i = 0; i < nmatch; i++)
	{
		if (pmatch[i].rm_so != -1)
		{
			substr = NewtMakeString2(src + pmatch[i].rm_so, pmatch[i].rm_eo - pmatch[i].rm_so, false);
			NewtSetArraySlot(r, i, substr);
		}
	}

	return r;
}


newtRef protoREGEX_regfree(newtRefArg preg)
{
	if (! protoREGEX_refIsPreg(preg))
		return kNewtRefUnbind;

    regfree((regex_t*)NewtRefToBinary(preg));

	return kNewtRefNIL;
}


#pragma mark -

newtRef protoREGEX_compile(newtRefArg rcvr)
{
	newtRefVar	preg;

	if (NewtRefIsNIL(rcvr))
		return kNewtRefUnbind;

	preg = protoREGEX_regcomp(NcGetSlot(rcvr, NSSYM(pattern)), NcGetSlot(rcvr, NSSYM(option)));

	return NcSetSlot(rcvr, NSSYM(_preg), preg);
}


newtRef protoREGEX_match(newtRefArg rcvr, newtRefArg str)
{
	newtRefVar  preg;
	newtRefVar  r;

	if (NewtRefIsNIL(rcvr))
		return kNewtRefUnbind;

	NcSetSlot(rcvr, NSSYM(_matchs), kNewtRefNIL);
	preg = NcGetSlot(rcvr, NSSYM(_preg));

	if (NewtRefIsNIL(preg))
	{
		protoREGEX_compile(rcvr);
		preg = NcGetSlot(rcvr, NSSYM(_preg));

		if (NewtRefIsNIL(preg))
			return kNewtRefNIL;
	}

	r = protoREGEX_regexec(preg, str);

	NcSetSlot(rcvr, NSSYM(_matchs), r);

	return r;
}


newtRef protoREGEX_cleanup(newtRefArg rcvr)
{
	newtRefVar  preg;

	if (NewtRefIsNIL(rcvr))
		return kNewtRefUnbind;

	NcSetSlot(rcvr, NSSYM(_matchs), kNewtRefNIL);
	preg = NcGetSlot(rcvr, NSSYM(_preg));

	if (NewtRefIsNotNIL(preg))
	{
		protoREGEX_regfree(preg);
		NcSetSlot(rcvr, NSSYM(_preg), kNewtRefNIL);
	}

	return kNewtRefNIL;
}


void protoREGEX_install(void) 
{
	newtRefVar  r;
  
	r = NcMakeFrame();
  
	NcSetSlot(r, NSSYM(Class),		NSSYM(Regex));
  
  //	NcSetSlot(r, NSSYM(_gcScript),	NewtMakeNativeFunc(MyCleanup,	0, "_gcScript()"));
	NcSetSlot(r, NSSYM(Compile),	NewtMakeNativeFunc(protoREGEX_compile,	0, "Compile()"));
	NcSetSlot(r, NSSYM(Match),		NewtMakeNativeFunc(protoREGEX_match,		1, "Match(str)"));
	NcSetSlot(r, NSSYM(Cleanup),	NewtMakeNativeFunc(protoREGEX_cleanup,	0, "Cleanup()"));
  
	NcSetSlot(r, NSSYM(pattern),	kNewtRefNIL);
	NcSetSlot(r, NSSYM(option),		kNewtRefNIL);
	NcSetSlot(r, NSSYM(_preg),		kNewtRefNIL);
	NcSetSlot(r, NSSYM(_matchs),	kNewtRefNIL);
  
	NcDefMagicPointer(NSSYM(protoREGEX), r);
}

#if !TARGET_OS_IPHONE
/*------------------------------------------------------------------------*/
/** 拡張ライブラリのインストール
 *
 * @return			なし
 */

void newt_install(void)
{
  protoREGEX_install();
}
#endif
