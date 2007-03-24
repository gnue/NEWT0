# Microsoft Developer Studio Project File - Name="newt_app" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=newt_app - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "newt_app.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "newt_app.mak" CFG="newt_app - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "newt_app - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "newt_app - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "newt_app - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../.." /I "../../src" /I "../../src/newt_core/incs" /I "../../src/newt_core/incs/VC6" /I "../../src/parser" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"..\..\build\newt.exe"

!ELSEIF  "$(CFG)" == "newt_app - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../src" /I "../../src/newt_core/incs" /I "../../src/newt_core/incs/VC6" /I "../../src/parser" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# SUBTRACT CPP /X
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"..\..\build\newtd.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "newt_app - Win32 Release"
# Name "newt_app - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter ""
# Begin Group "newt_core"

# PROP Default_Filter ""
# Begin Group "incs"

# PROP Default_Filter ""
# Begin Group "VC6"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\newt_core\incs\VC6\config.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\VC6\dlfcn.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\VC6\endian.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\VC6\iconv.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\VC6\NewtConf.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\VC6\stdbool.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\VC6\stdint.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\VC6\unistd.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtBC.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtCore.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtEnv.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtErrs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtFile.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtFns.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtGC.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtIconv.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtIO.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtLib.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtMem.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtNSOF.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtObj.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtParser.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtPrint.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtStr.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtType.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\NewtVM.h
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\incs\platform.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\newt_core\NewtBC.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtEnv.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtFile.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtFns.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtGC.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtIconv.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtIO.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtMem.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtNSOF.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtObj.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtParser.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtPrint.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtStr.c
# End Source File
# Begin Source File

SOURCE=..\..\src\newt_core\NewtVM.c
# End Source File
# End Group
# Begin Group "parser"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\parser\lookup_words.c
# End Source File
# Begin Source File

SOURCE=..\..\src\parser\lookup_words.h
# End Source File
# Begin Source File

SOURCE=..\..\src\parser\newt.l

!IF  "$(CFG)" == "newt_app - Win32 Release"

# Begin Custom Build - Performing Lex Build Step on $(InputPath)
InputPath=..\..\src\parser\newt.l

"../../build/lex.yy.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\cygwin\bin\flex -o../../build/lex.yy.c ../../src/parser/newt.l

# End Custom Build

!ELSEIF  "$(CFG)" == "newt_app - Win32 Debug"

# Begin Custom Build - Performing Lex Build Step on $(InputPath)
InputPath=..\..\src\parser\newt.l

"../../build/lex.yy.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\cygwin\bin\flex -o../../build/lex.yy.c ../../src/parser/newt.l

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\parser\newt.y

!IF  "$(CFG)" == "newt_app - Win32 Release"

# Begin Custom Build - Performing Bison Build Step on $(InputPath)
InputPath=..\..\src\parser\newt.y

BuildCmds= \
	c:\cygwin\bin\bison -d -o ../../build/y.tab.c ../../src/parser/newt.y

"../../build/y.tab.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"../../build/y.tab.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "newt_app - Win32 Debug"

# Begin Custom Build - Performing Bison Build Step on $(InputPath)
InputPath=..\..\src\parser\newt.y

BuildCmds= \
	c:\cygwin\bin\bison -d -o ../../build/y.tab.c ../../src/parser/newt.y

"../../build/y.tab.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"../../build/y.tab.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\src\parser\yacc.h
# End Source File
# End Group
# Begin Group "utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\utils\endian_utils.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\endian_utils.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\main.c
# End Source File
# Begin Source File

SOURCE=..\..\src\version.h
# End Source File
# End Group
# Begin Group "build"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\build\lex.yy.c
# End Source File
# Begin Source File

SOURCE=..\..\build\y.tab.c
# End Source File
# End Group
# End Target
# End Project
