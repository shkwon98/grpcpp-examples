project( client )

add_executable( ${PROJECT_NAME} )
target_sources( ${PROJECT_NAME}
    PRIVATE main.cpp
            test_client/sequential_file_reader.cpp
            test_client/utils.cpp )
target_link_libraries( ${PROJECT_NAME}
    PRIVATE robl::api )