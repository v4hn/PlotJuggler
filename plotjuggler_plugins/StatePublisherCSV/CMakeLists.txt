include_directories(../)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

qt5_wrap_ui(UI_SRC publisher_csv_dialog.ui)

add_library(PublisherCSV SHARED publisher_csv.cpp ${UI_SRC})

target_link_libraries(PublisherCSV ${Qt5Widgets_LIBRARIES} plotjuggler_base)

install(TARGETS PublisherCSV DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
