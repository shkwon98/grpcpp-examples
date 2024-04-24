cmake_minimum_required( VERSION 3.10.2 )
project( grpc_example_cpp
         LANGUAGES CXX )

# Command-line arguments:
set( CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE )
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# Dependencies:
find_package( protobuf REQUIRED )
find_package( gRPC REQUIRED )
get_target_property( grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION )

### protos API LIBRARY ###
set( protos_OUTPUT_DIR ${CMAKE_BINARY_DIR}/protos )
file( COPY ${CMAKE_SOURCE_DIR}/protos/
    DESTINATION ${protos_OUTPUT_DIR} )
file( GLOB_RECURSE robl_protos_files CONFIGURE_DEPENDS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${protos_OUTPUT_DIR}/*.proto")

add_library( robl_api ${robl_protos_files} )
target_link_libraries( robl_api
    PUBLIC  ${PROTOBUF_LIBRARIES}
            gRPC::grpc++ )
target_include_directories( robl_api
    PUBLIC ${protos_OUTPUT_DIR} )
protobuf_generate( TARGET robl_api
    LANGUAGE       cpp
    IMPORT_DIRS    ${protos_OUTPUT_DIR} 
    PROTOC_OUT_DIR ${protos_OUTPUT_DIR} )
protobuf_generate( TARGET robl_api
    LANGUAGE       grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN         "protoc-gen-grpc=${grpc_cpp_plugin_location}"
    IMPORT_DIRS    ${protos_OUTPUT_DIR} 
    PROTOC_OUT_DIR ${protos_OUTPUT_DIR} )
add_library( robl::api ALIAS robl_api )

### SUBDIRECTORIES ###
add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/src/client )
add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/src/server )