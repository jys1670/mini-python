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
        src/lexer.cpp
        src/lexer.h
        src/main.cpp
        src/parse.cpp
        src/parse.h
        src/runtime.cpp
        src/runtime.h
        src/statement.cpp
        src/statement.h
)

add_executable(
        unit-tests
        src/lexer.cpp
        src/lexer.h
        src/parse.cpp
        src/parse.h
        src/runtime.cpp
        src/runtime.h
        src/statement.cpp
        src/statement.h
        tests/lexer_test_open.cpp
        tests/main.cpp
        tests/parse_test.cpp
        tests/runtime_test.cpp
        tests/statement_test.cpp
        tests/test_runner_p.h
)
