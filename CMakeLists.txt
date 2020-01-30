#############
### Yatta ###
#############
cmake_minimum_required(VERSION 3.9.2)
project(Yatta VERSION 1.11.6 DESCRIPTION "Library for compressing and diffing buffers and files.")
include(ExternalProject)

# Set mandatory requirements
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CXX_EXTENSIONS YES)
set(USE32BIT FALSE)
set(CMAKE_TOOLCHAIN_FILE 64bit.toolchain)
option(EXAMPLES "Build Examples" OFF)
option(BUILD_TESTING "Build Unit Tests" ON)
option(CODE_COVERAGE "Enable code coverage reporting for GCC/Clang" OFF)
option(STATIC_ANALYSIS "Enable static code analysis using GCC" OFF)


# Set compilation flags per-compiler
if(MSVC)
    set(CMAKE_GENERATOR_PLATFORM x64)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Oi /Ot /GF /Qpar /std:c++17 /Zc:rvalueCast /W4 /WX /permissive-")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Gy /MDd /JMC")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /GL /MD")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG:incremental")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FASTLINK /INCREMENTAL:NO")
    set_property(GLOBAL PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)app")
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER Other)
elseif(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wshadow -Wno-unused-value -Wunused-variable -Wunused-parameter -Wunused-function -Wno-system-headers -Wno-deprecated -Woverloaded-virtual -Wwrite-strings -Wno-invalid-token-paste -Wno-unknown-pragmas")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    #if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    #else()
    #endif()
endif()
find_package(Threads REQUIRED)


# Add our own source files
add_subdirectory(src)


# Optionally build examples
if(EXAMPLES)
    #add_subdirectory( examples/Unpacker )
    #add_subdirectory( examples/Updater )
    #add_subdirectory( examples/Installer )
    #add_subdirectory( examples/Uninstaller )
    #add_subdirectory( examples/Wizard )
endif()


# Optionally build unit tests
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)

    # Optionally perform static code analysis tests
    if (STATIC_ANALYSIS)
        include(CTest)
        find_program(MEMORYCHECK_COMMAND valgrind)
        if (CTEST_MEMORYCHECK_COMMAND)
            set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full")
        endif()
    endif()
endif()


#################
# DOXYGEN CHECK #
#################
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    option(GENERATE_DOCS "Generate documentation using Doxygen" true)
    if(GENERATE_DOCS)
        set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/docs/)
        set(DOXYGEN_MARKDOWN_SUPPORT YES)
        set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
        set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
        set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${PROJECT_SOURCE_DIR}/README.md)
        set(DOXYGEN_EXCLUDE ${PROJECT_SOURCE_DIR}/external/ ${PROJECT_SOURCE_DIR}/src/lz4)
        doxygen_add_docs(Documentation ${PROJECT_SOURCE_DIR} COMMENT "Generate docs")
    endif()
endif()