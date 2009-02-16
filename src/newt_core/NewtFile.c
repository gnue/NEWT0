/*------------------------------------------------------------------------*/
/**
 * @file	NewtFile.c
 * @brief   ファイル処理
 *
 * @author  M.Nukui
 * @date	2004-01-25
 *
 * Copyright (C) 2003-2004 M.Nukui All rights reserved.
 */


/* ヘッダファイル */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "NewtType.h"


#if defined(__WIN32__)
	#include "win/dlfcn.h"
#elif defined(HAVE_DLOPEN)
	#include <dlfcn.h>
#endif

#ifdef HAVE_GETPWNAM
	#include <pwd.h>
#endif /* HAVE_GETPWNAM */


#include "NewtCore.h"
#include "NewtVM.h"
#include "NewtBC.h"
#include "NewtIO.h"
#include "NewtFile.h"


/* 定数 */

enum {
	typeScript,
	typeDylib,
};


/* 型宣言 */

/// ファイル拡張子
typedef struct {
	newtRefVar  ext;	///< 拡張子
	int			type;	///< タイプ
} file_ext_t;


#ifdef HAVE_DLOPEN
/*------------------------------------------------------------------------*/
/** 動的ライブラリをインストールする
 *
 * @param fname		[in] ファイルのパス
 *
 * @return			動的ライブラリのデスクプリタ
 */

void * NewtDylibInstall(const char * fname)
{
    newt_install_t	install_call;
    void *	lib;

    lib = dlopen(fname, RTLD_LAZY);

    if (lib != NULL)
    {
        install_call = (newt_install_t)dlsym(lib, "newt_install");

        if (install_call == NULL)
        {
            dlclose(lib);
            return NULL;
        }

        (*install_call)();
    }

    return lib;
}

#endif /* HAVE_DLOPEN */


/*------------------------------------------------------------------------*/
/** ファイルの存在確認
 *
 * @param path		[in] ファイルのパス
 *
 * @retval			true	ファイルが存在する
 * @retval			false   ファイルが存在しない
 */

bool NewtFileExists(char * path)
{
	FILE *  f;

	f = fopen(path, "r");

	if (f != NULL)
	{
		fclose(f);
		return true;
	}

	return false;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** ファイルセパレータを返す
 *
 * @return			ファイルセパレータ
 */

char NewtGetFileSeparator(void)
{
	return '/';
}


/*------------------------------------------------------------------------*/
/** ホームディレクトリのパスを取得
 *
 * @param s			[in] ファイルのパス
 * @param subdir	[out]サブディレクトリ
 *
 * @return			ホームディレクトリのパス
 *
 * @note			取得されたホームディレクトリの文字列は free する必要がある
 */

#ifdef HAVE_GETPWNAM

char * NewtGetHomeDir(const char * s, char ** subdir)
{	// UNIX の場合
	struct passwd * pswd = NULL;
	uint32_t	len;
	char *  login = NULL;
	char *  dir = NULL;
	char *  sepp;
	char	sep;

	sep = NewtGetFileSeparator();
 	sepp = strchr(s + 1, sep);

	if (sepp != NULL)
	{
		len = sepp - (s + 1);
		login = malloc(len + 1);
		strncpy(login, s + 1, len);
		pswd = getpwnam(s + 1);
	}
	else
	{
		login = (char *)s + 1;
	}

	if (*login)
		pswd = getpwnam(login);
	else
		pswd = getpwuid(getuid());

	if (pswd != NULL)
		dir = pswd->pw_dir;

	if (subdir != NULL)
		*subdir = sepp;

	if (s + 1 != login)
		free(login);

	return dir;
}

#else

char * NewtGetHomeDir(const char * s, char ** subdir)
{	// Windows の場合
	return NULL;
}

#endif /* HAVE_GETPWNAM */


/*------------------------------------------------------------------------*/
/** ディレクトリ名とファイル名からパスを作成
 *
 * @param s1		[in] ディレクトリ名
 * @param s2		[in] ファイル名
 * @param sep		[in] ファイルセパレータ
 *
 * @return			作成されたパス
 *
 * @note			取得されたホームディレクトリの文字列は free する必要がある
 */

char * NewtJoinPath(char * s1, char * s2, char sep)
{
	char *		path;
	uint32_t	len;
	uint32_t	len1;
	uint32_t	len2;

	len1 = strlen(s1);
	len2 = strlen(s2);

	len = len1 + len2 + 2;

	path = malloc(len);
	if (path == NULL) return NULL;

	strcpy(path, s1);

	path[len1] = sep;
	strncpy(path + len1 + 1, s2, len2 + 1);

	return path;
}


/*------------------------------------------------------------------------*/
/** 相対パスを絶対パスに展開する
 *
 * @param s			[i/o]相対パス→絶対パス
 *
 * @return			絶対パス
 */

char * NewtRelToAbsPath(char * s)
{
	char *  src;
	char *  dst;
	char	sep;

	sep = NewtGetFileSeparator();

	for (src = dst = s; *src;)
	{
		if (src[0] == sep && src[1] == '.')
		{
			if (src[2] == sep || ! src[2])
			{
				src += 2;
				continue;
			}
			else if (src[2] == '.' && src[3] == sep)
			{
				src += 3;

				while (s < dst)
				{
					dst--;
					if (*dst == sep) break;
				}

				continue;
			}
		}

		if (src != dst)
			*dst = *src;

		src++;
		dst++;
	}

	if (s < dst && *(dst - 1) == sep)
		*(dst - 1) = '\0';
	else if (src != dst)
		*dst = '\0';

	return s;
}


/*------------------------------------------------------------------------*/
/** 相対パスを絶対パスに展開する
 *
 * @param s			[in] 相対パス（C文字列）
 *
 * @return			絶対パス（文字列オブジェクト）
 *
 * @note			~, ~user はホームディレクトリに展開される
 */

newtRef NewtExpandPath(const char *	s)
{
    newtRefVar	r = kNewtRefUnbind;
	char *  subdir = NULL;
	char *  dir = NULL;
	char *  wd = NULL;
	char	sep;

	sep = NewtGetFileSeparator();

	if (*s == sep)
	{
		dir = (char *)s;
	}
#ifdef __WIN32__
	else if (isalpha(*s) && s[1] == ':')
	{
		dir = (char *)s;
	}
#endif
	else if (*s == '~')
	{
		dir = NewtGetHomeDir(s, &subdir);

		if (subdir != NULL && subdir[1])
			subdir++;
		else
			subdir = NULL;
	}
	else
	{
		subdir = (char *)s;
	}

	if (dir == NULL)
#ifdef HAVE_GETCWD
		dir = wd = getcwd(NULL, 0);
#else
		dir = "";
#endif /* HAVE_GETCWD */

	if (subdir != NULL)
	{
		dir = NewtJoinPath(dir, subdir, sep);
		NewtRelToAbsPath(dir);
	}

	r = NSSTR(dir);

	if (subdir != NULL)
		free(dir);

	if (wd != NULL)
		free(wd);

	return r;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** パスからファイル名を取出す
 *
 * @param s			[in] パスへのポインタ
 * @param len		[in] パスの文字数
 *
 * @return			ファイル名
 */

char * NewtBaseName(char * s, uint32_t len)
{
	uint32_t	base = 0;
	uint32_t	i;
	char		sep;

	sep = NewtGetFileSeparator();

	for (i = 0; i < len; i++)
	{
		if (s[i] == sep)
			base = i + 1;
	}

	if (base < len)
		return (s + base);
	else
		return NULL;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** ソースファイルのコンパイル
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] コンパイルするソースファイルのパス
 *
 * @return			引数 0 の関数オブジェクト
 */

newtRef NsCompileFile(newtRefArg rcvr, newtRefArg r)
{
    char *	fname;

    if (! NewtRefIsString(r))
        return NewtThrow(kNErrNotAString, r);

    fname = NewtRefToString(r);

    return NBCCompileFile(fname, true);
}


/*------------------------------------------------------------------------*/
/** ライブラリのロード
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] ロードするライブラリのパス
 *
 * @return			動的ライブラリのデスクプリタ
 */

#ifdef HAVE_DLOPEN

newtRef NsLoadLib(newtRefArg rcvr, newtRefArg r)
{
    char *	fname;
    void *	lib;

    if (! NewtRefIsString(r))
        return NewtThrow(kNErrNotAString, r);

    fname = NewtRefToString(r);
    lib = NewtDylibInstall(fname);

    if (lib != NULL)
	{
        return NewtMakeInteger((int32_t)lib);
	}
	else
	{
		const char *  errmsg;

		errmsg = dlerror();

		if (errmsg != NULL)
		{
			NewtFprintf(stderr,"%s\n", errmsg);
		}

        return NewtThrow(kNErrDylibNotOpen, r);
	}
}

#endif /* HAVE_DLOPEN */



/*------------------------------------------------------------------------*/
/** ソースファイルのロード
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] ロードするソースファイルのパス
 *
 * @return			実行結果のオブジェクト
 */

newtRef NsLoad(newtRefArg rcvr, newtRefArg r)
{
	newtRefVar  result = kNewtRefUnbind;
    newtRefVar	fn;

    fn = NsCompileFile(rcvr, r);

    if (NewtRefIsNotNIL(fn))
        result = NVMCall(fn, 0, NULL);

    return result;
}


/*------------------------------------------------------------------------*/
/** ライブラリの要求
 *
 * @param r			[in] ロードするライブラリのシンボル文字列
 *
 * @return			ロードされたライブラリのシンボル
 *
 * @note			シンボルによりライブラリを要求する。
 *					拡張子は必要ない。適宜ライブラリパスにより検索される。
 *					一度ロードされたライブラリは読込まれない。
 *					ライブラリが見つからなくても例外は発生しない。
 */

newtRef NcRequire0(newtRefArg r)
{
	newtRefVar  newtlib;
	newtRefVar  requires;
	newtRefVar  sym;
	newtRefVar  env;

	if (NewtRefIsSymbol(r))
	{
		sym = r;
	}
	else
	{
		if (! NewtRefIsString(r))
			return NewtThrow(kNErrNotASymbol, r);

		sym = NcMakeSymbol(r);
	}

    requires = NcGetGlobalVar(NSSYM0(requires));

	if (! NewtRefIsFrame(requires))
	{
		requires = NcMakeFrame();
		NcDefGlobalVar(NSSYM0(requires), requires);
	}

	if (NewtHasSlot(requires, sym))
		return kNewtRefNIL;

	{
		newtRefVar	initObj[] = {kNewtRefUnbind, kNewtRefUnbind};
		file_ext_t	lib_exts[] = {

#ifdef HAVE_DLOPEN
/*
			{NSSTR(".dylib"),			typeDylib},
			{NSSTR(".so"),				typeDylib},
			{NSSTR(".dll"),				typeDylib},
*/
			{NSSTR(__DYLIBSUFFIX__),	typeDylib},
#endif /* HAVE_DLOPEN */

			{NSSTR(".newt"),			typeScript},
		};

		newtRefVar  lib;
		newtRefVar  dir;
		newtRefVar  patharray;
		newtRefVar  path;
		uint32_t	len;
		uint32_t	nb_suffixes;
		uint32_t	i;
		uint32_t	j;

		patharray = NewtMakeArray2(kNewtRefNIL, sizeof(initObj) / sizeof(newtRefVar), initObj);

		env = NcGetGlobalVar(NSSYM0(_ENV_));
		newtlib = NcGetVariable(env, NSSYM0(NEWTLIB));

		if (NewtRefIsNIL(newtlib))
		{
			newtRefVar	initPath[] = {NSSTR(__LIBDIR__), NSSTR(".")};
			newtlib = NewtMakeArray2(kNewtRefNIL, sizeof(initPath) / sizeof(newtRefVar), initPath);
		}

		len = NewtLength(newtlib);
		nb_suffixes = sizeof(lib_exts) / sizeof(lib_exts[0]);

		for (j = 0; j < nb_suffixes; j++)
		{
			NewtSetArraySlot(patharray, 1, lib_exts[j].ext);

			for (i = 0; i <= len; i++)
			{
				if (i == len) {
					dir = NcGetGlobalVar(NSSYM0(_EXEDIR_));
				} else {
					dir = NewtGetArraySlot(newtlib, i);
				}
				if (NewtRefIsString(dir))
				{
					NewtSetArraySlot(patharray, 0, NcJoinPath(dir, r));
					path = NcStringer(patharray);

					if (NewtFileExists(NewtRefToString(path)))
					{
#ifdef HAVE_DLOPEN
						if (lib_exts[j].type == typeDylib)
						{
							lib = NcLoadLib(path);
							NcSetSlot(requires, sym, lib);
						}
						else
#endif /* HAVE_DLOPEN */
						{
							NcSetSlot(requires, sym, path);
							NcLoad(path);
						}
	
						return sym;
					}
				}
			}
		}
	}

    return kNewtRefUnbind;
}


/*------------------------------------------------------------------------*/
/** ライブラリの要求
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] ロードするライブラリのシンボル文字列
 *
 * @return			ロードされたライブラリのシンボル
 *
 * @note			シンボルによりライブラリを要求する。
 *					拡張子は必要ない。適宜ライブラリパスにより検索される。
 *					一度ロードされたライブラリは読込まれない。
 */

newtRef NsRequire(newtRefArg rcvr, newtRefArg r)
{
	newtRefVar  result;

	result = NcRequire0(r);

	if (result == kNewtRefUnbind)
		NewtThrow(kNErrFileNotFound, r);

	return result;
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** ファイルの存在確認
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] ファイルのパス
 *
 * @retval			true	ファイルが存在する
 * @retval			false   ファイルが存在しない
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsFileExists(newtRefArg rcvr, newtRefArg r)
{
    if (! NewtRefIsString(r))
        return NewtThrow(kNErrNotAString, r);

	return NewtMakeBoolean(NewtFileExists(NewtRefToString(r)));
}


#if 0
#pragma mark -
#endif
/*------------------------------------------------------------------------*/
/** パスからディレクトリ名を取出す
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] ファイルのパス
 *
 * @return			ディレクトリ名
 */

newtRef NsDirName(newtRefArg rcvr, newtRefArg r)
{
	char *  base;
	char *  s;
	char	sep;

    if (! NewtRefIsString(r))
        return NewtThrow(kNErrNotAString, r);

	s = NewtRefToString(r);
    base = NewtBaseName(s, NewtStringLength(r));
	sep = NewtGetFileSeparator();

	if (base != NULL && s < base)
	{
		if (base - 1 != s && *(base - 1) == sep)
			base--;

		if (s < base)
			return NewtMakeString2(s, base - s, false);
	}

	return NSSTR(".");
}


/*------------------------------------------------------------------------*/
/** パスからファイル名を取出す
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] ファイルのパス
 *
 * @return			ファイル名
 *
 * @note			スクリプトからの呼出し用
 */

newtRef NsBaseName(newtRefArg rcvr, newtRefArg r)
{
	char *  base;

    if (! NewtRefIsString(r))
        return NewtThrow(kNErrNotAString, r);

    base = NewtBaseName(NewtRefToString(r), NewtStringLength(r));

	if (base != NULL)
		return NSSTR(base);
	else
		return r;
}


/*------------------------------------------------------------------------*/
/** ディレクトリ名とファイル名からパスを作成
 *
 * @param rcvr		[in] レシーバ
 * @param r1		[in] ディレクトリ名
 * @param r2		[in] ファイル名
 *
 * @return			作成されたパス
 */

newtRef NsJoinPath(newtRefArg rcvr, newtRefArg r1, newtRefArg r2)
{
	char		sep = NewtGetFileSeparator();
	newtRefVar	initObj[] = {r1, NewtMakeCharacter(sep), r2};
	newtRefVar  r;

	r = NewtMakeArray2(kNewtRefNIL, sizeof(initObj) / sizeof(newtRefVar), initObj);

	return NcStringer(r);
}


/*------------------------------------------------------------------------*/
/** 相対パスを絶対パスに展開する
 *
 * @param rcvr		[in] レシーバ
 * @param r			[in] 相対パス
 *
 * @return			絶対パス
 *
 * @note			~, ~user はホームディレクトリに展開される
 *					スクリプトからの呼出し用
 */

newtRef NsExpandPath(newtRefArg rcvr, newtRefArg r)
{
    if (! NewtRefIsString(r))
        return NewtThrow(kNErrNotAString, r);

    return NewtExpandPath(NewtRefToString(r));
}


/*------------------------------------------------------------------------*/
/** Read a file and create a binary object.
 *
 * @param rcvr		[in] receiver
 * @param r		[in] filename
 *
 * @return		binary object containing the entire file data
 */

newtRef NsLoadBinary(newtRefArg rcvr, newtRefArg r)
{
    if (NewtRefIsString(r)) {
        char *filename = NewtRefToString(r);
        FILE *f = fopen(filename, "rb");
        if (!f)
            return NewtThrow(kNErrFileNotFound, r);
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint8_t *data = (uint8_t*)malloc(size);
        int err = fread(data, size, 1, f);
        fclose(f);
        return NewtMakeBinary(NSSYM(data), data, size, false); 
    } else {
        return NewtThrow(kNErrNotAString, r);
    }
}

/*------------------------------------------------------------------------*/
/** Save a binary object to a file.
 *
 * @param rcvr		[in] receiver
 * @param r1		[in] binary object
 * @param r2		[in] filename
 *
 * @return		error code
 */

newtRef NsSaveBinary(newtRefArg rcvr, newtRefArg r1, newtRefArg r2)
{
    if (NewtRefIsBinary(r1)) {
        if (NewtRefIsString(r2)) {
            char *filename = NewtRefToString(r2);
            FILE *f = fopen(filename, "wb");
            if (!f)
                return NewtThrow(kNErrFileNotFound, r2);
            uint8_t *data = NewtRefToBinary(r1);
            int size = NewtBinaryLength(r1);
            fwrite(data, size, 1, f);
            fclose(f);
            return kNewtRefNIL;
        } else {
            return NewtThrow(kNErrNotAString, r2);
        }
    } else {
        return NewtThrow(kNErrNotABinaryObject, r1);
    }
}


