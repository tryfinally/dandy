# Common library - shared types and protocol
add_library(dandy_common STATIC
    src/protocol.cpp
    src/messages.cpp
    src/serialization.cpp
)

target_include_directories(dandy_common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(dandy_common
    PUBLIC
        Threads::Threads
)

# Alias for cleaner linking
add_library(dandy::common ALIAS dandy_common)
