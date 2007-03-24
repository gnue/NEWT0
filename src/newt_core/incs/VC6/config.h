/*------------------------------------------------------------------------*/
/**
 * @file	config.h
 * @brief   コンフィグ設定
 *
 * @author  M.Nukui
 * @date	2005-04-30
 *
 * Copyright (C) 2005 M.Nukui All rights reserved.
 */


#ifndef	CONFIG_H
#define	CONFIG_H

/* マクロ定義 */
#undef HAVE_INTTYPES_H
#define HAVE_STDINT_H 1
#define HAVE_MEMORY_H 1
#define HAVE_UNISTD_H 1
#define HAVE_ENDIAN_H 1
#undef HAVE_MACHINE_ENDIAN_H
#undef HAVE_TERMIOS_H 

#define HAVE_LIBICONV 1
#undef HAVE_DLOPEN
#undef HAVE_MMAP
#undef HAVE_CHDIR
#undef HAVE_GETCWD
#undef HAVE_GETPWNAM

#define __PLATFORM__ "WIN32_MSVC6"
#define __DYLIBSUFFIX__ ".dll"
#define __LIBDIR__ "%SystemRoot%\\system32\\"

#undef __IPOD__

#define strcasecmp stricmp
#define strncasecmp strnicmp
#define vsnprintf _vsnprintf

#endif /* CONFIG_H */
