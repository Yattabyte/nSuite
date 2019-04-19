################
### Unpacker ###
################
set (Module Unpacker)

# Get source files
file (GLOB_RECURSE ROOT RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp" "*.c" "*.h" "*.rc")
# Mimic the file-folder structure
foreach(source IN LISTS ROOT)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

# Add source files
add_executable(${Module} 	
	${ROOT} 
	${CORE_DIR}/Common.h	
	${CORE_DIR}/Resource.h
	${CORE_DIR}/Threader.h 					
	${CORE_DIR}/Instructions.h 							
	${CORE_DIR}/Instructions.cpp 	
	${CORE_DIR}/BufferTools.h 
	${CORE_DIR}/BufferTools.cpp
	${CORE_DIR}/DirectoryTools.h 
	${CORE_DIR}/DirectoryTools.cpp	
	${CORE_DIR}/TaskLogger.h	
)

# Set windows/visual studio settings
target_compile_Definitions(${Module} PRIVATE $<$<CONFIG:DEBUG>:DEBUG>)
set_target_properties(${Module} PROPERTIES 	
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	PDB_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)app"
)

# Force highest c++ version supported
if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
		set_target_properties(${Module} PROPERTIES CXX_STANDARD 17)
		set_target_properties(${Module} PROPERTIES CXX_STANDARD_REQUIRED ON)
    endif()
endif()