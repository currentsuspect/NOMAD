# DSP Module CMakeLists.txt
# Pure signal processing algorithms - no external dependencies except core

add_library(nomad_dsp INTERFACE)

target_include_directories(nomad_dsp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(nomad_dsp INTERFACE
    nomad_core
)

# DSP is header-only for now (templates and inline implementations)
# This allows maximum optimization at the call site

target_sources(nomad_dsp INTERFACE
    # Processors
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/processor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/filter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/dynamics.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/oscillator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/delay.hpp
    
    # Analysis
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/fft.hpp
    
    # Utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/util/buffer.hpp
)

# Add alias for consistent naming
add_library(Nomad::DSP ALIAS nomad_dsp)
