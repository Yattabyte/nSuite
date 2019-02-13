cmake_minimum_required(VERSION 3.0)
# Project Name: FreeStaller
project(FreeStaller)

# Get dependency directory locations from the user
set (CORE_DIR "${PROJECT_SOURCE_DIR}/src")
set (PROJECT_BIN ${CMAKE_SOURCE_DIR})
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
					
# add all sub-projects and plugins here
add_subdirectory( "src/compressor" )
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT compressor)
add_subdirectory( "src/decompressor" )
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT decompressor)
 
# Enable folder structure
set_property (GLOBAL PROPERTY USE_FOLDERS ON)
# ALL_BUILD, ZERO_CHECK, and other build functions' folder
set_property (GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER Build_Functions)