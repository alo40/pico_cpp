cmake_minimum_required(VERSION 3.13)

project(vedirect_parser_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

add_executable(test_vedirect_parser
    test_vedirect_parser.c
    ../src/vedirect_parser.c
)

target_include_directories(test_vedirect_parser PRIVATE
    ../include
)

target_compile_options(test_vedirect_parser PRIVATE
    -Wall
    -Wextra
    -Werror
)

add_test(
    NAME vedirect_parser
    COMMAND test_vedirect_parser
)
