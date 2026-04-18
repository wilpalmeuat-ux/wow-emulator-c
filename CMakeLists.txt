cmake_minimum_required(VERSION 3.16)
project(WoWEmulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -O2")
set(CMAKE_C_FLAGS_DEBUG "-g")

enable_language(CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

# MySQL client library
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)
include_directories(${MYSQL_INCLUDE_DIRS})

# Threads
find_package(Threads REQUIRED)

# Headers
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/shared
    ${CMAKE_SOURCE_DIR}/include/scripting
    ${CMAKE_SOURCE_DIR}/include/worldserver
    ${CMAKE_SOURCE_DIR}/include/authserver
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/scripting
)

# ═══════════════════════════════════════════
#  All C sources
# ═══════════════════════════════════════════
set(C_SOURCES
    src/main.c
    src/shared/log.c
    src/shared/ByteBuffer.c
    src/shared/BigNumber.c
    src/shared/SRP6.c
    src/shared/update_mask.c
    src/shared/wow_packet.c
    src/shared/database.c
    src/shared/Config/Config.c
    src/shared/Network.c
    src/authserver/authserver.c
    src/authserver/auth_main.c
    src/worldserver/worldserver.c
    src/worldserver/worldsession.c
    src/worldserver/player.c
    src/worldserver/worldsocket.c
    src/worldserver/packets.c
    src/scripting/script_engine.c
    src/scripting/wss_lexer.c
    src/scripting/wss_vm.c
    src/scripting/wss_value.c
    src/scripting/wss_wow_hooks.c
    src/scripting/wss_chunk.c
    src/scripting/wss_compiler.c
    src/scripting/wss_objectstore.c
    src/scripting/wss_scanner.c
    src/scripts/core/builtin_statements.c
)

set(CXX_SOURCES
    src/worldserver/world_main.cpp
    src/worldserver/WorldServer.cpp
)

add_executable(wow-emulator ${C_SOURCES} ${CXX_SOURCES})
target_link_libraries(wow-emulator ${MYSQL_LIBRARIES} Threads::Threads m)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/core)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/commands)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/creatures)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/spells)
