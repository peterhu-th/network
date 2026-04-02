add_library(network
        mapper/AudioRecordMapper.cpp mapper/AudioRecordMapper.h
        service/AudioRecordService.cpp service/AudioRecordService.h
        controller/AudioRecordController.cpp controller/AudioRecordController.h
        utils/IdGenerator.cpp utils/IdGenerator.h
        utils/FileIndexer.cpp utils/FileIndexer.h
        utils/ThrottledFile.h
        ApiResponse.h NetworkDTO.h
)

find_package(Qt6 REQUIRED COMPONENTS Core Network Sql HttpServer)

target_include_directories(network PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(network PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Sql
        Qt6::HttpServer
        core
)