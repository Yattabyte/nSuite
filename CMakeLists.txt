#################
### Yatta lib ###
##################
cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0079 NEW)
include(ExternalProject)
project(YattaLib VERSION 1.4.0 DESCRIPTION "Directory and buffer compression & diffing tools.")


# Set consistent compiler flags for our project and our dependencies
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Oi /Ot /GF /Qpar /std:c++17 /Zc:rvalueCast /W3 /WX-")
set(CMAKE_CXX_FLAGS_SILENT "${CMAKE_CXX_FLAGS} /MP /Oi /Ot /GF /Qpar /std:c++17 /Zc:rvalueCast- /W0 /WX-")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Gy /MDd /JMC")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /GL /MD")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG:incremental")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FASTLINK /INCREMENTAL:NO")
set_property(GLOBAL PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)app")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER Other)


# Add our own source files
add_subdirectory( src )


#################
# DOXYGEN CHECK #
#################
find_package(Doxygen QUIET)
if (DOXYGEN_FOUND)
	option(GENERATE_DOCS "Generate documentation using Doxygen" true)
	if (GENERATE_DOCS)		
		set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/docs/)
		set(DOXYGEN_MARKDOWN_SUPPORT YES)
		set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
		set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
		set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${PROJECT_SOURCE_DIR}/README.md)
		set(DOXYGEN_EXCLUDE ${PROJECT_SOURCE_DIR}/external/)
		doxygen_add_docs(Documentation ${PROJECT_SOURCE_DIR} COMMENT "Generate docs")
	endif (GENERATE_DOCS)
endif (DOXYGEN_FOUND)