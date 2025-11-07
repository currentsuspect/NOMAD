// © 2025 Nomad Studios – All Rights Reserved. Licensed for personal & educational use only.
#include "NUIDebugOverlay.h"
#include "../Graphics/NUIRenderer.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
    #include <Psapi.h>
    #pragma comment(lib, "psapi.lib")
#endif

// GLAD for GPU memory queries
#include "../External/glad/include/glad/glad.h"

namespace NomadUI {

NUIDebugOverlay::NUIDebugOverlay()
    : frameIndex_(0)
    , avgFrameTime_(16.67f)
    , minFrameTime_(999.0f)
    , maxFrameTime_(0.0f)
    , currentFPS_(60.0f)
    , cpuMemoryMB_(0)
    , gpuMemoryMB_(0)
    , cacheHitRate_(0.0f)
    , drawCallCount_(0)
    , triangleCount_(0)
    , visible_(false)
    , updateTimer_(0.0f)
    , graphWidth_(400)
    , graphHeight_(120)
    , posX_(10.0f)
    , posY_(10.0f)
{
    frameTimesMs_.fill(16.67f);
}

void NUIDebugOverlay::onUpdate(double deltaTime) {
    if (!visible_) return;
    
    updateTimer_ += static_cast<float>(deltaTime);
    
    // Update metrics at fixed interval
    if (updateTimer_ >= UPDATE_INTERVAL) {
        updateTimer_ = 0.0f;
        collectMetrics();
        updateFrameTimeHistory();
    }
}

void NUIDebugOverlay::onRender(NUIRenderer& renderer) {
    if (!visible_) return;
    
    // Draw semi-transparent background
    float padding = 10.0f;
    float totalWidth = graphWidth_ + padding * 2;
    float totalHeight = graphHeight_ + 180.0f; // Graph + stats
    
    NUIRect bgRect(posX_, posY_, totalWidth, totalHeight);
    renderer.fillRoundedRect(bgRect, 8.0f, NUIColor::fromHex(BG_SECONDARY));
    
    // Draw title
    float textX = posX_ + padding;
    float textY = posY_ + padding;
    renderer.drawText("NOMAD Performance Monitor", NUIPoint(textX, textY), 
                      14.0f, NUIColor::fromHex(TEXT_PRIMARY));
    
    // Draw graph
    drawGraph(&renderer, textX, textY + 25.0f);
    
    // Draw stats
    drawStats(&renderer, textX, textY + graphHeight_ + 35.0f);
}

void NUIDebugOverlay::setGraphSize(int width, int height) {
    graphWidth_ = width;
    graphHeight_ = height;
}

void NUIDebugOverlay::setPosition(float x, float y) {
    posX_ = x;
    posY_ = y;
}

void NUIDebugOverlay::drawGraph(NUIRenderer* renderer, float x, float y) {
    // Draw graph background
    NUIRect graphBg(x, y, graphWidth_, graphHeight_);
    renderer->fillRect(graphBg, NUIColor(0.1f, 0.1f, 0.12f, 1.0f));
    
    // Draw reference lines (16ms, 33ms, 66ms)
    float ref60fps = graphHeight_ * (1.0f - 16.67f / 100.0f);
    float ref30fps = graphHeight_ * (1.0f - 33.33f / 100.0f);
    float ref15fps = graphHeight_ * (1.0f - 66.67f / 100.0f);
    
    renderer->drawLine(NUIPoint(x, y + ref60fps), 
                      NUIPoint(x + graphWidth_, y + ref60fps),
                      1.0f, NUIColor::fromHex(COLOR_SUCCESS));
    
    renderer->drawLine(NUIPoint(x, y + ref30fps), 
                      NUIPoint(x + graphWidth_, y + ref30fps),
                      1.0f, NUIColor::fromHex(COLOR_WARNING));
    
    renderer->drawLine(NUIPoint(x, y + ref15fps), 
                      NUIPoint(x + graphWidth_, y + ref15fps),
                      1.0f, NUIColor::fromHex(COLOR_ERROR));
    
    // Draw frame time graph
    float barWidth = (float)graphWidth_ / FRAME_HISTORY;
    float maxTime = std::max(maxFrameTime_, 100.0f); // Scale to at least 100ms
    
    for (int i = 0; i < FRAME_HISTORY; ++i) {
        int idx = (frameIndex_ + i) % FRAME_HISTORY;
        float frameTime = frameTimesMs_[idx];
        float barHeight = (frameTime / maxTime) * graphHeight_;
        barHeight = std::min(barHeight, (float)graphHeight_);
        
        float barX = x + i * barWidth;
        float barY = y + graphHeight_ - barHeight;
        
        // Color based on frame time
        uint32_t color;
        if (frameTime < 16.67f) {
            color = COLOR_SUCCESS; // 60+ FPS - Green
        } else if (frameTime < 33.33f) {
            color = COLOR_WARNING; // 30-60 FPS - Yellow
        } else {
            color = COLOR_ERROR; // <30 FPS - Red
        }
        
        NUIRect bar(barX, barY, barWidth - 1.0f, barHeight);
        renderer->fillRect(bar, NUIColor::fromHex(color));
    }
    
    // Draw labels
    std::stringstream ss;
    ss << "Frame Time (ms)  |  Avg: " << std::fixed << std::setprecision(2) 
       << avgFrameTime_ << "  Min: " << minFrameTime_ << "  Max: " << maxFrameTime_;
    renderer->drawText(ss.str(), NUIPoint(x, y - 18.0f), 
                      10.0f, NUIColor::fromHex(TEXT_SECONDARY));
}

void NUIDebugOverlay::drawStats(NUIRenderer* renderer, float x, float y) {
    float lineHeight = 18.0f;
    float currentY = y;
    
    std::stringstream ss;
    
    // FPS
    ss << "FPS: " << std::fixed << std::setprecision(1) << currentFPS_;
    uint32_t fpsColor = currentFPS_ >= 60.0f ? COLOR_SUCCESS : 
                       (currentFPS_ >= 30.0f ? COLOR_WARNING : COLOR_ERROR);
    renderer->drawText(ss.str(), NUIPoint(x, currentY), 
                      12.0f, NUIColor::fromHex(fpsColor));
    ss.str("");
    currentY += lineHeight;
    
    // Draw calls
    ss << "Draw Calls: " << drawCallCount_;
    renderer->drawText(ss.str(), NUIPoint(x, currentY), 
                      12.0f, NUIColor::fromHex(TEXT_PRIMARY));
    ss.str("");
    currentY += lineHeight;
    
    // Triangles
    ss << "Triangles: " << triangleCount_;
    renderer->drawText(ss.str(), NUIPoint(x, currentY), 
                      12.0f, NUIColor::fromHex(TEXT_PRIMARY));
    ss.str("");
    currentY += lineHeight;
    
    // Cache hit rate
    ss << "Cache Hit Rate: " << std::fixed << std::setprecision(1) 
       << (cacheHitRate_ * 100.0f) << "%";
    uint32_t cacheColor = cacheHitRate_ >= 0.9f ? COLOR_SUCCESS : 
                         (cacheHitRate_ >= 0.8f ? COLOR_WARNING : COLOR_ERROR);
    renderer->drawText(ss.str(), NUIPoint(x, currentY), 
                      12.0f, NUIColor::fromHex(cacheColor));
    ss.str("");
    currentY += lineHeight + 5.0f;
    
    // Memory
    drawMemoryBar(renderer, x, currentY, "CPU RAM", cpuMemoryMB_, 4096);
    currentY += 35.0f;
    
    if (gpuMemoryMB_ > 0) {
        drawMemoryBar(renderer, x, currentY, "GPU VRAM", gpuMemoryMB_, 512);
    }
}

void NUIDebugOverlay::drawMemoryBar(NUIRenderer* renderer, float x, float y, 
                                    const char* label, size_t used, size_t total) {
    // Label
    std::stringstream ss;
    ss << label << ": " << used << " / " << total << " MB";
    renderer->drawText(ss.str(), NUIPoint(x, y), 
                      10.0f, NUIColor::fromHex(TEXT_SECONDARY));
    
    // Bar
    float barWidth = 300.0f;
    float barHeight = 12.0f;
    float barY = y + 14.0f;
    
    // Background
    NUIRect barBg(x, barY, barWidth, barHeight);
    renderer->fillRect(barBg, NUIColor(0.1f, 0.1f, 0.12f, 1.0f));
    
    // Fill
    float fillRatio = std::min((float)used / (float)total, 1.0f);
    float fillWidth = barWidth * fillRatio;
    
    uint32_t fillColor;
    if (fillRatio < 0.7f) {
        fillColor = COLOR_SUCCESS;
    } else if (fillRatio < 0.9f) {
        fillColor = COLOR_WARNING;
    } else {
        fillColor = COLOR_ERROR;
    }
    
    if (fillWidth > 0) {
        NUIRect barFill(x, barY, fillWidth, barHeight);
        renderer->fillRect(barFill, NUIColor::fromHex(fillColor));
    }
}

void NUIDebugOverlay::collectMetrics() {
    // Get profiler data
    auto& profiler = Nomad::Profiler::getInstance();
    const auto& currentFrame = profiler.getCurrentFrame();
    
    currentFPS_ = static_cast<float>(profiler.getFPS());
    drawCallCount_ = currentFrame.drawCalls;
    triangleCount_ = currentFrame.triangles;
    
    // Get CPU memory usage (Windows)
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        cpuMemoryMB_ = pmc.WorkingSetSize / (1024 * 1024);
    }
#endif
    
    // Get GPU memory usage (NVIDIA)
    GLint availMemKB = 0;
    glGetIntegerv(0x9049, &availMemKB); // GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX
    if (availMemKB > 0) {
        GLint totalMemKB = 0;
        glGetIntegerv(0x9048, &totalMemKB); // GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
        if (totalMemKB > 0) {
            gpuMemoryMB_ = (totalMemKB - availMemKB) / 1024;
        }
    }
    
    // Cache hit rate (would need to be passed from renderer)
    // For now, use a placeholder
    cacheHitRate_ = 0.85f; // TODO: Get from NUIRenderCache
}

void NUIDebugOverlay::updateFrameTimeHistory() {
    auto& profiler = Nomad::Profiler::getInstance();
    const auto& currentFrame = profiler.getCurrentFrame();
    
    float frameTime = static_cast<float>(currentFrame.totalTimeMs);
    
    // Add to history
    frameTimesMs_[frameIndex_] = frameTime;
    frameIndex_ = (frameIndex_ + 1) % FRAME_HISTORY;
    
    // Update stats
    float sum = 0.0f;
    minFrameTime_ = 999.0f;
    maxFrameTime_ = 0.0f;
    
    for (int i = 0; i < FRAME_HISTORY; ++i) {
        float t = frameTimesMs_[i];
        sum += t;
        minFrameTime_ = std::min(minFrameTime_, t);
        maxFrameTime_ = std::max(maxFrameTime_, t);
    }
    
    avgFrameTime_ = sum / FRAME_HISTORY;
}

} // namespace NomadUI
