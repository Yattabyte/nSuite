#############
### Yatta ###
#############
set (Module yatta)

# Configure and acquire files
configure_file(Resource.h.in Resource.h)
set(FILES
	# Header files
	Buffer.h
	Directory.h
	nSuite.h
	StringConversions.h
	Log.h
	Progress.h
	Resource.h
	Threader.h
	lz4/lz4.h
	
	# Source files
	Buffer.cpp
	Directory.cpp
	lz4/lz4.c
)

# Create Library using the supplied files
add_library(${Module} STATIC ${FILES})
target_include_directories(${Module} 
	PUBLIC include
)

# Set all project settings
target_compile_Definitions(${Module} PRIVATE $<$<CONFIG:DEBUG>:DEBUG>)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${Module})
set_target_properties(${Module} PROPERTIES 	
	VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)app"
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	PDB_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	VERSION ${PROJECT_VERSION}	
)