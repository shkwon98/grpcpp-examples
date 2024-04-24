project( server )

add_executable( ${PROJECT_NAME} )
target_sources( ${PROJECT_NAME}
    PRIVATE main.cpp )
target_link_libraries( ${PROJECT_NAME}
    PRIVATE robl::api )
target_compile_features( ${PROJECT_NAME}
    PRIVATE cxx_std_17 )