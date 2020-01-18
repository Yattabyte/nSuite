##############
### Wizard ###
##############
set (Module Wizard)

# Create Executable
add_executable(${Module}	
	# Header files
	include/Commands/DiffCommand.h
	include/Commands/InstallerCommand.h
	include/Commands/PackagerCommand.h
	include/Commands/PackCommand.h
	include/Commands/PatchCommand.h
	include/Commands/UnpackCommand.h
	
	# Source Files
	Wizard.rc
	src/Wizard.cpp
	src/Commands/DiffCommand.cpp
	src/Commands/InstallerCommand.cpp
	src/Commands/PackagerCommand.cpp
	src/Commands/PackCommand.cpp
	src/Commands/PatchCommand.cpp
	src/Commands/UnpackCommand.cpp
)

# Add library dependencies
add_dependencies(${Module} nSuite-Static Installer Unpacker)
target_link_libraries(${Module} PUBLIC nSuite-Static)
target_include_directories(${Module} PRIVATE include)

# Set all project settings
set_target_properties(${Module} PROPERTIES 	
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	PDB_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)app"
	VERSION ${PROJECT_VERSION}
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