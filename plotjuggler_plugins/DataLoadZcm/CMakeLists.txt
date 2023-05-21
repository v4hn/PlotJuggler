if(BUILDING_WITH_VCPKG)
    message(STATUS "Finding Zcm with vcpkg")
    set(Zcm_LIBRARIES libzcm)
elseif(BUILDING_WITH_CONAN)
    message(STATUS "Finding Zcm with conan")
    set(Zcm_LIBRARIES libzcm)
else()
    message(STATUS "Finding Zcm without package managers")
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
endif()

find_package(Zcm QUIET)

if(Zcm_FOUND)

    message(STATUS "[Zcm] found")

    include_directories(../ ${Zcm_INCLUDE_DIR})

    add_definitions(${QT_DEFINITIONS})
    add_definitions(-DQT_PLUGIN)

    QT5_WRAP_UI ( UI_SRC ../selectlistdialog.ui )

    SET( SRC dataload_zcm.cpp )

    add_library(DataLoadZcm SHARED ${SRC} ${UI_SRC}  )

    target_link_libraries(DataLoadZcm
        ${Qt5Widgets_LIBRARIES}
        ${Qt5Xml_LIBRARIES}
        plotjuggler_base
        ${Zcm_LIBRARIES}
        )

    install(TARGETS DataLoadZcm DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
else()
    message("[Zcm] not found. Skipping plugin DataLoadZcm.")
endif()
