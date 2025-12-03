/**
 * @file node.hpp
 * @brief Audio processing node for the audio graph
 * @author Nomad Team
 * @date 2025
 * 
 * Defines the base interface for all audio processing nodes in the graph.
 * Nodes can be sources (oscillators, samplers), processors (effects, filters),
 * or sinks (output, analyzers).
 */

#pragma once

#include "../../core/base/types.hpp"
#include "../../core/base/config.hpp"
#include "../../dsp/util/buffer.hpp"

#include <string>
#include <vector>
#include <atomic>
#include <memory>

namespace nomad::audio {

// Forward declarations
class AudioGraph;
class Connection;

/**
 * @brief Node type classification
 */
enum class NodeType : u8 {
    Source,      ///< Generates audio (oscillator, sampler, input)
    Processor,   ///< Processes audio (effect, filter)
    Sink,        ///< Consumes audio (output, analyzer)
    Mixer,       ///< Combines multiple inputs
    Splitter,    ///< Splits to multiple outputs
    Utility      ///< Non-audio (MIDI, control)
};

/**
 * @brief Port direction
 */
enum class PortDirection : u8 {
    Input,
    Output
};

/**
 * @brief Port type
 */
enum class PortType : u8 {
    Audio,       ///< Audio signal
    Control,     ///< Control rate (automation, modulation)
    Midi,        ///< MIDI events
    Sidechain    ///< Sidechain input for dynamics
};

/**
 * @brief Port descriptor
 */
struct PortDescriptor {
    u32 id;
    PortDirection direction;
    PortType type;
    std::string name;
    u32 channels;        ///< Number of channels (1=mono, 2=stereo, etc.)
    
    PortDescriptor(u32 portId, PortDirection dir, PortType portType,
                   std::string portName, u32 numChannels = 2)
        : id(portId), direction(dir), type(portType)
        , name(std::move(portName)), channels(numChannels) {}
};

/**
 * @brief Processing context passed to nodes
 */
struct ProcessContext {
    u32 sampleRate;
    u32 bufferSize;
    u64 samplePosition;      ///< Current position in samples
    f64 tempo;               ///< Current tempo in BPM
    f64 beatsPerBar;         ///< Time signature numerator
    f64 beatUnit;            ///< Time signature denominator
    bool isPlaying;
    bool isRecording;
    bool isLooping;
};

/**
 * @brief Unique identifier for nodes
 */
using NodeId = u32;
constexpr NodeId INVALID_NODE_ID = 0;

/**
 * @brief Base class for all audio processing nodes
 * 
 * @rt-safety Derived classes must ensure process() is real-time safe:
 *            - No memory allocation
 *            - No blocking operations
 *            - No system calls
 */
class INode {
public:
    virtual ~INode() = default;
    
    //=========================================================================
    // Identity
    //=========================================================================
    
    /**
     * @brief Get unique node ID
     */
    [[nodiscard]] virtual NodeId id() const noexcept = 0;
    
    /**
     * @brief Get node type
     */
    [[nodiscard]] virtual NodeType type() const noexcept = 0;
    
    /**
     * @brief Get human-readable name
     */
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    
    //=========================================================================
    // Ports
    //=========================================================================
    
    /**
     * @brief Get all port descriptors
     */
    [[nodiscard]] virtual const std::vector<PortDescriptor>& ports() const noexcept = 0;
    
    /**
     * @brief Get input port count
     */
    [[nodiscard]] virtual u32 inputPortCount() const noexcept = 0;
    
    /**
     * @brief Get output port count
     */
    [[nodiscard]] virtual u32 outputPortCount() const noexcept = 0;
    
    //=========================================================================
    // Processing
    //=========================================================================
    
    /**
     * @brief Prepare for processing
     * 
     * Called when audio starts or parameters change.
     * NOT real-time safe - can allocate, block, etc.
     * 
     * @param sampleRate Sample rate in Hz
     * @param maxBufferSize Maximum buffer size in samples
     */
    virtual void prepare(u32 sampleRate, u32 maxBufferSize) = 0;
    
    /**
     * @brief Process audio
     * 
     * @rt-safety MUST be real-time safe
     * 
     * @param inputs Input buffers (one per input port)
     * @param outputs Output buffers (one per output port)
     * @param context Processing context
     */
    virtual void process(const std::vector<dsp::AudioBuffer>& inputs,
                        std::vector<dsp::AudioBuffer>& outputs,
                        const ProcessContext& context) noexcept = 0;
    
    /**
     * @brief Release resources
     * 
     * Called when audio stops. NOT real-time safe.
     */
    virtual void release() = 0;
    
    /**
     * @brief Reset internal state
     * 
     * Clear delay lines, envelopes, etc.
     * May be called from audio thread - should be RT-safe.
     */
    virtual void reset() noexcept = 0;
    
    //=========================================================================
    // State
    //=========================================================================
    
    /**
     * @brief Check if node is bypassed
     */
    [[nodiscard]] virtual bool isBypassed() const noexcept = 0;
    
    /**
     * @brief Set bypass state
     */
    virtual void setBypassed(bool bypassed) noexcept = 0;
    
    /**
     * @brief Check if node is muted
     */
    [[nodiscard]] virtual bool isMuted() const noexcept = 0;
    
    /**
     * @brief Set mute state
     */
    virtual void setMuted(bool muted) noexcept = 0;
};

/**
 * @brief Base implementation of INode with common functionality
 */
class NodeBase : public INode {
public:
    explicit NodeBase(NodeId nodeId, NodeType nodeType, std::string nodeName)
        : m_id(nodeId)
        , m_type(nodeType)
        , m_name(std::move(nodeName))
        , m_bypassed(false)
        , m_muted(false)
    {}
    
    [[nodiscard]] NodeId id() const noexcept override { return m_id; }
    [[nodiscard]] NodeType type() const noexcept override { return m_type; }
    [[nodiscard]] std::string_view name() const noexcept override { return m_name; }
    
    [[nodiscard]] const std::vector<PortDescriptor>& ports() const noexcept override {
        return m_ports;
    }
    
    [[nodiscard]] u32 inputPortCount() const noexcept override {
        u32 count = 0;
        for (const auto& port : m_ports) {
            if (port.direction == PortDirection::Input) ++count;
        }
        return count;
    }
    
    [[nodiscard]] u32 outputPortCount() const noexcept override {
        u32 count = 0;
        for (const auto& port : m_ports) {
            if (port.direction == PortDirection::Output) ++count;
        }
        return count;
    }
    
    [[nodiscard]] bool isBypassed() const noexcept override {
        return m_bypassed.load(std::memory_order_acquire);
    }
    
    void setBypassed(bool bypassed) noexcept override {
        m_bypassed.store(bypassed, std::memory_order_release);
    }
    
    [[nodiscard]] bool isMuted() const noexcept override {
        return m_muted.load(std::memory_order_acquire);
    }
    
    void setMuted(bool muted) noexcept override {
        m_muted.store(muted, std::memory_order_release);
    }

protected:
    /**
     * @brief Add a port to this node
     */
    void addPort(PortDirection direction, PortType type, 
                 std::string portName, u32 channels = 2) {
        u32 portId = static_cast<u32>(m_ports.size());
        m_ports.emplace_back(portId, direction, type, std::move(portName), channels);
    }
    
    NodeId m_id;
    NodeType m_type;
    std::string m_name;
    std::vector<PortDescriptor> m_ports;
    std::atomic<bool> m_bypassed;
    std::atomic<bool> m_muted;
    
    // Processing state
    u32 m_sampleRate = 44100;
    u32 m_maxBufferSize = 512;
};

} // namespace nomad::audio
