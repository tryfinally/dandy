# Aggressive AI plugin
add_library(aggressive_ai SHARED
    aggressive_ai.cpp
)

target_include_directories(aggressive_ai
    PRIVATE
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/client/include
)

target_link_libraries(aggressive_ai
    PRIVATE
        dandy::common
)

# Set output name without 'lib' prefix
set_target_properties(aggressive_ai PROPERTIES
    PREFIX ""
    OUTPUT_NAME "aggressive_ai"
)
