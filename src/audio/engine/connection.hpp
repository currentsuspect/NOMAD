/**
 * @file connection.hpp
 * @brief Audio graph connections between nodes
 * @author Nomad Team
 * @date 2025
 * 
 * Defines connections between node ports in the audio graph.
 */

#pragma once

#include "../../core/base/types.hpp"
#include "node.hpp"

#include <memory>

namespace nomad::audio {

/**
 * @brief Unique identifier for connections
 */
using ConnectionId = u32;
constexpr ConnectionId INVALID_CONNECTION_ID = 0;

/**
 * @brief Endpoint of a connection
 */
struct ConnectionEndpoint {
    NodeId nodeId;
    u32 portId;
    
    ConnectionEndpoint() : nodeId(INVALID_NODE_ID), portId(0) {}
    ConnectionEndpoint(NodeId node, u32 port) : nodeId(node), portId(port) {}
    
    [[nodiscard]] bool isValid() const noexcept {
        return nodeId != INVALID_NODE_ID;
    }
    
    [[nodiscard]] bool operator==(const ConnectionEndpoint& other) const noexcept {
        return nodeId == other.nodeId && portId == other.portId;
    }
    
    [[nodiscard]] bool operator!=(const ConnectionEndpoint& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * @brief Connection between two node ports
 * 
 * Represents a directed edge in the audio graph from a source
 * (output port) to a destination (input port).
 */
class Connection {
public:
    Connection(ConnectionId connId, ConnectionEndpoint src, ConnectionEndpoint dst)
        : m_id(connId)
        , m_source(src)
        , m_destination(dst)
        , m_gain(1.0f)
        , m_enabled(true)
    {}
    
    [[nodiscard]] ConnectionId id() const noexcept { return m_id; }
    [[nodiscard]] const ConnectionEndpoint& source() const noexcept { return m_source; }
    [[nodiscard]] const ConnectionEndpoint& destination() const noexcept { return m_destination; }
    
    /**
     * @brief Get connection gain (for mixing)
     */
    [[nodiscard]] f32 gain() const noexcept { return m_gain; }
    
    /**
     * @brief Set connection gain
     */
    void setGain(f32 gain) noexcept { m_gain = gain; }
    
    /**
     * @brief Check if connection is enabled
     */
    [[nodiscard]] bool isEnabled() const noexcept { return m_enabled; }
    
    /**
     * @brief Enable/disable connection
     */
    void setEnabled(bool enabled) noexcept { m_enabled = enabled; }
    
    /**
     * @brief Check if this connection involves a specific node
     */
    [[nodiscard]] bool involvesNode(NodeId nodeId) const noexcept {
        return m_source.nodeId == nodeId || m_destination.nodeId == nodeId;
    }

private:
    ConnectionId m_id;
    ConnectionEndpoint m_source;
    ConnectionEndpoint m_destination;
    f32 m_gain;
    bool m_enabled;
};

} // namespace nomad::audio
