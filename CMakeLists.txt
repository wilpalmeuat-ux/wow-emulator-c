cmake_minimum_required(VERSION 3.16)
project(WoWEmulator C)

# ═══════════════════════════════════════════
#  Windows-only C11 build targeting MSVC
# ═══════════════════════════════════════════
set(CMAKE_C_STANDARD 11)

if(MSVC)
    # MSVC flags
    add_compile_options(/W3 /O2 /D_CRT_SECURE_NO_WARNINGS /DWIN32 /D_WIN32)
    add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
else()
    # MinGW / GCC on Windows
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2 -DWIN32 -D_WIN32")
endif()

# ═══════════════════════════════════════════
#  MySQL Connector/C
#  Set MYSQL_DIR to your MySQL install path
#  e.g. cmake -DMYSQL_DIR="C:/Program Files/MySQL/MySQL Server 8.0"
# ═══════════════════════════════════════════
if(NOT DEFINED MYSQL_DIR)
    # Auto-detect common paths
    if(EXISTS "C:/Program Files/MySQL/MySQL Server 8.0")
        set(MYSQL_DIR "C:/Program Files/MySQL/MySQL Server 8.0")
    elseif(EXISTS "C:/mysql")
        set(MYSQL_DIR "C:/mysql")
    elseif(EXISTS "C:/Program Files/MySQL/MySQL Server 5.7")
        set(MYSQL_DIR "C:/Program Files/MySQL/MySQL Server 5.7")
    else()
        message(WARNING "MySQL not found. Set -DMYSQL_DIR=<path>")
        set(MYSQL_DIR "C:/mysql")
    endif()
endif()

message(STATUS "MySQL directory: ${MYSQL_DIR}")

# MySQL include + lib
include_directories("${MYSQL_DIR}/include")
link_directories("${MYSQL_DIR}/lib")

# ═══════════════════════════════════════════
#  Project include directories
# ═══════════════════════════════════════════
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
#  Source files (pure C, Windows only)
# ═══════════════════════════════════════════
set(SHARED_SOURCES
    src/shared/log.c
    src/shared/ByteBuffer.c
    src/shared/BigNumber.c
    src/shared/SRP6.c
    src/shared/sha1.c
    src/shared/database.c
    src/shared/dbc_loader.c
    src/shared/Config/Config.c
)

set(AUTH_SOURCES
    src/authserver/authserver.c
    src/authserver/auth_main.c
)

set(WORLD_SOURCES
    src/worldserver/worldserver.c
    src/worldserver/worldsession.c
    src/worldserver/player.c
    src/worldserver/worldsocket.c
    src/worldserver/packets.c
    src/worldserver/update_object.c
    src/worldserver/spell_system.c
    src/worldserver/combat_system.c
    src/worldserver/item_system.c
    src/worldserver/quest_system.c
    src/worldserver/loot_system.c
    src/worldserver/npc_ai.c
    src/worldserver/grid_system.c
    src/worldserver/guild_system.c
    src/worldserver/group_system.c
    src/worldserver/mail_system.c
    src/worldserver/auction_system.c
    src/worldserver/talent_system.c
    src/worldserver/achievement_system.c
    src/worldserver/weather_system.c
    src/worldserver/opcode_handlers.c
    src/worldserver/battleground.c
    src/worldserver/instance_system.c
    src/worldserver/profession_system.c
    src/worldserver/pet_system.c
    src/worldserver/vehicle_system.c
    src/worldserver/phasing_system.c
    src/worldserver/anticheat.c
)

set(SCRIPTING_SOURCES
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

set(ALL_SOURCES
    src/main.c
    ${SHARED_SOURCES}
    ${AUTH_SOURCES}
    ${WORLD_SOURCES}
    ${SCRIPTING_SOURCES}
)

# ═══════════════════════════════════════════
#  Build targets
# ═══════════════════════════════════════════

# Combined server (auth + world in one binary)
add_executable(wow-emulator ${ALL_SOURCES})
target_link_libraries(wow-emulator libmysql ws2_32)

# Set output directory
set_target_properties(wow-emulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin"
)

# ═══════════════════════════════════════════
#  Create script directories
# ═══════════════════════════════════════════
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/core)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/commands)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/creatures)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/spells)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/events)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# ═══════════════════════════════════════════
#  Copy config + SQL to output
# ═══════════════════════════════════════════
configure_file(${CMAKE_SOURCE_DIR}/configs/worldserver.conf
               ${CMAKE_SOURCE_DIR}/bin/configs/worldserver.conf COPYONLY)

message(STATUS "")
message(STATUS "=== WoW 3.3.5a Emulator (C + WSS Scripting) ===")
message(STATUS "  Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  MySQL        : ${MYSQL_DIR}")
message(STATUS "  Output       : ${CMAKE_SOURCE_DIR}/bin/")
message(STATUS "================================================")
