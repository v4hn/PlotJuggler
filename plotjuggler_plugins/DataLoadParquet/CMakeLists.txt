if(BUILDING_WITH_VCPKG)
  message(STATUS "Finding Parquet with vcpkg")
  set(PARQUET_LIBRARIES re2::re2 arrow_shared parquet_shared
                        arrow_bundled_dependencies)

  find_package(arrow CONFIG QUIET)

else()
  message(STATUS "Finding Parquet without package managers")
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

  find_package(Arrow CONFIG QUIET)
  find_package(Parquet CONFIG QUIET)
endif()

if(Arrow_FOUND)
  message(STATUS "[Parquet] found")
  include_directories(../ ${ARROW_INCLUDE_DIR} ${PARQUET_INCLUDE_DIR})

  qt5_wrap_ui(UI_SRC dataload_parquet.ui)

  set(SRC dataload_parquet.cpp)

  add_library(DataLoadParquet SHARED ${SRC} ${UI_SRC})

  target_link_libraries(
    DataLoadParquet PRIVATE Qt5::Widgets Qt5::Xml ${PARQUET_SHARED_LIB}
                            plotjuggler_base)

  target_compile_definitions(DataLoadParquet PRIVATE QT_PLUGIN)

  install(TARGETS DataLoadParquet DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})

else()
  message("[Parquet] not found. Skipping plugin DataLoadParquet.")
endif()
