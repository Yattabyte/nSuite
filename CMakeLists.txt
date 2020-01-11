#############
### Yatta ###
#############
cmake_minimum_required(VERSION 3.9.2)
project(Yatta VERSION 1.6.0  DESCRIPTION "Library for compressing and diffing buffers and files.")
include(ExternalProject)


# Set consistent compiler flags for our project and our dependencies
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CXX_EXTENSIONS YES)
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Oi /Ot /GF /Qpar /std:c++17 /Zc:rvalueCast /W4 /WX /permissive-")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Gy /MDd /JMC")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /GL /MD")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG:incremental")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FASTLINK /INCREMENTAL:NO")
	set_property(GLOBAL PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)app")
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER Other)
elseif(CMAKE_COMPILER_IS_GNUCC)
	option(ENABLE_COVERAGE "Enable code coverage reporting for gcc/clang" FALSE)
    add_compile_options("-Wall" "-Wextra" "-Werror" "$<$<CONFIG:RELEASE>:-O3>")
	if(ENABLE_COVERAGE)
		add_compile_options(--coverage -O0)
		add_link_options(--coverage)
	endif()
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # No Clang specific options
    else()
        # No GCC specific options
    endif()
endif()
find_package (Threads REQUIRED)


# Add our own source files
add_subdirectory( src )


# Optionally build unit tests
enable_testing()
option(UNIT_TESTS "Build Unit Tests" TRUE)
if (UNIT_TESTS)
	add_subdirectory( tests/UnitTests )
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT UnitTests)
endif (UNIT_TESTS)


# Optionally build examples
option(EXAMPLES "Build Examples" FALSE)
if (EXAMPLES)
	#add_subdirectory( examples/Unpacker )
	#add_subdirectory( examples/Updater )
	#add_subdirectory( examples/Installer )
	#add_subdirectory( examples/Uninstaller )
	#add_subdirectory( examples/Wizard )
endif (EXAMPLES)


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