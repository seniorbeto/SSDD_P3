cmake_minimum_required(VERSION 3.10)
project(SSDD_P2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
add_compile_options(-Wall -Wextra -Werror -pedantic -pedantic-errors -Wconversion -Wsign-conversion -pthread -fPIC)

add_library(claves SHARED proxy-sock.c common/lines.c)
add_library(encript SHARED common/encript.c)
add_library(serialitation SHARED common/serialitation.c)

add_executable(servidor-sock servidor-sock.c common/claves.c common/lines.c)
target_link_libraries(servidor-sock PRIVATE pthread rt encript serialitation)

add_executable(app-cliente app-cliente.c)
target_link_libraries(app-cliente PRIVATE claves pthread rt encript serialitation)

# Esto sirve para indicar al cliente que busque las librerías en el directorio actual
set_target_properties(app-cliente PROPERTIES BUILD_RPATH ".")
