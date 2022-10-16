cmake_minimum_required(VERSION 3.24)
project(mini-python LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(Doxygen)
if (DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile
            ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doxygen
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
endif ()

include_directories(src)

add_executable(
        mini-python
        src/main.cpp
)
