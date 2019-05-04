######################
### nSuite Library ###
######################
set (ModuleStatic nSuite-Static)
set (ModuleShared nSuite-Shared)

configure_file(Resource.h.in include/Resource.h)

# Group Files
set(FILES
	# Source files
	src/Buffer.cpp
	src/nSuite.cpp
	src/Instructions.cpp	
	
	# Header files
	include/Buffer.h
	include/nSuite.h
	include/Instructions.h
	include/StringConversions.h
	include/Log.h
	include/Progress.h
	include/Resource.h
	include/Threader.h
)


# Create Library
add_library(${ModuleStatic} STATIC ${FILES})
add_library(${ModuleShared} SHARED ${FILES})

# Add library dependencies
target_link_libraries(${ModuleStatic} PRIVATE 
	debug		${LZ4_DIR}/x64_Debug/liblz4_static.lib
	optimized	${LZ4_DIR}/x64_Release/liblz4_static.lib
)
target_link_libraries(${ModuleShared} PRIVATE 
	debug		${LZ4_DIR}/x64_Debug/liblz4_static.lib
	optimized	${LZ4_DIR}/x64_Release/liblz4_static.lib
)
target_include_directories(${ModuleStatic} 
	PUBLIC include 
	PRIVATE ${LZ4_DIR}/lib
)
target_include_directories(${ModuleShared} 
	PUBLIC include 
	PRIVATE ${LZ4_DIR}/lib
)

# Set all project settings
set_target_properties(${ModuleStatic} PROPERTIES 	
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	PDB_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	VERSION ${PROJECT_VERSION}	
)
set_target_properties(${ModuleShared} PROPERTIES 	
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
		set_target_properties(${ModuleStatic} PROPERTIES 
			CXX_STANDARD 17
			CXX_STANDARD_REQUIRED ON
		)
		set_target_properties(${ModuleShared} PROPERTIES 
			CXX_STANDARD 17
			CXX_STANDARD_REQUIRED ON
		)
    endif()
endif()