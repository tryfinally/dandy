# Game client executable
add_executable(dandy_client
    src/main.cpp
    src/tcp_client.cpp
    src/display.cpp
    src/plugin_loader.cpp
    src/game_client.cpp
)

target_include_directories(dandy_client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(dandy_client
    PRIVATE
        dandy::common
        Threads::Threads
        ${CMAKE_DL_LIBS}
)

# Alias
add_executable(dandy::client ALIAS dandy_client)
