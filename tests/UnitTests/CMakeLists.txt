##################
### Unit Tests ###
##################
set(Module UnitTests)

# Configure and acquire files
set(FILES main.cpp)

# Create Library using the supplied files
add_executable(${Module} ${FILES})

# Add library dependencies
add_dependencies(${Module} yatta)
target_compile_features(${Module} PRIVATE cxx_std_17)
target_link_libraries(${Module} PUBLIC ${CMAKE_THREAD_LIBS_INIT} yatta)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	target_link_libraries(${Module} PRIVATE $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>:c++experimental>)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_link_libraries(${Module} PRIVATE stdc++fs)
endif()

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

add_test(NAME LibraryTest COMMAND ${Module} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/app/)