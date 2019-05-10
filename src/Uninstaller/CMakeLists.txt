###################
### Uninstaller ###
###################
set (Module Uninstaller)

# Create Executable
add_executable(${Module}
	# Header Files
	include/Uninstaller.h
	include/Screens/Fail.h
	include/Screens/Finish.h
	include/Screens/Screen.h
	include/Screens/Uninstall.h
	include/Screens/Welcome.h
	
	# Source Files
	Uninstaller.rc
	src/Uninstaller.cpp
	src/Screens/Fail.cpp
	src/Screens/Finish.cpp
	src/Screens/Uninstall.cpp
	src/Screens/Welcome.cpp 
)

# Add library dependencies
add_dependencies(${Module} nSuite-Static)
target_link_libraries(${Module} PUBLIC nSuite-Static)
target_link_libraries(${Module} PRIVATE
	"Comctl32.lib" 
	"propsys.lib" 
	"Shlwapi.lib"
	"Gdiplus.lib"
)
target_include_directories(${Module} PRIVATE include)

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

# Force highest c++ version supported
if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
		set_target_properties(${Module} PROPERTIES 
			CXX_STANDARD 17
			CXX_STANDARD_REQUIRED ON
		)
    endif()
endif()