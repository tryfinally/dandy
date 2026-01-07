# Simple AI plugin
add_library(simple_ai SHARED
    simple_ai.cpp
)

target_include_directories(simple_ai
    PRIVATE
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/client/include
)

target_link_libraries(simple_ai
    PRIVATE
        dandy::common
)

# Set output name without 'lib' prefix
set_target_properties(simple_ai PROPERTIES
    PREFIX ""
    OUTPUT_NAME "simple_ai"
)
