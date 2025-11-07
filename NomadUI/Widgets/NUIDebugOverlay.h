// © 2025 Nomad Studios – All Rights Reserved. Licensed for personal & educational use only.
#pragma once

#include "../Core/NUIComponent.h"
#include "../Graphics/NUIRenderer.h"
#include "../../NomadCore/include/NomadProfiler.h"
#include <array>
#include <string>

namespace NomadUI {

/**
 * @brief Real-time debug overlay displaying performance metrics
 * 
 * Features:
 * - Frame time graph (last 120 frames)
 * - FPS counter
 * - Memory usage (CPU & GPU)
 * - Draw call count
 * - Cache hit rate
 * - Toggle with F12
 */
class NUIDebugOverlay : public NUIComponent {
public:
    NUIDebugOverlay();
    virtual ~NUIDebugOverlay() = default;
    
    // Component interface
    void onRender(NUIRenderer& renderer) override;
    void onUpdate(double deltaTime) override;
    
    // Control
    void toggle() { visible_ = !visible_; }
    void show() { visible_ = true; }
    void hide() { visible_ = false; }
    bool isVisible() const { return visible_; }
    
    // Configuration
    void setGraphSize(int width, int height);
    void setPosition(float x, float y);
    
private:
    // Metrics
    static constexpr int FRAME_HISTORY = 120;
    std::array<float, FRAME_HISTORY> frameTimesMs_;
    int frameIndex_;
    
    float avgFrameTime_;
    float minFrameTime_;
    float maxFrameTime_;
    float currentFPS_;
    
    size_t cpuMemoryMB_;
    size_t gpuMemoryMB_;
    float cacheHitRate_;
    uint32_t drawCallCount_;
    uint32_t triangleCount_;
    
    // Display state
    bool visible_;
    float updateTimer_;
    static constexpr float UPDATE_INTERVAL = 0.033f; // 30 Hz update
    
    // Graph settings
    int graphWidth_;
    int graphHeight_;
    float posX_;
    float posY_;
    
    // Rendering helpers
    void drawGraph(NUIRenderer* renderer, float x, float y);
    void drawStats(NUIRenderer* renderer, float x, float y);
    void drawMemoryBar(NUIRenderer* renderer, float x, float y, const char* label, 
                       size_t used, size_t total);
    
    // Data collection
    void collectMetrics();
    void updateFrameTimeHistory();
    
    // Colors (Nomad theme)
    static constexpr uint32_t COLOR_PRIMARY = 0x785affFF;    // Vibrant purple
    static constexpr uint32_t COLOR_SUCCESS = 0x00d49eFF;    // Teal green
    static constexpr uint32_t COLOR_WARNING = 0xffb400FF;    // Amber
    static constexpr uint32_t COLOR_ERROR = 0xff4465FF;      // Vibrant red
    static constexpr uint32_t BG_SECONDARY = 0x1e1e22E6;     // Semi-transparent panel
    static constexpr uint32_t TEXT_PRIMARY = 0xeeeef2FF;     // Crisp white
    static constexpr uint32_t TEXT_SECONDARY = 0xaaaab2FF;   // Labels
};

} // namespace NomadUI
