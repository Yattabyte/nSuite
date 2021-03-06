#################
### Installer ###
#################
set (Module Installer)

# Configure and acquire files
configure_file(Resource.hpp.in Resource.hpp)
set(FILES 	
	# Header Files
	Installer.hpp
	Resource.hpp
	Screens/Agreement.hpp
	Screens/Directory.hpp
	Screens/Fail.hpp
	Screens/Finish.hpp
	Screens/Screen.hpp
	Screens/Install.hpp
	Screens/Welcome.hpp
	
	# Source Files
	Installer.rc
	Installer.cpp
	Screens/Agreement.cpp
	Screens/Directory.cpp
	Screens/Fail.cpp
	Screens/Finish.cpp
	Screens/Install.cpp
	Screens/Welcome.cpp
)

# Create Executable
add_executable(${Module} ${FILES})

# Add library dependencies
target_include_directories(${Module} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(${Module} yatta Uninstaller)
target_link_libraries(${Module} PUBLIC yatta)
target_link_libraries(${Module} PRIVATE
	"Comctl32.lib" 
	"propsys.lib" 
	"Shlwapi.lib"
	"Gdiplus.lib"
)

# Set all project settings
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
set_target_properties(${Module} PROPERTIES 	
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	PDB_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)app"	
	VERSION ${PROJECT_VERSION}
	LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
)