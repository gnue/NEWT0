/*------------------------------------------------------------------------*/
/**
 * @file	NewtPrint.c
 * @brief   プリント関係
 *
 * @author  M.Nukui
 * @date	2005-04-11
 *
 * Copyright (C) 2003-2005 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <ctype.h>

#include "NewtPrint.h"
#include "NewtCore.h"
#include "NewtObj.h"
#include "NewtEnv.h"
#include "NewtIO.h"


/* 関数プロトタイプ */

static int32_t		NewtGetPrintLength(void);
static int32_t		NewtGetPrintDepth(void);
static int32_t		NewtGetPrintIndent(void);
static int32_t		NewtGetPrintBinaries(void);

static bool			NewtSymbolIsPrint(char * str, int len);
static bool			NewtStrIsPrint(char * str, int len);
static char *		NewtCharToEscape(int c);

static void			NIOPrintIndent(newtStream_t * f, int32_t level);
static void			NIOPrintEscapeStr(newtStream_t * f, char * str, int len, char bar);
static void			NIOPrintRef(newtStream_t * f, newtRefArg r);
static void			NIOPrintSpecial(newtStream_t * f, newtRefArg r);
static void			NIOPrintInteger(newtStream_t * f, newtRefArg r);
static void			NIOPrintReal(newtStream_t * f, newtRefArg r);
static void			NIOPrintObjCharacter(newtStream_t * f, newtRefArg r);
static void			NIOPrintObjNamedMP(newtStream_t * f, newtRefArg r);
static void			NIOPrintObjMagicPointer(newtStream_t * f, newtRefArg r);
static void			NIOPrintObjBinary(newtStream_t * f, newtRefArg r);
static void			NIOPrintObjSymbol(newtStream_t * f, newtRefArg r);
static void			NIOPrintObjString(newtStream_t * f, newtRefArg r);
static void			NIOPrintObjArray(newtStream_t * f, newtRefArg r, int32_t depth, bool literal);
static void			NIOPrintFnFrame(newtStream_t * f, newtRefArg r);
static void			NIOPrintRegexFrame(newtStream_t * f, newtRefArg r);
static void			NIOPrintObjFrame(newtStream_t * f, newtRefArg r, int32_t depth, bool literal);
static void			NIOPrintLiteral(newtStream_t * f, newtRefArg r, bool * literalP);
static void			NIOPrintObj2(newtStream_t * f, newtRefArg r, int32_t depth, bool literal);

static void			NIOPrintCharacter(newtStream_t * f, newtRefArg r);
static void			NIOPrintSymbol(newtStream_t * f, newtRefArg r);
static void			NIOPrintString(newtStream_t * f, newtRefArg r);
static void			NIOPrintArray(newtStream_t * f, newtRefArg r);
static void			NIOPrintObj(newtStream_t * f, newtRefArg r);
static void			NIOPrint(newtStream_t * f, newtRefArg r);

static void			NIOInfo(newtStream_t * f, newtRefArg r);


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 配列またはフレームのプリント可能な長さを返す
 *
 * @return			プリント可能な長さ
 */

int32_t NewtGetPrintLength(void)
{
	newtRefVar	n;
	int32_t		printLength = -1;

	n = NcGetGlobalVar(NSSYM0(printLength));

	if (NewtRefIsInteger(n))
		printLength = NewtRefToInteger(n);

	if (printLength < 0)
		printLength = 0x7fffffff;

	return printLength;
}


/*------------------------------------------------------------------------*/
/** 配列またはフレームのプリント可能な深さを返す
 *
 * @return			プリント可能な深さ
 */

int32_t NewtGetPrintDepth(void)
{
    newtRefVar	n;
    int32_t		depth = 3;

    n = NcGetGlobalVar(NSSYM0(printDepth));

    if (NewtRefIsInteger(n))
        depth = NewtRefToInteger(n);

	return depth;
}


/*------------------------------------------------------------------------*/
/** Get the number of tabs (or spaces if negative) for indenting.
 *
 * @return			number of characters to indent
 */

int32_t NewtGetPrintIndent(void)
{
    newtRefVar	n;
    int32_t		indent = 1;

    n = NcGetGlobalVar(NSSYM0(printIndent));

    if (NewtRefIsInteger(n))
        indent = NewtRefToInteger(n);

	return indent;
}


/*------------------------------------------------------------------------*/
/** If set, brint binary objects as well.
 *
 * @return			0 or 1
 */

int32_t NewtGetPrintBinaries(void)
{
    newtRefVar	n;
    int32_t		pb = 0;

    n = NcGetGlobalVar(NSSYM0(printBinaries));

    if (NewtRefIsInteger(n))
        pb = NewtRefToInteger(n);

	return pb;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** シンボル文字列が表示可能か調べる
 *
 * @param str		[in] シンボル文字列
 * @param len		[in] 文字列の長さ
 *
 * @retval			true	表示可能
 * @retval			false	表示不可
 */

bool NewtSymbolIsPrint(char * str, int len)
{
	int c;
	int i;

	if (len == 0)
		return false;

	c = str[0];

	if ('0' <= c && c <= '9')
		return false;

	for (i = 0; i < len && str[i]; i++)
	{
		c = str[i];

		if (c == '_') continue;
		if ('a' <= c && c <= 'z') continue;
		if ('A' <= c && c <= 'Z') continue;
		if ('0' <= c && c <= '9') continue;

		return false;
	}

	return true;
}


/*------------------------------------------------------------------------*/
/** 文字列が表示可能か調べる
 *
 * @param str		[in] 文字列
 * @param len		[in] 文字列の長さ
 *
 * @retval			true	表示可能
 * @retval			false	表示不可
 */

bool NewtStrIsPrint(char * str, int len)
{
	int		i;

	for (i = 0; i < len && str[i]; i++)
	{
		if (str[i] == '"')
			return false;

		if (! isprint(str[i]))
			return false;
	}

	return true;
}


/*------------------------------------------------------------------------*/
/** Convert a bunch characters into their escaped variant.
 *  This is required when printing symbols containing vertical bars.
 *
 * @param c			[in] unicode character
 *
 * @return		 	ASCII string or NULL
 */

char * NewtCharAndBarToEscape(int c)
{	
	if (c=='|') {
		return "\\|";
	} else if (c=='\\') { 
		return "\\\\";
	} else {
		return NewtCharToEscape(c);
	}
}


/*------------------------------------------------------------------------*/
/** 文字をエスケープ文字列に変換する
 *
 * @param c			[in] 文字
 *
 * @return			エスケープ文字列
 */

char * NewtCharToEscape(int c)
{
	char *	s = NULL;

	switch (c)
	{
		case '\n':
			s = "\\n";
			break;

		case '\r':
			s = "\\r";
			break;

		case '\t':
			s = "\\t";
			break;

		case '"':
			s = "\\\"";
			break;
	}

	return s;
}


/*------------------------------------------------------------------------*/
/** Print a NewLine character and some spaces to indent the following text. 
 * @param f			[in] destination stream
 * @param depth		[in] max indent level minus current indent level
 */
void NIOPrintIndent(newtStream_t * f, int32_t depth)
{
	int32_t i, n = newt_env._indentDepth - depth;
	NIOFputc('\n', f);
	if (NEWT_INDENT>0) {
		n *= NEWT_INDENT;
		for (i=0; i<n; i++) {
			NIOFputc('\t', f);
		}
	} else {
		n *= -NEWT_INDENT;
		for (i=0; i<n; i++) {
			NIOFputc(' ', f);
		}
	}
}


/*------------------------------------------------------------------------*/
/** 文字列をエスケープでプリントする
 *
 * @param f		[in] 出力ファイル
 * @param str		[in] 文字列
 * @param len		[in] 文字列の長さ
 * @param bar		[in] set to 1 to also create an escape sequence 
 *			     for the vertical bar character
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintEscapeStr(newtStream_t * f, char * str, int len, char bar)
{
	bool	unicode = false;
	char *	s;
	int		c;
	int		i;

	for (i = 0; i < len; i++)
	{
		c = str[i];

		if (bar)
			s = NewtCharAndBarToEscape(c);
		else
			s = NewtCharToEscape(c);

		if (s != NULL)
		{
			if (unicode)
			{
				NIOFputs("\\u", f);
				unicode = false;
			}

			NIOFputs(s, f);
		}
		else if (isprint(c))
		{
			if (unicode)
			{
				NIOFputs("\\u", f);
				unicode = false;
			}

			NIOFputc(c, f);
		}
		else
		{
			if (! unicode)
			{
				NIOFputs("\\u", f);
				unicode = true;
			}

			NIOFprintf(f, "%02x%02x", c, str[++i]);
		}
	}
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** 出力ファイルにオブジェクト参照を１６進数でプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintRef(newtStream_t * f, newtRefArg r)
{
    NIOFprintf(f, "#%08x", r);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに特殊オブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintSpecial(newtStream_t * f, newtRefArg r)
{
    int	n;

    n = NewtRefToSpecial(r);
    NIOFprintf(f, "<Special, %04x>", n);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに整数オブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintInteger(newtStream_t * f, newtRefArg r)
{
    int	n;

    n = NewtRefToInteger(r);
    NIOFprintf(f, "%d", n);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに浮動小数点オブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintReal(newtStream_t * f, newtRefArg r)
{
    double	n;

    n = NewtRefToReal(r);
    NIOFprintf(f, "%f", n);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに文字オブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintObjCharacter(newtStream_t * f, newtRefArg r)
{
	char *	s;
    int		c;

    c = NewtRefToCharacter(r);

	NIOFputc('$', f);

	s = NewtCharToEscape(c);

	if (s != NULL)
		NIOFputs(s, f);
	else if (0xff00 & c)
		NIOFprintf(f, "\\u%04x", c);
	else if (isprint(c))
		NIOFputc(c, f);
	else
		NIOFprintf(f, "\\%02x", c);
}


#ifdef __NAMED_MAGIC_POINTER__

/*------------------------------------------------------------------------*/
/** 出力ファイルに名前付マジックポインタをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintObjNamedMP(newtStream_t * f, newtRefArg r)
{
	newtRefVar	sym;

	sym = NewtMPToSymbol(r);

	NIOFputc('@', f);
	NIOPrintObjSymbol(f, sym);
}

#endif /* __NAMED_MAGIC_POINTER__ */


/*------------------------------------------------------------------------*/
/** 出力ファイルにマジックポインタをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintObjMagicPointer(newtStream_t * f, newtRefArg r)
{
	int32_t	table;
	int32_t	index;

	table = NewtMPToTable(r);
	index = NewtMPToIndex(r);

    NIOFprintf(f, "@%d", index);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにバイナリオブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintObjBinary(newtStream_t * f, newtRefArg r)
{
    newtRefVar	klass;
    int	ptr;
    int	len;

    ptr = r;
    len = NewtBinaryLength(r);
    klass = NcClassOf(r);
    if (newt_env._printBinaries)
    {
        uint8_t *data = NewtRefToBinary(r);
        NIOFputs("MakeBinaryFromHex(\"", f);
        int i; for (i=0; i<len; i++) NIOFprintf(f, "%02X", data[i]);
        if (NewtRefIsSymbol(klass))
        {
            NIOFputs("\", '", f);
            NIOPrintObj2(f, klass, 0, true);
            NIOFputs(")", f);
        } else {
            NIOFputs("\", NIL)", f);
        }
    } else {
        NIOFputs("<Binary, ", f);
        if (NewtRefIsSymbol(klass))
        {
            NIOFputs("class \"", f);
            NIOPrintObj2(f, klass, 0, true);
            NIOFputs("\", ", f);
        }
        NIOFprintf(f, "length %d>", len);
    }
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにシンボルオブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintObjSymbol(newtStream_t * f, newtRefArg r)
{
    newtSymDataRef	sym;
	int		len;

    sym = NewtRefToSymbol(r);
	len = NewtSymbolLength(r);

	if (NewtSymbolIsPrint(sym->name, len))
	{
		NIOFputs(sym->name, f);
	}
	else
	{
		NIOFputc('|', f);
		NIOPrintEscapeStr(f, sym->name, len, 1);
		NIOFputc('|', f);
	}
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに文字列オブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintObjString(newtStream_t * f, newtRefArg r)
{
    char *	s;
	int		len;

    s = NewtRefToString(r);
	len = NewtStringLength(r);

	if (NewtStrIsPrint(s, len))
	{
//		NIOFprintf(f, "\"%s\"", s);
		NIOFputs("\"", f);
		NIOFputs(s, f);
		NIOFputs("\"", f);
	}
	else
	{
		NIOFputc('"', f);
		NIOPrintEscapeStr(f, s, len, 0);
		NIOFputc('"', f);
	}
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに配列オブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 * @param depth		[in] 深さ
 * @param literal	[in] リテラルフラグ
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintObjArray(newtStream_t * f, newtRefArg r, int32_t depth, bool literal)
{
    newtObjRef	obj;
    newtRef *	slots;
    newtRefVar	klass;
    uint32_t	len;
    uint32_t	i;

    obj = NewtRefToPointer(r);
    len = NewtObjSlotsLength(obj);
    slots = NewtObjToSlots(obj);

    if (literal && NcClassOf(r) == NSSYM0(pathExpr))
    {
        for (i = 0; i < len; i++)
        {
            if (0 < i)
                NIOFputs(".", f);
    
            NIOPrintObj2(f, slots[i], 0, literal);
        }
    }
    else
    {
        NIOFputs("[", f);
    
        klass = NcClassOf(r);
    
        if (NewtRefIsNotNIL(klass) && ! NewtRefEqual(klass, NSSYM0(array)))
        {
			if (NEWT_INDENT) NIOPrintIndent(f, depth-1);
            NIOPrintObj2(f, klass, 0, true);
            NIOFputs(": ", f);
        }
    
        if (depth < 0)
        {
            NIOPrintRef(f, r);
        }
        else
        {
			int32_t	printLength;

            depth--;
			printLength = NewtGetPrintLength();

            for (i = 0; i < len; i++)
            {
                if (0 < i)
                    NIOFputs(", ", f);
        
				if (printLength <= i)
				{
					NIOFputs("...", f);
					break;
				}

				if (NEWT_INDENT) NIOPrintIndent(f, depth);
                NIOPrintObj2(f, slots[i], depth, literal);
            }
			depth++;
        }

		if (NEWT_INDENT) NIOPrintIndent(f, depth);
        NIOFputs("]", f);
    }
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに関数オブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintFnFrame(newtStream_t * f, newtRefArg r)
{
	newtRefVar	indefinite;
    int32_t		numArgs;
	char *		indefiniteStr = "";

    numArgs = NewtRefToInteger(NcGetSlot(r, NSSYM0(numArgs)));
	indefinite = NcGetSlot(r, NSSYM0(indefinite));

	if (NewtRefIsNotNIL(indefinite))
		indefiniteStr = "...";

	NIOFprintf(f, "<function, %d arg(s)%s #%08x>", numArgs, indefiniteStr, r);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに正規表現オブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintRegexFrame(newtStream_t * f, newtRefArg r)
{
	newtRefVar	pattern;
	newtRefVar	option;

	pattern = NcGetSlot(r, NSSYM0(pattern));
	option = NcGetSlot(r, NSSYM0(option));

//    NIOFprintf(f, "/%s/", NewtRefToString(pattern));
    NIOFputs("/", f);
    NIOFputs(NewtRefToString(pattern), f);
    NIOFputs("/", f);

	if (NewtRefIsString(option))
		NIOFputs(NewtRefToString(option), f);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにフレームオブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 * @param depth		[in] 深さ
 * @param literal	[in] リテラルフラグ
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintObjFrame(newtStream_t * f, newtRefArg r, int32_t depth, bool literal)
{
    newtObjRef	obj;
    newtRef *	slots;
    newtRefVar	slot;
    uint32_t	index;
    uint32_t	len;
    uint32_t	i;

    if (!newt_env._printBinaries)
    {
        if (NewtRefIsFunction(r) && ! NEWT_DUMPBC)
        {
            NIOPrintFnFrame(f, r);
            return;
        }

        if (NewtRefIsRegex(r) && ! NEWT_DUMPBC)
        {
            NIOPrintRegexFrame(f, r);
            return;
        }
    }

    obj = NewtRefToPointer(r);
    len = NewtObjSlotsLength(obj);
    slots = NewtObjToSlots(obj);

    NIOFputs("{", f);

    if (depth < 0)
    {
        NIOPrintRef(f, r);
    }
    else
    {
		int32_t	printLength;

        depth--;
		printLength = NewtGetPrintLength();

        for (i = 0; i < len; i++)
        {
            if (0 < i) {
                NIOFputs(", ", f);
			}

			if (printLength <= i)
			{
				NIOFputs("...", f);
				if (NEWT_INDENT) NIOPrintIndent(f, depth);
				break;
			}
			if (NEWT_INDENT) NIOPrintIndent(f, depth);

            slot = NewtGetMapIndex(obj->as.map, i, &index);
			if (slot == kNewtRefUnbind) break;

            NIOPrintObjSymbol(f, slot);
            NIOFputs(": ", f);
            NIOPrintObj2(f, slots[i], depth, literal);

        }
		if (NEWT_INDENT) NIOPrintIndent(f, depth+1);
    }

    NIOFputs("}", f);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにリテラルの印をプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 * @param literalP	[i/o]リテラルフラグ
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintLiteral(newtStream_t * f, newtRefArg r, bool * literalP)
{
    if (! *literalP && NewtRefIsLiteral(r))
    {
		NIOFputc('\'', f);
        *literalP = true;
    }
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにオブジェクトをプリント（再帰呼出し用）
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 * @param depth		[in] 深さ
 * @param literal	[in] リテラルフラグ
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintObj2(newtStream_t * f, newtRefArg r, int32_t depth, bool literal)
{
    switch (NewtGetRefType(r, true))
    {
        case kNewtNil:
            NIOFputs("NIL", f);
            break;

        case kNewtTrue:
            NIOFputs("TRUE", f);
            break;

        case kNewtUnbind:
            NIOFputs("#UNBIND", f);
            break;

        case kNewtSpecial:
            NIOPrintSpecial(f, r);
            break;

        case kNewtInt30:
        case kNewtInt32:
            NIOPrintInteger(f, r);
            break;

        case kNewtReal:
            NIOPrintReal(f, r);
            break;

        case kNewtCharacter:
            NIOPrintObjCharacter(f, r);
            break;

        case kNewtMagicPointer:
			if (0 <= depth)
			{
				newtRefVar	r2;

				r2 = NcResolveMagicPointer(r);

				if (! NewtRefIsMagicPointer(r2))
				{
					NIOPrintObj2(f, r2, depth, literal);
					break;
				}
			}

#ifdef __NAMED_MAGIC_POINTER__
			if (NewtRefIsNamedMP(r))
			{	// Named Magic Ponter
				NIOPrintObjNamedMP(f, r);
				break;
			}
#endif /* __NAMED_MAGIC_POINTER__ */

			NIOPrintObjMagicPointer(f, r);
            break;

        case kNewtBinary:
            NIOPrintObjBinary(f, r);
            break;

        case kNewtArray:
            NIOPrintLiteral(f, r, &literal);
            NIOPrintObjArray(f, r, depth, literal);
            break;

        case kNewtFrame:
            NIOPrintLiteral(f, r, &literal);
            NIOPrintObjFrame(f, r, depth, literal);
            break;

        case kNewtSymbol:
            NIOPrintLiteral(f, r, &literal);
            NIOPrintObjSymbol(f, r);
            break;

        case kNewtString:
            NIOPrintObjString(f, r);
            break;

        default:
            NIOFputs("###UNKNOWN###", f);
            break;
    }
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにオブジェクトをプリント
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintObj(newtStream_t * f, newtRefArg r)
{
	int32_t depth = NewtGetPrintDepth();
	newt_env._indentDepth = depth;
	int32_t indent = NewtGetPrintIndent();
	newt_env._indent = indent;
	int32_t pb = NewtGetPrintBinaries();
	newt_env._printBinaries = pb;
    NIOPrintObj2(f, r, depth, false);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにオブジェクトをプリント
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 */

void NewtPrintObj(FILE * f, newtRefArg r)
{
	newtStream_t	stream;

	NIOSetFile(&stream, f);
    NIOPrintObj(&stream, r);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにオブジェクトをプリント（改行あり）
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 */

void NewtPrintObject(FILE * f, newtRefArg r)
{
	newtStream_t	stream;

	NIOSetFile(&stream, f);

    NIOPrintObj(&stream, r);
    NIOFputs("\n", &stream);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに文字オブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintCharacter(newtStream_t * f, newtRefArg r)
{
	NIOFputc(NewtRefToCharacter(r), f);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにシンボルオブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintSymbol(newtStream_t * f, newtRefArg r)
{
    newtSymDataRef	sym;

	sym = NewtRefToSymbol(r);
	NIOFputs(sym->name, f);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに文字列オブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintString(newtStream_t * f, newtRefArg r)
{
	NIOFputs(NewtRefToString(r), f);
}


/*------------------------------------------------------------------------*/
/** 出力ファイルに配列オブジェクトをプリントする
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrintArray(newtStream_t * f, newtRefArg r)
{
	newtRef *	slots;
	uint32_t	len;
	uint32_t	i;

	slots = NewtRefToSlots(r);
	len = NewtLength(r);

	for (i = 0; i < len; i++)
	{
		NIOPrint(f, slots[i]);
	}
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにオブジェクトをプリント
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOPrint(newtStream_t * f, newtRefArg r)
{
    switch (NewtGetRefType(r, true))
    {
        case kNewtNil:
        case kNewtTrue:
        case kNewtUnbind:
            break;

        case kNewtSpecial:
            NIOPrintSpecial(f, r);
            break;

        case kNewtInt30:
        case kNewtInt32:
            NIOPrintInteger(f, r);
            break;

        case kNewtReal:
            NIOPrintReal(f, r);
            break;

        case kNewtCharacter:
			NIOPrintCharacter(f, r);
            break;

        case kNewtMagicPointer:
			{
				newtRefVar	r2;

				r2 = NcResolveMagicPointer(r);

				if (! NewtRefIsMagicPointer(r2))
				{
					NIOPrint(f, r2);
				}
			}
            break;

        case kNewtArray:
			NIOPrintArray(f, r);
			break;

        case kNewtBinary:
        case kNewtFrame:
            break;

        case kNewtSymbol:
			NIOPrintSymbol(f, r);
            break;

        case kNewtString:
			NIOPrintString(f, r);
            break;

        default:
            break;
    }
}


/*------------------------------------------------------------------------*/
/** 出力ファイルにオブジェクトをプリント
 *
 * @param f			[in] 出力ファイル
 * @param r			[in] オブジェクト
 *
 * @return			なし
 */

void NewtPrint(FILE * f, newtRefArg r)
{
	newtStream_t	stream;

	NIOSetFile(&stream, f);
	NIOPrint(&stream, r);
}


/*------------------------------------------------------------------------*/
/** 標準出力に関数情報を表示
 *
 * @param r			[in] オブジェクト
 *
 * @return			なし
 *
 * @note			newtStream_t を使用
 */

void NIOInfo(newtStream_t * f, newtRefArg r)
{
    newtRefVar	fn = kNewtRefUnbind;

    if (NewtRefIsSymbol(r))
        fn = NcGetGlobalFn(r);
    else
        fn = (newtRef)r;

    if (NewtRefIsFunction(fn))
    {
        newtRefVar	docString;

        docString = NcGetSlot(fn, NSSYM0(docString));

        if (NewtRefIsNotNIL(docString))
        {
            if (NewtRefIsString(docString))
			{
//                NIOFprintf(f, "%s\n", NewtRefToString(docString));
                NIOFputs(NewtRefToString(docString), f);
                NIOFputs("\n", f);
            }
			else
            {
			    NIOPrintObj(f, docString);
			}
        }
    }
}


/*------------------------------------------------------------------------*/
/** 標準出力に関数情報を表示
 *
 * @param r			[in] オブジェクト
 *
 * @return			なし
 */

void NewtInfo(newtRefArg r)
{
	newtStream_t	stream;

	NIOSetFile(&stream, stdout);
	NIOInfo(&stream, r);
}


/*------------------------------------------------------------------------*/
/** 標準出力に全グローバル関数の関数情報を表示
 *
 * @return			なし
 */

void NewtInfoGlobalFns(void)
{
	newtStream_t	stream;
    newtRef *	slots;
    newtRefVar	fns;
    uint32_t	len;
    uint32_t	i;

	NIOSetFile(&stream, stdout);

    fns = NcGetGlobalFns();
    len = NewtLength(fns);

    slots = NewtRefToSlots(fns);

    for (i = 0; i < len; i++)
    {
        NIOInfo(&stream, slots[i]);
    }
}

