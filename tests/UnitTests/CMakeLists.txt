##################
### Unit Tests ###
##################
set (Module UnitTests)

# Configure and acquire files
set(FILES main.cpp)

# Create Library using the supplied files
add_executable(${Module} ${FILES})

# Add library dependencies
add_dependencies(${Module} yatta UnitTests)
target_link_libraries(${Module} PUBLIC yatta)

# Set all project settings
target_compile_Definitions(${Module} PRIVATE $<$<CONFIG:DEBUG>:DEBUG>)
set_target_properties(${Module} PROPERTIES 	
	VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)app"
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	PDB_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	VERSION ${PROJECT_VERSION}	
)