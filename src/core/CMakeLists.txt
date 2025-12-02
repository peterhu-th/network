add_library(core STATIC
    Config.cpp
    Logger.cpp
    Types.h
    Config.h
    Logger.h
)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(core PUBLIC Qt5::Core)
