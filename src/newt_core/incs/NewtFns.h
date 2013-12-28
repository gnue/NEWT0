/*------------------------------------------------------------------------*/
/**
 * @file	NewtFns.h
 * @brief   組込み関数
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * @note	NS...　で始まる関数はインタプリタの関数として定義可能
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTFNS_H
#define	NEWTFNS_H

/*

関数ネーミングルール

  Ns******	NewtonScript ネイティブコード（第一引数に rcvr あり、スクリプトから使用）
  Nc******	NewtonScript ネイティブコード（第一引数に rcvr なし、C言語から使用）
  NVM*****	VM関連
  NPS*****	パーサー関連
  NSOF*****	NSOF関連
  Newt*****	オブジェクト関連、その他

使わないように：(OBSOLETE)
  NS******	Cocoa APIs

*/


/* ヘッダファイル */
#include "NewtType.h"


/* マクロ */
#define NcSelf()					NVMSelf()						///< self を取得
#define NcGetVariable(frame, slot)	NcFullLookup(frame, slot)

#define NcThrow(name, data)			NsThrow(kNewtRefNIL, name, data)
#define NcTotalClone(r)				NsTotalClone(kNewtRefNIL, r)
#define NcDeeplyLength(r)			NsDeeplyLength(kNewtRefNIL, r)
#define NcHasSlot(frame, slot)		NsHasSlot(kNewtRefNIL, frame, slot)
#define NcGetSlot(frame, slot)		NsGetSlot(kNewtRefNIL, frame, slot)
#define NcSetSlot(frame, slot, v)	NsSetSlot(kNewtRefNIL, frame, slot, v)
#define NcRemoveSlot(frame, slot)	NsRemoveSlot(kNewtRefNIL, frame, slot)
#define NcStrCat(str, v)			NsStrCat(kNewtRefNIL, str, v)
#define NcMakeSymbol(r)				NsMakeSymbol(kNewtRefNIL, r)
#define NcMakeFrame()				NsMakeFrame(kNewtRefNIL)
#define NcMakeBinary(len, klass)	NsMakeBinary(kNewtRefNIL, len, klass)
#define NcMakeBinaryFromHex(hex, klass)	NsMakeBinaryFromHex(kNewtRefNIL, hex, klass)
#define NcPrintObject(r)			NsPrintObject(kNewtRefNIL, r)
#define NcPrint(r)					NsPrint(kNewtRefNIL, r)


/* 関数プロトタイプ */

#ifdef __cplusplus
extern "C" {
#endif

// NewtonScript native functions(new style)
newtRef		NcProtoLookupFrame(newtRefArg start, newtRefArg name);
newtRef		NcProtoLookup(newtRefArg start, newtRefArg name);
newtRef		NcLexicalLookup(newtRefArg start, newtRef name);
newtRef		NcFullLookupFrame(newtRefArg start, newtRefArg name);
newtRef		NcFullLookup(newtRefArg start, newtRefArg name);
newtRef		NcLookupSymbol(newtRefArg r, newtRefArg name);

newtRef		NsThrow(newtRefArg rcvr, newtRefArg name, newtRefArg data);
newtRef		NsRethrow(newtRefArg rcvr);
newtRef		NcClone(newtRefArg r);									// bytecode
newtRef		NsTotalClone(newtRefArg rcvr, newtRefArg r);
newtRef		NcLength(newtRefArg r);									// bytecode
newtRef		NsDeeplyLength(newtRefArg rcvr, newtRefArg r);
newtRef		NsSetLength(newtRefArg rcvr, newtRefArg r, newtRefArg len);
newtRef		NsHasSlot(newtRefArg rcvr, newtRefArg frame, newtRefArg slot);
newtRef		NsGetSlot(newtRefArg rcvr, newtRefArg frame, newtRefArg slot);
newtRef		NsSetSlot(newtRefArg rcvr, newtRefArg frame, newtRefArg slot, newtRefArg v);
newtRef		NsRemoveSlot(newtRefArg rcvr, newtRefArg frame, newtRefArg slot);
newtRef		NcSetArraySlot(newtRefArg r, newtRefArg p, newtRefArg v);
newtRef		NcHasPath(newtRefArg r, newtRefArg p);					// bytecode
newtRef		NcGetPath(newtRefArg r, newtRefArg p);					// bytecode
newtRef		NcSetPath(newtRefArg r, newtRefArg p, newtRefArg v);	// bytecode
newtRef		NcARef(newtRefArg r, newtRefArg p);						// bytecode
newtRef		NcSetARef(newtRefArg r, newtRefArg p, newtRefArg v);	// bytecode
newtRef		NsHasVariable(newtRefArg rcvr, newtRefArg r, newtRefArg name);
newtRef		NsGetVariable(newtRefArg rcvr, newtRefArg frame, newtRefArg slot);
newtRef		NsSetVariable(newtRefArg rcvr, newtRefArg frame, newtRefArg slot, newtRefArg v);
newtRef		NsHasVar(newtRefArg rcvr, newtRefArg name);
newtRef		NsPrimClassOf(newtRefArg rcvr, newtRefArg r);
newtRef		NcClassOf(newtRefArg r);								// bytecode
newtRef		NcSetClass(newtRefArg r, newtRefArg c);					// bytecode
newtRef		NcRefEqual(newtRefArg r1, newtRefArg r2);				// bytecode
newtRef		NsObjectEqual(newtRefArg rcvr, newtRefArg r1, newtRefArg r2);
newtRef		NsSymbolCompareLex(newtRefArg rcvr, newtRefArg r1, newtRefArg r2);
newtRef		NsHasSubclass(newtRefArg rcvr, newtRefArg sub, newtRefArg supr);
newtRef		NsIsSubclass(newtRefArg rcvr, newtRefArg sub, newtRefArg supr);
newtRef		NsIsInstance(newtRefArg rcvr, newtRefArg obj, newtRefArg rr);
newtRef		NsIsArray(newtRefArg rcvr, newtRefArg r);
newtRef		NsIsFrame(newtRefArg rcvr, newtRefArg r);
newtRef		NsIsBinary(newtRefArg rcvr, newtRefArg r);
newtRef		NsIsSymbol(newtRefArg rcvr, newtRefArg r);
newtRef		NsIsString(newtRefArg rcvr, newtRefArg r);
newtRef		NsIsCharacter(newtRefArg rcvr, newtRefArg r);
newtRef		NsIsInteger(newtRefArg rcvr, newtRefArg r);
newtRef		NsIsReal(newtRefArg rcvr, newtRefArg r);
newtRef		NsIsNumber(newtRefArg rcvr, newtRefArg r);
newtRef		NsIsImmediate(newtRefArg rcvr, newtRefArg r);
newtRef		NsIsFunction(newtRefArg rcvr, newtRefArg r);
newtRef		NsIsReadonly(newtRefArg rcvr, newtRefArg r);

newtRef		NcAddArraySlot(newtRefArg r, newtRefArg v);				// bytecode
newtRef		NcStringer(newtRefArg r);								// bytecode
newtRef		NsStrCat(newtRefArg rcvr, newtRefArg str, newtRefArg v);
newtRef		NsMakeSymbol(newtRefArg rcvr, newtRefArg r);
newtRef		NsMakeFrame(newtRefArg rcvr);
newtRef NsMakeArray(newtRefArg rcvr, newtRefArg size, newtRefArg initialValue);
newtRef		NsMakeBinary(newtRefArg rcvr, newtRefArg length, newtRefArg klass);
newtRef		NsMakeBinaryFromHex(newtRefArg rcvr, newtRefArg hex, newtRefArg klass);

newtRef		NcBAnd(newtRefArg r1, newtRefArg r2);					// bytecode
newtRef		NcBOr(newtRefArg r1, newtRefArg r2);					// bytecode
newtRef		NcBNot(newtRefArg r);									// bytecode
newtRef		NsAnd(newtRefArg rcvr, newtRefArg r1, newtRefArg r2); 
newtRef		NsOr(newtRefArg rcvr, newtRefArg r1, newtRefArg r2); 
newtRef		NcAdd(newtRefArg r1, newtRefArg r2);					// bytecode
newtRef		NcSubtract(newtRefArg r1, newtRefArg r2);				// bytecode
newtRef		NcMultiply(newtRefArg r1, newtRefArg r2);				// bytecode
newtRef		NcDivide(newtRefArg r1, newtRefArg r2); 				// bytecode
newtRef		NcDiv(newtRefArg r1, newtRefArg r2); 					// bytecode
newtRef		NsMod(newtRefArg rcvr, newtRefArg r1, newtRefArg r2); 
newtRef		NsShiftLeft(newtRefArg rcvr, newtRefArg r1, newtRefArg r2); 
newtRef		NsShiftRight(newtRefArg rcvr, newtRefArg r1, newtRefArg r2); 
newtRef		NcLessThan(newtRefArg r1, newtRefArg r2);				// bytecode
newtRef		NcGreaterThan(newtRefArg r1, newtRefArg r2);			// bytecode
newtRef		NcGreaterOrEqual(newtRefArg r1, newtRefArg r2);			// bytecode 
newtRef		NcLessOrEqual(newtRefArg r1, newtRefArg r2);			// bytecode

newtRef		NsCurrentException(newtRefArg rcvr);
newtRef		NsMakeRegex(newtRefArg rcvr, newtRefArg pattern, newtRefArg opt);

newtRef 	NsApply(newtRefArg rcvr, newtRefArg func, newtRefArg params);
newtRef 	NsPerform(newtRefArg rcvr, newtRefArg frame, newtRefArg message, newtRefArg params);
newtRef 	NsPerformIfDefined(newtRefArg rcvr, newtRefArg frame, newtRefArg message, newtRefArg params);
newtRef 	NsProtoPerform(newtRefArg rcvr, newtRefArg frame, newtRefArg message, newtRefArg params);
newtRef 	NsProtoPerformIfDefined(newtRefArg rcvr, newtRefArg frame, newtRefArg message, newtRefArg params);

newtRef		NsPrintObject(newtRefArg rcvr, newtRefArg r);
newtRef		NsPrint(newtRefArg rcvr, newtRefArg r);
newtRef		NsInfo(newtRefArg rcvr, newtRefArg r);
newtRef		NsDumpFn(newtRefArg rcvr, newtRefArg r);
newtRef		NsDumpBC(newtRefArg rcvr, newtRefArg r);
newtRef		NsDumpStacks(newtRefArg rcvr);

newtRef		NsCompile(newtRefArg rcvr, newtRefArg r);
newtRef		NsGetEnv(newtRefArg rcvr, newtRefArg r);

newtRef		NsExtractByte(newtRefArg rcvr, newtRefArg r, newtRefArg offset);
newtRef		NsExtractWord(newtRefArg rcvr, newtRefArg r, newtRefArg offset);

newtRef NsRef(newtRefArg rcvr, newtRefArg integer);
newtRef NsRefOf(newtRefArg rcvr, newtRefArg object);
newtRef NsNegate(newtRefArg rcvr, newtRefArg integer);
newtRef NsSetContains(newtRefArg rcvr, newtRefArg array, newtRefArg item);

#ifdef __cplusplus
}
#endif


#endif /* NEWTFNS_H */

