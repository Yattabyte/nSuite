####################
### nSuite Tools ###
####################
cmake_minimum_required(VERSION 3.0)
project(nStallerTools VERSION 1.2.0 DESCRIPTION "Directory Distribution Tools")

# Get dependency directory locations from the user
set (LZ4_DIR "" CACHE PATH "LZ4 root directory")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
set_property(GLOBAL PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)app")


# add all sub-projects and plugins here
add_subdirectory( "src/nSuite" )
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