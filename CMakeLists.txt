cmake_minimum_required(VERSION 3.7)
project(ElaboratoC)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES src/main.c inc/matrix.h src/matrix.c inc/utils.h src/utils.c)
add_executable(ElaboratoC ${SOURCE_FILES})