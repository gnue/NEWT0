cmake_minimum_required(VERSION 3.10)

project("NEWT/0")

set (sources
	src/newt_core/NewtBC.c
	src/newt_core/NewtEnv.c
	src/newt_core/NewtFile.c
	src/newt_core/NewtFns.c
	src/newt_core/NewtGC.c
	src/newt_core/NewtIconv.c
	src/newt_core/NewtIO.c
	src/newt_core/NewtMem.c
	src/newt_core/NewtNSOF.c
	src/newt_core/NewtPkg.c
	src/newt_core/NewtObj.c
	src/newt_core/NewtParser.c
	src/newt_core/NewtPrint.c
	src/newt_core/NewtStr.c
	src/newt_core/NewtVM.c
	src/parser/lookup_words.c
	src/utils/endian_utils.c
	${CMAKE_CURRENT_BINARY_DIR}/newt.l.c
	${CMAKE_CURRENT_BINARY_DIR}/y.tab.c
	src/parser/newt.l
	src/parser/newt.y
)

set (headers
	src/version.h
	src/newt_core/incs/NewtBC.h
	src/newt_core/incs/NewtConf.h
	src/newt_core/incs/NewtCore.h
	src/newt_core/incs/NewtEnv.h
	src/newt_core/incs/NewtErrs.h
	src/newt_core/incs/NewtFile.h
	src/newt_core/incs/NewtFns.h
	src/newt_core/incs/NewtGC.h
	src/newt_core/incs/NewtIconv.h
	src/newt_core/incs/NewtIO.h
	src/newt_core/incs/NewtLib.h
	src/newt_core/incs/NewtMem.h
	src/newt_core/incs/NewtNSOF.h
	src/newt_core/incs/NewtObj.h
	src/newt_core/incs/NewtParser.h
	src/newt_core/incs/NewtPkg.h
	src/newt_core/incs/NewtPrint.h
	src/newt_core/incs/NewtStr.h
	src/newt_core/incs/NewtType.h
	src/newt_core/incs/NewtVM.h
	src/newt_core/incs/platform.h
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#
# ---- lex and yacc -----------------------------------------------------------
#

find_package(FLEX REQUIRED)
flex_target(lexer src/parser/newt.l ${CMAKE_CURRENT_BINARY_DIR}/newt.l.c)

find_package(BISON REQUIRED)
bison_target(parser src/parser/newt.y ${CMAKE_CURRENT_BINARY_DIR}/y.tab.c)

add_flex_bison_dependency(lexer parser)

#
# ---- newt library ---------------------------------------------------------
#

find_library(ICONV iconv)

add_library(libnewt STATIC
	${sources}
	${headers}
)

target_include_directories(libnewt PUBLIC
	src/newt_core/incs/darwin
	src/newt_core/incs/
	src/parser/
	src/
)

target_link_libraries(libnewt PUBLIC
	${ICONV}
)

set_target_properties(libnewt
	PROPERTIES OUTPUT_NAME newt
)

install(TARGETS libnewt
	ARCHIVE DESTINATION ${NEWT_INSTALL_PREFIX}lib
)

install(DIRECTORY src/newt_core/incs/
	DESTINATION ${NEWT_INSTALL_PREFIX}include/newt
)
