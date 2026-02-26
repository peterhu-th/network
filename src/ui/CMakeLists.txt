add_library(ui
        MainWindow.cpp
        MainWindow.h
)

target_include_directories(ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui PUBLIC
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    network
    core
)
