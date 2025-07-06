include_directories(../)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

qt5_wrap_ui(UI_SRC dataload_csv.ui datetimehelp.ui)

set(SRC dataload_csv.cpp datetimehelp.cpp)

add_library(DataLoadCSV SHARED ${SRC} ${UI_SRC})
target_link_libraries(DataLoadCSV ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES}
                      plotjuggler_base QCodeEditor)

install(TARGETS DataLoadCSV DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
