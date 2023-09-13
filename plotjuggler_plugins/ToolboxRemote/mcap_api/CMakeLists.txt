
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core)

if (NOT (Qt5_FOUND))
    message(FATAL_ERROR "Qt libraries were not found.")
endif()

#---- library ----

add_library(mcap_remote_api STATIC
    mcap_api.h
    mcap_protocol.h
    mcap_protocol.cpp
)

target_link_libraries(mcap_remote_api PUBLIC
    Qt5::Core
    ${CONAN_DEPENDENCIES}
)
