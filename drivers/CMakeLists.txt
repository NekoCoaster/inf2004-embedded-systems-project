cmake_minimum_required(VERSION 3.12)

set(PICO_SDK_FETCH_FROM_GIT on)

# Notice that it references to the earler 2 Cmake
include(pico_sdk_import.cmake)
include(pico_extras_import_optional.cmake)

# Replace `pico_proj` with whatever, its the definition for project name.

project(pico_proj C CXX ASM) # Technically can remove CXX and ASM

set(CMAKE_BUILD_TYPE Debug)
set(PICO_BOARD pico_w)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17) # Technically can remove this

pico_sdk_init()

#Define a function to add subdirectories recursively, ignoring folders with the following conditions:
# - The folder is named 'build'
# - The folder starts with a '.'
# - The folder does not contain a CMakeLists.txt file

function(add_subdirectories_recursively dir)
    file(GLOB children RELATIVE ${dir} ${dir}/*)
    foreach(child ${children})
        if(IS_DIRECTORY ${dir}/${child} AND NOT ${child} STREQUAL "build" AND NOT child MATCHES "^\\.") # Check if it's not the 'build' folder
            # Check if there's a CMakeLists.txt file in the directory
            file(GLOB cmakelists ${dir}/${child}/CMakeLists.txt)
            if(cmakelists)
                add_subdirectory(${dir}/${child})
                add_subdirectories_recursively(${dir}/${child}) # Recursively add subdirectories
            endif()
        endif()
    endforeach()
endfunction()

# Call the function to start adding subdirectories from the root directory
add_subdirectories_recursively(${CMAKE_SOURCE_DIR})