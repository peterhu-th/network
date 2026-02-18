
add_library(network_lib
    NetworkService.cpp
    NetworkService.h
    HttpServer.cpp
    HttpServer.h
    DatabaseManager.cpp
    DatabaseManager.h
    IdGenerator.cpp
    IdGenerator.h
    FileIndexer.cpp
    FileIndexer.h
)

target_include_directories(network_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(network_lib PUBLIC
    Qt5::Core
    Qt5::Network
    Qt5::Sql
    core
)
