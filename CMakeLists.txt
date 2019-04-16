####################
### nSuite Tools ###
####################
cmake_minimum_required(VERSION 3.0)
project(nStallerTools)

# Get dependency directory locations from the user
set (LZ4_DIR "" CACHE PATH "LZ4 root directory")
set (CORE_DIR "${PROJECT_SOURCE_DIR}/src")
set (PROJECT_BIN ${CMAKE_SOURCE_DIR})
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

# Include directories of dependencies for entire project
include_directories( 
	${LZ4_DIR}/lib/
	${CORE_DIR}		
)

# Add libraries common throughout entire project
link_libraries(	
	debug		${LZ4_DIR}/x64_Debug/liblz4_static.lib
	optimized	${LZ4_DIR}/x64_Release/liblz4_static.lib
	Gdiplus.lib
)

# add all sub-projects and plugins here
add_subdirectory( "src/unStaller" )
add_subdirectory( "src/nStaller" )
add_subdirectory( "src/nUpdater" )
add_subdirectory( "src/nSuite" )

# Visual studio specific setting: make nSuite the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT nSuite)
 
# Enable folder structure
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# ALL_BUILD, ZERO_CHECK, and other build functions' folder
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER Build_Functions)