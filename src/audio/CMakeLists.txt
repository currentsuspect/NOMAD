# Audio Engine Module
# Real-time audio processing, timeline, mixing

add_library(nomad_audio INTERFACE)

target_include_directories(nomad_audio INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(nomad_audio INTERFACE
    nomad_core
    nomad_dsp
)

# Source groups for IDE organization
source_group("engine" FILES
    engine/node.hpp
    engine/connection.hpp
    engine/graph.hpp
)

source_group("transport" FILES
    transport/playhead.hpp
)

source_group("mixer" FILES
    mixer/channel.hpp
)

source_group("timeline" FILES
    timeline/track.hpp
)
