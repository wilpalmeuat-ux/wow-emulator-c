# Shared library - common code for both servers
add_library(Shared INTERFACE)

target_include_directories(Shared INTERFACE
    ${PROJECT_SOURCE_DIR}/include/shared
    ${PROJECT_SOURCE_DIR}/include
)

target_sources(Shared INTERFACE
    # Network
    ${PROJECT_SOURCE_DIR}/src/shared/Network/Socket.cpp
    ${PROJECT_SOURCE_DIR}/src/shared/Network/SocketMgr.cpp
    ${PROJECT_SOURCE_DIR}/src/shared/Network/TcpConnection.cpp
    ${PROJECT_SOURCE_DIR}/src/shared/Network/Buffer.cpp

    # Database
    ${PROJECT_SOURCE_DIR}/src/shared/Database/Database.cpp
    ${PROJECT_SOURCE_DIR}/src/shared/Database/PreparedStatement.cpp
    ${PROJECT_SOURCE_DIR}/src/shared/Database/QueryResult.cpp

    # Utils
    ${PROJECT_SOURCE_DIR}/src/shared/Utils/Util.cpp
    ${PROJECT_SOURCE_DIR}/src/shared/Utils/ByteBuffer.cpp
    ${PROJECT_SOURCE_DIR}/src/shared/Utils/BigNumber.cpp
    ${PROJECT_SOURCE_DIR}/src/shared/Utils/SRP6.cpp
    ${PROJECT_SOURCE_DIR}/src/shared/Utils/CryptoHash.cpp
    ${PROJECT_SOURCE_DIR}/src/shared/Utils/WorldPacket.cpp

    # Logging
    ${PROJECT_SOURCE_DIR}/src/shared/Logging/Log.cpp

    # Config
    ${PROJECT_SOURCE_DIR}/src/shared/Config/Config.cpp
)
