#pragma once

#include "../NUIRenderer.h"
#include <vector>
#include <unordered_map>

namespace NomadUI {

/**
 * OpenGL 3.3+ renderer implementation for Nomad UI.
 * 
 * Features:
 * - Shader-based rendering
 * - Batched draw calls
 * - SDF text rendering
 * - Post-processing effects
 */
class NUIRendererGL : public NUIRenderer {
public:
    NUIRendererGL();
    ~NUIRendererGL() override;
    
    // ========================================================================
    // Lifecycle
    // ========================================================================
    
    bool initialize(int width, int height) override;
    void shutdown() override;
    void resize(int width, int height) override;
    
    // ========================================================================
    // Frame Management
    // ========================================================================
    
    void beginFrame() override;
    void endFrame() override;
    void clear(const NUIColor& color) override;
    
    // ========================================================================
    // State Management
    // ========================================================================
    
    void pushTransform(float tx, float ty, float rotation = 0.0f, float scale = 1.0f) override;
    void popTransform() override;
    void setClipRect(const NUIRect& rect) override;
    void clearClipRect() override;
    void setOpacity(float opacity) override;
    
    // ========================================================================
    // Primitive Drawing
    // ========================================================================
    
    void fillRect(const NUIRect& rect, const NUIColor& color) override;
    void fillRoundedRect(const NUIRect& rect, float radius, const NUIColor& color) override;
    void strokeRect(const NUIRect& rect, float thickness, const NUIColor& color) override;
    void strokeRoundedRect(const NUIRect& rect, float radius, float thickness, const NUIColor& color) override;
    void fillCircle(const NUIPoint& center, float radius, const NUIColor& color) override;
    void strokeCircle(const NUIPoint& center, float radius, float thickness, const NUIColor& color) override;
    void drawLine(const NUIPoint& start, const NUIPoint& end, float thickness, const NUIColor& color) override;
    void drawPolyline(const NUIPoint* points, int count, float thickness, const NUIColor& color) override;
    
    // ========================================================================
    // Gradient Drawing
    // ========================================================================
    
    void fillRectGradient(const NUIRect& rect, const NUIColor& colorStart, const NUIColor& colorEnd, bool vertical = true) override;
    void fillCircleGradient(const NUIPoint& center, float radius, const NUIColor& colorInner, const NUIColor& colorOuter) override;
    
    // ========================================================================
    // Effects
    // ========================================================================
    
    void drawGlow(const NUIRect& rect, float radius, float intensity, const NUIColor& color) override;
    void drawShadow(const NUIRect& rect, float offsetX, float offsetY, float blur, const NUIColor& color) override;
    
    // ========================================================================
    // Text Rendering
    // ========================================================================
    
    void drawText(const std::string& text, const NUIPoint& position, float fontSize, const NUIColor& color) override;
    void drawTextCentered(const std::string& text, const NUIRect& rect, float fontSize, const NUIColor& color) override;
    NUISize measureText(const std::string& text, float fontSize) override;
    
    // ========================================================================
    // Texture/Image Drawing
    // ========================================================================
    
    void drawTexture(uint32_t textureId, const NUIRect& destRect, const NUIRect& sourceRect) override;
    uint32_t loadTexture(const std::string& filepath) override;
    uint32_t createTexture(const uint8_t* data, int width, int height) override;
    void deleteTexture(uint32_t textureId) override;
    
    // ========================================================================
    // Batching
    // ========================================================================
    
    void beginBatch() override;
    void endBatch() override;
    void flush() override;
    
    // ========================================================================
    // Info
    // ========================================================================
    
    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }
    const char* getBackendName() const override { return "OpenGL 3.3+"; }
    
private:
    // Vertex structure for batching
    struct Vertex {
        float x, y;
        float u, v;
        float r, g, b, a;
    };
    
    // Shader program
    struct ShaderProgram {
        uint32_t id = 0;
        int32_t projectionLoc = -1;
        int32_t transformLoc = -1;
        int32_t opacityLoc = -1;
        int32_t primitiveTypeLoc = -1;
        int32_t radiusLoc = -1;
        int32_t sizeLoc = -1;
    };
    
    // Transform stack
    struct Transform {
        float tx = 0.0f;
        float ty = 0.0f;
        float rotation = 0.0f;
        float scale = 1.0f;
    };
    
    // Initialize OpenGL resources
    bool initializeGL();
    bool loadShaders();
    void createBuffers();
    
    // Shader helpers
    uint32_t compileShader(const char* source, uint32_t type);
    uint32_t linkProgram(uint32_t vertexShader, uint32_t fragmentShader);
    
    // Drawing helpers
    void addVertex(float x, float y, float u, float v, const NUIColor& color);
    void addQuad(const NUIRect& rect, const NUIColor& color);
    void applyTransform(float& x, float& y);
    void updateProjectionMatrix();
    
    // State
    int width_ = 0;
    int height_ = 0;
    float globalOpacity_ = 1.0f;
    bool batching_ = false;
    
    // OpenGL objects
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    uint32_t ebo_ = 0;
    
    // Shaders
    ShaderProgram primitiveShader_;
    ShaderProgram textShader_;
    
    // Vertex batch
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    
    // Transform stack
    std::vector<Transform> transformStack_;
    
    // Textures
    std::unordered_map<uint32_t, uint32_t> textures_;
    uint32_t nextTextureId_ = 1;
    
    // Projection matrix (orthographic)
    float projectionMatrix_[16];
};

} // namespace NomadUI
