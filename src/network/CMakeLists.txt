add_library(network
        mapper/AudioRecordMapper.cpp mapper/AudioRecordMapper.h
        mapper/UserMapper.cpp mapper/UserMapper.h
        service/AudioRecordService.cpp service/AudioRecordService.h
        service/AuthService.cpp service/AuthService.h
        service/AdminService.cpp service/AdminService.h
        controller/AudioRecordController.cpp controller/AudioRecordController.h
        controller/AuthController.cpp controller/AuthController.h
        controller/AdminController.cpp controller/AdminController.h
        utils/IdGenerator.cpp utils/IdGenerator.h
        utils/FileIndexer.cpp utils/FileIndexer.h
        utils/SmtpClient.cpp utils/SmtpClient.h
        utils/CryptoUtils.cpp utils/CryptoUtils.h
        utils/AuthMiddleware.cpp utils/AuthMiddleware.h
        utils/JwtUtils.h utils/ThrottledFile.h utils/ZipUtils.h utils/MemoryCache.h utils/dbUtils.h
        NetworkResponse.h NetworkDTO.h
        ${CMAKE_SOURCE_DIR}/third-party/miniz/miniz.c
)

find_package(Qt6 REQUIRED COMPONENTS Core Network Sql HttpServer Concurrent)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

pkg_check_modules(TAGLIB REQUIRED taglib)

target_include_directories(network PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${TAGLIB_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/third-party/jwt-cpp/include
        ${CMAKE_SOURCE_DIR}/third-party/miniz
)

target_link_libraries(network PUBLIC
        ${TAGLIB_LIBRARIES}
        Qt6::Core
        Qt6::Network
        Qt6::Sql
        Qt6::HttpServer
        Qt6::Concurrent
        CURL::libcurl
        OpenSSL::SSL
        OpenSSL::Crypto
        core
)
