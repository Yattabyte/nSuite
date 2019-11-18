####################
### nSuite Tools ###
####################
cmake_minimum_required(VERSION 3.10)
project(nSuiteTools VERSION 1.3.2 DESCRIPTION "Directory and buffer compression & diffing tools.")

# Get dependency directory locations from the user
set(LZ4_DIR "" CACHE PATH "LZ4 root directory")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
set_property(GLOBAL PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)app")

# Show option to generate docs if doxygen is found
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
	option(GENERATE_DOCS "Generate documentation using Doxygen" true)
	if (GENERATE_DOCS)		
		set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/docs/)
		set(DOXYGEN_MARKDOWN_SUPPORT YES)
		set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
		set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${PROJECT_SOURCE_DIR}/README.md)
		doxygen_add_docs(Documentation ${PROJECT_SOURCE_DIR} COMMENT "Generate docs")
	endif(GENERATE_DOCS)
endif(DOXYGEN_FOUND)

# add all sub-projects and plugins here
add_subdirectory( "src/Library" )
add_subdirectory( "src/Installer" )
add_subdirectory( "src/Uninstaller" )
add_subdirectory( "src/Unpacker" )
add_subdirectory( "src/Updater" )
add_subdirectory( "src/Wizard" )

# Visual studio specific setting: make nSuite the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT nSuite-Static)
# Set Global Properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER Other)