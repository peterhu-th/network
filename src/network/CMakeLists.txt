add_library(network
        mapper/AudioRecordMapper.cpp mapper/AudioRecordMapper.h
        service/AudioRecordService.cpp service/AudioRecordService.h
        controller/AudioRecordController.cpp controller/AudioRecordController.h
        utils/IdGenerator.cpp utils/IdGenerator.h
        utils/FileIndexer.cpp utils/FileIndexer.h
        utils/ThrottledFile.h utils/JwtUtils.h
        utils/JwtUtils.h utils/ThrottledFile.h
        NetworkResponse.h NetworkDTO.h
)

find_package(Qt6 REQUIRED COMPONENTS Core Network Sql HttpServer)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
pkg_check_modules(TAGLIB REQUIRED taglib)

target_include_directories(network PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${TAGLIB_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/third-party/jwt-cpp/include
        ${CMAKE_SOURCE_DIR}/third_party/miniz
)

target_link_libraries(network PUBLIC
        ${TAGLIB_LIBRARIES}
        Qt6::Core
        Qt6::Network
        Qt6::Sql
        Qt6::HttpServer
        OpenSSL::Crypto
        core
)