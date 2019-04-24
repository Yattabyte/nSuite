######################
### nSuite Library ###
######################
set (Module nSuite)

# Create Library
add_library(${Module}
	# Source files
	src/BufferTools.cpp
	src/DirectoryTools.cpp
	src/Instructions.cpp	
	
	# Header files
	include/BufferTools.h
	include/Common.h
	include/DirectoryTools.h
	include/Instructions.h	
	include/Resource.h
	include/TaskLogger.h
	include/Threader.h	
)

# Add library dependencies
target_link_libraries(${Module} PRIVATE 
	debug		${LZ4_DIR}/x64_Debug/liblz4_static.lib
	optimized	${LZ4_DIR}/x64_Release/liblz4_static.lib
)
target_include_directories(${Module} 
	PUBLIC include 
	PRIVATE ${LZ4_DIR}/lib
)

# Set all project settings
set_target_properties(${Module} PROPERTIES 	
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	PDB_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
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