# Game server executable
add_executable(dandy_server
    src/main.cpp
    src/tcp_server.cpp
    src/session.cpp
    src/game_engine.cpp
    src/world.cpp
    src/tank.cpp
    src/combat.cpp
)

target_include_directories(dandy_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(dandy_server
    PRIVATE
        dandy::common
        Threads::Threads
)

# Alias for reference
add_executable(dandy::server ALIAS dandy_server)
