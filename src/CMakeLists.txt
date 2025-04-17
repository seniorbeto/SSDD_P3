cmake_minimum_required(VERSION 3.10)
project(SSDD_P2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
add_compile_options( -pthread -fPIC)

include_directories(/usr/include/tirpc)

add_library(claves SHARED proxy-rpc.c rpc_api_clnt.c rpc_api_xdr.c)
target_link_libraries(claves PRIVATE pthread rt tirpc)

add_executable(servidor-rpc servidor-rpc.c common/claves.c rpc_api_svc.c rpc_api_xdr.c)
target_link_libraries(servidor-rpc PRIVATE pthread rt tirpc)

add_executable(app-cliente app-cliente.c)
target_link_libraries(app-cliente PRIVATE claves pthread rt)

# Esto sirve para indicar al cliente que busque las librerías en el directorio actual
set_target_properties(app-cliente PROPERTIES BUILD_RPATH ".")

# Esto es para desactivar los warnings de los ficheros generados por rpcgen
set_source_files_properties(
        rpc_api_clnt.c rpc_api_xdr.c proxy-rpc.c rpc_api_svc.c
        PROPERTIES COMPILE_FLAGS "-w"
)
