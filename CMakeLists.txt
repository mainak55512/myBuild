cmake_minimum_required(VERSION 3.9...3.12)
set(CMAKE_C_COMPILER "/usr/bin/clang-18" CACHE PATH "" FORCE)
project(myBuild)

# set(CMAKE_C_FLAGS "-g")

# Add .a file path here
set(LIBS 
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/lib/libyyjson.a
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/lib/libarena.a
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/lib/libcstring.a
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/lib/libccontainer.a
)

# Add include directory path here
set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/include
)

# Create imported targets for each library
foreach(lib_path ${LIBS})

    # Extract library name from path
    get_filename_component(lib_name ${lib_path} NAME_WE)

    # Remove 'lib' prefix if present
    string(REGEX REPLACE "^lib" "" lib_name ${lib_name})
    
    # Create unique target name
    set(target_name "lib-${lib_name}_local")
    
    # Create imported library target
    add_library(${target_name} STATIC IMPORTED)
    
    # Set the location of the static archive
    set_target_properties(${target_name} PROPERTIES
        IMPORTED_LOCATION "${lib_path}"
    )
    
    # Store target name for later linking
    list(APPEND IMPORTED_TARGETS ${target_name})

endforeach()

add_executable(${PROJECT_NAME} main.c)

# Add all include directories to the executable
target_include_directories(${PROJECT_NAME} PRIVATE ${HEADERS} ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link all imported libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${IMPORTED_TARGETS})
