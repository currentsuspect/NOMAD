/**
 * @file graph.hpp
 * @brief Audio processing graph
 * @author Nomad Team
 * @date 2025
 * 
 * The audio graph manages nodes and connections, providing topological
 * sorting for correct processing order and real-time safe modifications.
 */

#pragma once

#include "../../core/base/types.hpp"
#include "../../core/base/config.hpp"
#include "../../core/threading/lock_free_queue.hpp"
#include "../../dsp/util/buffer.hpp"
#include "node.hpp"
#include "connection.hpp"

#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <functional>

namespace nomad::audio {

/**
 * @brief Graph modification commands for real-time safe updates
 */
enum class GraphCommand : u8 {
    AddNode,
    RemoveNode,
    AddConnection,
    RemoveConnection,
    SetNodeBypassed,
    SetNodeMuted,
    Clear
};

/**
 * @brief Command message for graph modifications
 */
struct GraphCommandMessage {
    GraphCommand command;
    NodeId nodeId;
    ConnectionId connectionId;
    u32 param1;
    u32 param2;
    
    GraphCommandMessage() 
        : command(GraphCommand::Clear)
        , nodeId(INVALID_NODE_ID)
        , connectionId(INVALID_CONNECTION_ID)
        , param1(0), param2(0) {}
};

/**
 * @brief Result of graph validation
 */
struct GraphValidation {
    bool isValid = true;
    bool hasCycles = false;
    bool hasDisconnectedNodes = false;
    std::vector<NodeId> cycleNodes;
    std::vector<NodeId> disconnectedNodes;
    std::string errorMessage;
};

/**
 * @brief Audio processing graph
 * 
 * Manages a directed acyclic graph (DAG) of audio processing nodes.
 * Provides topological sorting for correct processing order and
 * supports real-time safe modifications via a command queue.
 * 
 * @rt-safety The process() method is real-time safe. Graph modifications
 *            are queued and applied between process calls.
 */
class AudioGraph {
public:
    AudioGraph();
    ~AudioGraph();
    
    // Non-copyable, non-movable
    AudioGraph(const AudioGraph&) = delete;
    AudioGraph& operator=(const AudioGraph&) = delete;
    AudioGraph(AudioGraph&&) = delete;
    AudioGraph& operator=(AudioGraph&&) = delete;
    
    //=========================================================================
    // Node Management
    //=========================================================================
    
    /**
     * @brief Add a node to the graph
     * @param node Node to add (ownership transferred)
     * @return Node ID, or INVALID_NODE_ID on failure
     * 
     * @note NOT real-time safe - call from main thread only
     */
    NodeId addNode(std::unique_ptr<INode> node);
    
    /**
     * @brief Remove a node from the graph
     * @param nodeId Node to remove
     * @return true if removed successfully
     * 
     * @note NOT real-time safe - call from main thread only
     */
    bool removeNode(NodeId nodeId);
    
    /**
     * @brief Get a node by ID
     * @param nodeId Node ID
     * @return Pointer to node, or nullptr if not found
     */
    [[nodiscard]] INode* getNode(NodeId nodeId);
    [[nodiscard]] const INode* getNode(NodeId nodeId) const;
    
    /**
     * @brief Get all nodes
     */
    [[nodiscard]] const std::unordered_map<NodeId, std::unique_ptr<INode>>& nodes() const {
        return m_nodes;
    }
    
    /**
     * @brief Get node count
     */
    [[nodiscard]] usize nodeCount() const noexcept { return m_nodes.size(); }
    
    //=========================================================================
    // Connection Management
    //=========================================================================
    
    /**
     * @brief Connect two node ports
     * @param source Source endpoint (output port)
     * @param destination Destination endpoint (input port)
     * @return Connection ID, or INVALID_CONNECTION_ID on failure
     * 
     * @note NOT real-time safe - call from main thread only
     */
    ConnectionId connect(ConnectionEndpoint source, ConnectionEndpoint destination);
    
    /**
     * @brief Disconnect a connection
     * @param connectionId Connection to remove
     * @return true if disconnected successfully
     * 
     * @note NOT real-time safe - call from main thread only
     */
    bool disconnect(ConnectionId connectionId);
    
    /**
     * @brief Disconnect all connections involving a node
     * @param nodeId Node to disconnect
     * @return Number of connections removed
     */
    u32 disconnectNode(NodeId nodeId);
    
    /**
     * @brief Get a connection by ID
     */
    [[nodiscard]] Connection* getConnection(ConnectionId connectionId);
    [[nodiscard]] const Connection* getConnection(ConnectionId connectionId) const;
    
    /**
     * @brief Get all connections
     */
    [[nodiscard]] const std::unordered_map<ConnectionId, Connection>& connections() const {
        return m_connections;
    }
    
    /**
     * @brief Get connections from a specific node
     */
    [[nodiscard]] std::vector<const Connection*> getConnectionsFrom(NodeId nodeId) const;
    
    /**
     * @brief Get connections to a specific node
     */
    [[nodiscard]] std::vector<const Connection*> getConnectionsTo(NodeId nodeId) const;
    
    //=========================================================================
    // Processing
    //=========================================================================
    
    /**
     * @brief Prepare all nodes for processing
     * @param sampleRate Sample rate in Hz
     * @param maxBufferSize Maximum buffer size
     * 
     * @note NOT real-time safe
     */
    void prepare(u32 sampleRate, u32 maxBufferSize);
    
    /**
     * @brief Process audio through the graph
     * @param context Processing context
     * 
     * @rt-safety Real-time safe
     */
    void process(const ProcessContext& context) noexcept;
    
    /**
     * @brief Release all nodes
     * 
     * @note NOT real-time safe
     */
    void release();
    
    /**
     * @brief Reset all nodes
     * 
     * @rt-safety Real-time safe (if all nodes are RT-safe)
     */
    void reset() noexcept;
    
    //=========================================================================
    // Validation
    //=========================================================================
    
    /**
     * @brief Validate the graph structure
     * @return Validation result with details
     */
    [[nodiscard]] GraphValidation validate() const;
    
    /**
     * @brief Check if the graph has any cycles
     */
    [[nodiscard]] bool hasCycles() const;
    
    /**
     * @brief Get the processing order (topologically sorted)
     */
    [[nodiscard]] const std::vector<NodeId>& processingOrder() const {
        return m_processingOrder;
    }
    
    //=========================================================================
    // Graph State
    //=========================================================================
    
    /**
     * @brief Clear the entire graph
     * 
     * @note NOT real-time safe
     */
    void clear();
    
    /**
     * @brief Check if graph is empty
     */
    [[nodiscard]] bool isEmpty() const noexcept {
        return m_nodes.empty();
    }
    
    /**
     * @brief Check if graph is dirty (needs recompilation)
     */
    [[nodiscard]] bool isDirty() const noexcept {
        return m_dirty.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Force recompilation of processing order
     */
    void recompile();

private:
    /**
     * @brief Topologically sort the graph
     * @return true if sort succeeded (no cycles)
     */
    bool topologicalSort();
    
    /**
     * @brief DFS helper for topological sort
     */
    bool topologicalSortDFS(NodeId nodeId,
                           std::unordered_map<NodeId, int>& visited,
                           std::vector<NodeId>& result) const;
    
    /**
     * @brief Allocate working buffers for processing
     */
    void allocateBuffers(u32 bufferSize);
    
    /**
     * @brief Mark graph as dirty (needs recompilation)
     */
    void markDirty() noexcept {
        m_dirty.store(true, std::memory_order_release);
    }
    
    // Node storage
    std::unordered_map<NodeId, std::unique_ptr<INode>> m_nodes;
    NodeId m_nextNodeId = 1;
    
    // Connection storage
    std::unordered_map<ConnectionId, Connection> m_connections;
    ConnectionId m_nextConnectionId = 1;
    
    // Processing order (topologically sorted)
    std::vector<NodeId> m_processingOrder;
    std::atomic<bool> m_dirty{true};
    
    // Working buffers for processing
    std::vector<dsp::AudioBuffer> m_workBuffers;
    std::unordered_map<NodeId, std::vector<dsp::AudioBuffer>> m_nodeInputBuffers;
    std::unordered_map<NodeId, std::vector<dsp::AudioBuffer>> m_nodeOutputBuffers;
    
    // Processing state
    u32 m_sampleRate = 44100;
    u32 m_maxBufferSize = 512;
    bool m_prepared = false;
    
    // Command queue for RT-safe modifications
    core::SPSCQueue<GraphCommandMessage, 256> m_commandQueue;
};

} // namespace nomad::audio
