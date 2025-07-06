include_directories(../)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

qt5_wrap_ui(UI_SRC udp_server.ui)

set(SRC udp_server.cpp)

add_library(DataStreamUDP SHARED ${SRC} ${UI_SRC})

target_link_libraries(DataStreamUDP ${Qt5Widgets_LIBRARIES} Qt5::Network
                      plotjuggler_base)

install(TARGETS DataStreamUDP DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
