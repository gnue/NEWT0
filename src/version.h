/*------------------------------------------------------------------------*/
/**
 * @file	version.h
 * @brief   バージョン情報
 *
 * @author  M.Nukui
 * @date	2003-11-07
 *
 * Copyright (C) 2003-2005 M.Nukui All rights reserved.
 */


#ifndef	NEWTVERSION_H
#define	NEWTVERSION_H

/* マクロ定義 */
#define NEWT_NAME		"newt"									///< コマンド名
#define NEWT_PROTO		"/0"									///< プロト
#define NEWT_VERSION	"0.1.1"									///< バージョン
#define NEWT_COPYRIGHT	"Copyright (C) 2003-2005 Makoto Nukui"  ///< コピーライト
#define NEWT_BUILD		"2005-10-03-2"							///< ビルド番号
#define NEWT_PARAMS		"[switches] [--] [programfile]"			///< 引数

/// 使用方法
#define NEWT_USAGE		"  -t              enable trace mode\n"			\
						"  -l              dump lex info\n"				\
						"  -s              dump syntax tree\n"			\
						"  -b              dump byte code\n"			\
						"  -C directory    change working directory\n"	\
						"  -e 'command'    one line of script\n"		\
						"  -i [symbols]    print function info\n"		\
						"  -v              print version number\n"		\
						"  -h              print this help message\n"	\
						"  --newton,--nos2 Newton OS 2.0 compatible	\n"	\
						"  --copyright     print copyright\n"			\
						"  --version       print version number\n"

/// スタッフロール
#define NEWT_STAFF		"Program\n"										\
                        "  Makoto Nukui\n"								\
                        "\n"											\
                        "Patch contribute\n"							\
                        "  Paul Guyot\n"								\
                        "\n"											\
                        "Special Thanks\n"								\
                        "  sumim\n"										\
                        "  Rihatsu\n"									\
                        "  Paul Guyot\n"								\
						""

#endif /* NEWTVERSION_H */
