cmake_minimum_required(VERSION 3.16)
project(mcapRemoteClient LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core)

if (NOT (Qt5_FOUND))
    message(FATAL_ERROR "Qt libraries were not found.")
endif()

#---- library ----

add_library(mcap_files STATIC
    mcap_files.cpp
)

target_link_libraries(mcap_files PUBLIC
    Qt5::Core
    ${CONAN_DEPENDENCIES}
)
