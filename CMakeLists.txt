cmake_minimum_required(VERSION 3.27)
project(autolooper C)

set(CMAKE_C_STANDARD 11)

add_executable(autolooper
        main.c
        parse_wav.c)
