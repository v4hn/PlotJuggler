if(BUILDING_WITH_CONAN)
    find_package(zstd CONFIG REQUIRED)
    set(Zstd_LIBRARIES zstd::libzstd_static)
    set(Zstd_FOUND ON)
else()
    find_package(Zstd REQUIRED)
endif()

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)
include_directories(./)

find_package(Qt5 QUIET COMPONENTS WebSockets)

QT5_WRAP_UI ( UI_SRC
    widget_remote_load.ui
    dialog_file_selection.ui  )

add_library(mcap_api STATIC
    mcap_api/mcap_protocol.cpp
    mcap_client/mcap_client.cpp)
target_include_directories(mcap_api PUBLIC
    $<BUILD_INTERFACE:${Zstd_INCLUDE_DIRS}>)

target_link_libraries(mcap_api PUBLIC Qt5::WebSockets)

target_include_directories(mcap_api PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty)


add_subdirectory(mcap_api)
add_subdirectory(mcap_client)

add_library(ToolboxRemote SHARED
    widget_remote_load.cpp
    toolbox_remote.cpp
    ${UI_SRC} )

target_link_libraries(ToolboxRemote
    Qt5::Widgets
    Qt5::Xml
    plotjuggler_base
    plotjuggler_qwt
    mcap_remote_api
    mcap_remote_client)

install(TARGETS ToolboxRemote DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
