project( client )

add_executable( ${PROJECT_NAME} )
target_sources( ${PROJECT_NAME}
    PRIVATE main.cpp
            sequential_file_reader.cpp
            utils.cpp )
target_link_libraries( ${PROJECT_NAME}
    PRIVATE robl::api )