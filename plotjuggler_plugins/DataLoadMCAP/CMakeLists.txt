add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  dialog_mcap.ui  )


if(NOT TARGET mcap)
    ### mcap ###
    CPMAddPackage(
        NAME mcap
        GITHUB_REPOSITORY foxglove/mcap
        GIT_TAG releases/cpp/v2.0.0
        DOWNLOAD_ONLY YES
    )
    # mcap has no CMake support, so we create our own target
    add_library(mcap INTERFACE)
    target_include_directories(mcap INTERFACE "${mcap_SOURCE_DIR}/cpp/mcap/include")
 endif()
########################################


add_library(DataLoadMCAP SHARED
    mcap.cpp
    dataload_mcap.cpp
    dialog_mcap.cpp
    ${UI_SRC}  )

target_link_libraries(DataLoadMCAP PRIVATE
        ${Qt5Widgets_LIBRARIES}
        ${Qt5Xml_LIBRARIES}
        ${Qt5Concurrent_LIBRARIES}
        plotjuggler_base
        lz4::lz4_static
        zstd::libzstd_static
        mcap
    )

install(TARGETS DataLoadMCAP DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
