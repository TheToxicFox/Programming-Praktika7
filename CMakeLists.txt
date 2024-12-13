cmake_minimum_required(VERSION 3.10)

project(Praktika7 VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_executable(Praktika7_guess_number2 guess_number2.c)

target_compile_options(Praktika7_guess_number2 PRIVATE -Wall)
    
