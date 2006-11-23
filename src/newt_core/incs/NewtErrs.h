/*------------------------------------------------------------------------*/
/**
 * @file	NewtErrs.h
 * @brief   エラーコード
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * @note	独自定義以外は Apple Newton OS に準拠
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


#ifndef	NEWTERRS_H
#define	NEWTERRS_H

/* マクロ定義 */

#define kNErrNone						0						///< エラーなし
#define kNErrBase						(-48000)				///< システム定義エラー

// エラーベース
#define kNErrObjectBase					(kNErrBase - 200)		///< オブジェクトエラー
#define kNErrBadTypeBase				(kNErrBase - 400)		///< 不正タイプエラー
#define kNErrCompilerBase				(kNErrBase - 600)		///< コンパイラエラー
#define kNErrInterpreterBase			(kNErrBase - 800)		///< インタプリタエラー
#define kNErrFileBase					(kNErrBase - 1000)		///< ファイルエラー（独自定義）
#define kNErrSystemBase					(kNErrBase - 1100)		///< システムエラー（独自定義）
#define kNErrMiscBase					(kNErrBase - 2000)		///< その他のエラー（独自定義）

// オブジェクトエラー
#define kNErrObjectPointerOfNonPtr		(kNErrObjectBase - 0)
#define kNErrBadMagicPointer			(kNErrObjectBase - 1)
#define kNErrEmptyPath					(kNErrObjectBase - 2)
#define kNErrBadSegmentInPath			(kNErrObjectBase - 3)
#define kNErrPathFailed					(kNErrObjectBase - 4)
#define kNErrOutOfBounds				(kNErrObjectBase - 5)
#define kNErrObjectsNotDistinct			(kNErrObjectBase - 6)
#define kNErrLongOutOfRange				(kNErrObjectBase - 7)
#define kNErrSettingHeapSizeTwice		(kNErrObjectBase - 8)
#define kNErrGcDuringGc					(kNErrObjectBase - 9)
#define kNErrBadArgs					(kNErrObjectBase - 10)
#define kNErrStringTooBig				(kNErrObjectBase - 11)
#define kNErrTFramesObjectPtrOfNil		(kNErrObjectBase - 12)
#define kNErrUnassignedTFramesObjectPtr	(kNErrObjectBase - 13)
#define kNErrObjectReadOnly				(kNErrObjectBase - 14)
#define kNErrOutOfObjectMemory			(kNErrObjectBase - 16)
#define kNErrDerefMagicPointer			(kNErrObjectBase - 17)
#define kNErrNegativeLength				(kNErrObjectBase - 18)
#define kNErrOutOfRange					(kNErrObjectBase - 19)
#define kNErrCouldntResizeLockedObject	(kNErrObjectBase - 20)
#define kNErrBadPackageRef				(kNErrObjectBase - 21)
#define kNErrBadExceptionName			(kNErrObjectBase - 22)

// 不正タイプエラー
#define kNErrNotAFrame					(kNErrBadTypeBase - 0)
#define kNErrNotAnArray					(kNErrBadTypeBase - 1)
#define kNErrNotAString					(kNErrBadTypeBase - 2)
#define kNErrNotAPointer				(kNErrBadTypeBase - 3)
#define kNErrNotANumber					(kNErrBadTypeBase - 4)
#define kNErrNotAReal					(kNErrBadTypeBase - 5)
#define kNErrNotAnInteger				(kNErrBadTypeBase - 6)
#define kNErrNotACharacter				(kNErrBadTypeBase - 7)
#define kNErrNotABinaryObject			(kNErrBadTypeBase - 8)
#define kNErrNotAPathExpr				(kNErrBadTypeBase - 9)
#define kNErrNotASymbol					(kNErrBadTypeBase - 10)
#define kNErrNotAFunction				(kNErrBadTypeBase - 11)
#define kNErrNotAFrameOrArray			(kNErrBadTypeBase - 12)
#define kNErrNotAnArrayOrNil			(kNErrBadTypeBase - 13)
#define kNErrNotAStringOrNil			(kNErrBadTypeBase - 14)
#define kNErrNotABinaryObjectOrNil		(kNErrBadTypeBase - 15)
#define kNErrUnexpectedFrame			(kNErrBadTypeBase - 16)
#define kNErrUnexpectedBinaryObject		(kNErrBadTypeBase - 17)
#define kNErrUnexpectedImmediate		(kNErrBadTypeBase - 18)
#define kNErrNotAnArrayOrString			(kNErrBadTypeBase - 19)
#define kNErrNotAVBO					(kNErrBadTypeBase - 20)
#define kNErrNotAPackage				(kNErrBadTypeBase - 21)
#define kNErrNotNil						(kNErrBadTypeBase - 22)
#define kNErrNotASymbolOrNil			(kNErrBadTypeBase - 23)
#define kNErrNotTrueOrNil				(kNErrBadTypeBase - 24)
#define kNErrNotAnIntegerOrArray		(kNErrBadTypeBase - 25)

// コンパイルエラー
#define kNErrSyntaxError				(kNErrCompilerBase - 1)
#define kNErrAssignToConstant			(kNErrCompilerBase - 3)
#define kNErrCantTest					(kNErrCompilerBase - 4)
#define kNErrGlobalVarNotAllowed		(kNErrCompilerBase - 5)
#define kNErrCantHaveSameName			(kNErrCompilerBase - 6)
#define kNErrCantRedefineConstant		(kNErrCompilerBase - 7)
#define kNErrCantHaveSameNameInScope	(kNErrCompilerBase - 8)
#define kNErrNonLiteralExpression		(kNErrCompilerBase - 9)		///< 定数定義
#define kNErrEndOfInputString			(kNErrCompilerBase - 10)
#define kNErrOddNumberOfDigits			(kNErrCompilerBase - 11)	///< \\u
#define kNErrNoEscapes					(kNErrCompilerBase - 12)
#define kNErrInvalidHexCharacter		(kNErrCompilerBase - 13)	///< \\u 文字列
#define kNErrNotTowDigitHex				(kNErrCompilerBase - 17)	///< $\\ 文字
#define kNErrNotFourDigitHex			(kNErrCompilerBase - 18)	///< $\u 文字
#define kNErrIllegalCharacter			(kNErrCompilerBase - 19)
#define kNErrInvalidHexadecimal			(kNErrCompilerBase - 20)
#define kNErrInvalidReal				(kNErrCompilerBase - 21)
#define kNErrInvalidDecimal				(kNErrCompilerBase - 22)
#define kNErrNotConstant				(kNErrCompilerBase - 27)
#define kNErrNotDecimalDigit			(kNErrCompilerBase - 28)	///<  @

// インタプリタエラー
#define kNErrNotInBreakLoop				(kNErrInterpreterBase - 0)
#define kNErrTooManyArgs				(kNErrInterpreterBase - 2)
#define kNErrWrongNumberOfArgs			(kNErrInterpreterBase - 3)
#define kNErrZeroForLoopIncr			(kNErrInterpreterBase - 4)
#define kNErrNoCurrentException			(kNErrInterpreterBase - 6)
#define kNErrUndefinedVariable			(kNErrInterpreterBase - 7)
#define kNErrUndefinedGlobalFunction	(kNErrInterpreterBase - 8)
#define kNErrUndefinedMethod			(kNErrInterpreterBase - 9)
#define kNErrMissingProtoForResend		(kNErrInterpreterBase - 10)
#define kNErrNilContext					(kNErrInterpreterBase - 11)
#define kNErrBadCharForString			(kNErrInterpreterBase - 15)

// インタプリタエラー（独自定義）
#define kNErrInvalidFunc				(kNErrInterpreterBase - 100)	///< 関数が不正
#define kNErrInvalidInstruction			(kNErrInterpreterBase - 101)	///< バイトコードのインストラクションが不正

// ファイルエラー（独自定義）
#define kNErrFileNotFound				(kNErrFileBase - 0)				///< ファイルが存在しない
#define kNErrFileNotOpen				(kNErrFileBase - 1)				///< ファイルがオープンできない
#define kNErrDylibNotOpen				(kNErrFileBase - 2)				///< 動的ライブラリがオープンできない

// システムエラー（独自定義）
#define kNErrSystemError				(kNErrSystemBase - 0)			///< システムエラー

// その他のエラー（独自定義）
#define kNErrDiv0						(kNErrMiscBase - 0)				///< 0で割り算した
																		// Newton OS では例外は発生しない？
#define kNErrRegcomp					(kNErrMiscBase - 1)				///< 正規表現のコンパイルエラー
#define kNErrNSOFWrite					(kNErrMiscBase - 2)				///< NSOFの書込みエラー
#define kNErrNSOFRead					(kNErrMiscBase - 3)				///< NSOFの読込みエラー

#endif /* NEWTERRS_H */
