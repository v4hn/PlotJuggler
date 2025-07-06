include_directories(../)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

qt5_wrap_ui(UI_SRC ../selectlistdialog.ui ulog_parameters_dialog.ui)

set(SRC dataload_ulog.cpp ulog_parser.cpp ulog_parameters_dialog.cpp)

add_library(DataLoadULog SHARED ${SRC} ${UI_SRC})

target_link_libraries(DataLoadULog ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES}
                      plotjuggler_base)

install(TARGETS DataLoadULog DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
