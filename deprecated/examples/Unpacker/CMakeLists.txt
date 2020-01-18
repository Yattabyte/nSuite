################
### Unpacker ###
################
set (Module Unpacker)

# Create Executable
add_executable(${Module} 
	Unpacker.rc
	Unpacker.cpp
)

# Add library dependencies
add_dependencies(${Module} yatta)
target_link_libraries(${Module} PUBLIC yatta)

# Set all project settings
set_target_properties(${Module} PROPERTIES 	
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	PDB_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
	VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)app"
	VERSION ${PROJECT_VERSION}
)