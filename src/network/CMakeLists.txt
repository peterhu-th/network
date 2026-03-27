add_library(network
        mapper/AudioRecordMapper.cpp
        mapper/AudioRecordMapper.h
        service/AudioRecordService.cpp
        service/AudioRecordService.h
        controller/AudioRecordController.cpp
        controller/AudioRecordController.h
        utils/HttpServer.cpp
        utils/HttpServer.h
        utils/IdGenerator.cpp
        utils/IdGenerator.h
        utils/FileIndexer.cpp
        utils/FileIndexer.h
        NetworkDefines.h
)

target_include_directories(network PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(network PUBLIC
        Qt5::Core
        Qt5::Network
        Qt5::Sql
        core
)