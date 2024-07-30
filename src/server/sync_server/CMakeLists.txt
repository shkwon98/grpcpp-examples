project(sync_server
    LANGUAGES CXX)

file(GLOB_RECURSE SOURCES *.cpp)
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
    PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME}
    PRIVATE robl::api)
target_compile_features(${PROJECT_NAME}
    PRIVATE cxx_std_17)