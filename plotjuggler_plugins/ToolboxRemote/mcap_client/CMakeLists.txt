set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS WebSockets)

if (NOT (Qt5_FOUND))
    message(FATAL_ERROR "Qt libraries were not found.")
endif()

#---- library ----

add_library(mcap_remote_client mcap_client.cpp)

set_target_properties(mcap_remote_client PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(mcap_remote_client PUBLIC
    mcap_remote_api
    Qt5::WebSockets
)
