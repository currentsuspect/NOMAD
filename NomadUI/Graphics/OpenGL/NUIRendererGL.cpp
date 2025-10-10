#include "NUIRendererGL.h"
#include "../NUITextRenderer.h"
#include "../NUIFont.h"
#include "../NUITextRendererGDI.h"
#include "../NUITextRendererModern.h"
#include <cstring>
#include <cmath>
#include <iostream>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #define NOCOMM
    #include <Windows.h>
#endif

// GLAD must be included after Windows headers to avoid macro conflicts
#include <glad/glad.h>

// Suppress APIENTRY redefinition warning - both define the same value
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4005)
// Windows.h redefines APIENTRY but it's the same value, so we can ignore the warning
#pragma warning(pop)
#endif

namespace NomadUI {

// ============================================================================
// Shader Sources (embedded)
// ============================================================================

static const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

out vec2 vTexCoord;
out vec4 vColor;

uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
    vColor = aColor;
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
in vec2 vTexCoord;
in vec4 vColor;

out vec4 FragColor;

uniform int uPrimitiveType;
uniform float uRadius;
uniform vec2 uSize;

float sdRoundedRect(vec2 p, vec2 size, float radius) {
    vec2 d = abs(p) - size + radius;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

void main() {
    vec4 color = vColor;
    
    if (uPrimitiveType == 1) {
        // Rounded rectangle
        vec2 center = uSize * 0.5;
        vec2 pos = vTexCoord * uSize;
        float dist = sdRoundedRect(pos - center, center, uRadius);
        float alpha = 1.0 - smoothstep(-1.0, 1.0, dist);
        color.a *= alpha;
        if (color.a < 0.01) discard;
    }
    
    FragColor = color;
}
)";

// ============================================================================
// Constructor / Destructor
// ============================================================================

NUIRendererGL::NUIRendererGL() {
}

NUIRendererGL::~NUIRendererGL() {
    shutdown();
}

// ============================================================================
// Lifecycle
// ============================================================================

bool NUIRendererGL::initialize(int width, int height) {
    width_ = width;
    height_ = height;
    
    if (!initializeGL()) {
        return false;
    }
    
    if (!loadShaders()) {
        return false;
    }
    
    createBuffers();
    updateProjectionMatrix();
    
    // Initialize text rendering
    textRenderer_ = std::make_unique<NUITextRenderer>();
    if (!textRenderer_->initialize()) {
        std::cerr << "Failed to initialize text renderer" << std::endl;
        return false;
    }
    
    // Load default font
    defaultFont_ = std::make_shared<NUIFont>();
    if (!defaultFont_->loadFromFile("C:/Windows/Fonts/arial.ttf", 16)) {
        // Fallback to system font
        if (!defaultFont_->loadFromFile("C:/Windows/Fonts/calibri.ttf", 16)) {
            std::cerr << "Warning: Could not load default font, text rendering will use placeholders" << std::endl;
        }
    }
    
    // Don't cache ASCII glyphs during initialization to avoid crashes
    defaultFont_->cacheASCII();
    
    // Initialize modern text renderer (primary) - only if SDL2 is available
    #ifdef NOMADUI_SDL2_AVAILABLE
    modernTextRenderer_ = std::make_shared<NUITextRendererModern>();
    if (!modernTextRenderer_->initialize()) {
        std::cerr << "Warning: Modern text renderer failed to initialize" << std::endl;
    } else {
        // Load default font
        if (!modernTextRenderer_->loadFont("C:/Windows/Fonts/arial.ttf", 16)) {
            if (!modernTextRenderer_->loadFont("C:/Windows/Fonts/calibri.ttf", 16)) {
                std::cerr << "Warning: Could not load default font for modern renderer" << std::endl;
            }
        }
    }
    #else
    std::cout << "SDL2 not available - using GDI text rendering fallback" << std::endl;
    #endif
    
    // Initialize GDI text renderer as fallback
    gdiTextRenderer_ = std::make_shared<NUITextRendererGDI>();
    gdiTextRenderer_->initialize();
    
    // Set initial state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    return true;
}

void NUIRendererGL::shutdown() {
    // Shutdown text renderer
    if (textRenderer_) {
        textRenderer_->shutdown();
        textRenderer_.reset();
    }
    
    defaultFont_.reset();
    
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    
    if (ebo_) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    
    if (primitiveShader_.id) {
        glDeleteProgram(primitiveShader_.id);
        primitiveShader_.id = 0;
    }
}

void NUIRendererGL::resize(int width, int height) {
    width_ = width;
    height_ = height;
    updateProjectionMatrix();
    
    // Update modern text renderer viewport
    if (modernTextRenderer_) {
        modernTextRenderer_->setViewport(width, height);
    }
    
    glViewport(0, 0, width, height);
}

// ============================================================================
// Frame Management
// ============================================================================

void NUIRendererGL::beginFrame() {
    vertices_.clear();
    indices_.clear();
}

void NUIRendererGL::endFrame() {
    flush();
}

void NUIRendererGL::clear(const NUIColor& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

// ============================================================================
// State Management
// ============================================================================

void NUIRendererGL::pushTransform(float tx, float ty, float rotation, float scale) {
    Transform t;
    t.tx = tx;
    t.ty = ty;
    t.rotation = rotation;
    t.scale = scale;
    transformStack_.push_back(t);
}

void NUIRendererGL::popTransform() {
    if (!transformStack_.empty()) {
        transformStack_.pop_back();
    }
}

void NUIRendererGL::setClipRect(const NUIRect& rect) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(
        static_cast<int>(rect.x),
        static_cast<int>(height_ - rect.y - rect.height),
        static_cast<int>(rect.width),
        static_cast<int>(rect.height)
    );
}

void NUIRendererGL::clearClipRect() {
    glDisable(GL_SCISSOR_TEST);
}

void NUIRendererGL::setOpacity(float opacity) {
    globalOpacity_ = opacity;
}

// ============================================================================
// Primitive Drawing
// ============================================================================

void NUIRendererGL::fillRect(const NUIRect& rect, const NUIColor& color) {
    addQuad(rect, color);
}

void NUIRendererGL::fillRoundedRect(const NUIRect& rect, float radius, const NUIColor& color) {
    // For now, use simple rect
    // TODO: Implement proper rounded rect with shader
    addQuad(rect, color);
}

void NUIRendererGL::strokeRect(const NUIRect& rect, float thickness, const NUIColor& color) {
    // Draw 4 lines
    drawLine({rect.x, rect.y}, {rect.right(), rect.y}, thickness, color);
    drawLine({rect.right(), rect.y}, {rect.right(), rect.bottom()}, thickness, color);
    drawLine({rect.right(), rect.bottom()}, {rect.x, rect.bottom()}, thickness, color);
    drawLine({rect.x, rect.bottom()}, {rect.x, rect.y}, thickness, color);
}

void NUIRendererGL::strokeRoundedRect(const NUIRect& rect, float radius, float thickness, const NUIColor& color) {
    // For now, use simple stroke
    strokeRect(rect, thickness, color);
}

void NUIRendererGL::fillCircle(const NUIPoint& center, float radius, const NUIColor& color) {
    // Approximate circle with triangle fan
    const int segments = 32;
    const float angleStep = 2.0f * 3.14159f / segments;
    
    for (int i = 0; i < segments; ++i) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;
        
        addVertex(center.x, center.y, 0.5f, 0.5f, color);
        addVertex(center.x + std::cos(angle1) * radius, center.y + std::sin(angle1) * radius, 0, 0, color);
        addVertex(center.x + std::cos(angle2) * radius, center.y + std::sin(angle2) * radius, 1, 1, color);
    }
}

void NUIRendererGL::strokeCircle(const NUIPoint& center, float radius, float thickness, const NUIColor& color) {
    // Approximate with line segments
    const int segments = 32;
    const float angleStep = 2.0f * 3.14159f / segments;
    
    for (int i = 0; i < segments; ++i) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;
        
        NUIPoint p1(center.x + std::cos(angle1) * radius, center.y + std::sin(angle1) * radius);
        NUIPoint p2(center.x + std::cos(angle2) * radius, center.y + std::sin(angle2) * radius);
        
        drawLine(p1, p2, thickness, color);
    }
}

void NUIRendererGL::drawLine(const NUIPoint& start, const NUIPoint& end, float thickness, const NUIColor& color) {
    // Simple line as thin quad
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float len = std::sqrt(dx * dx + dy * dy);
    
    if (len < 0.001f) return;
    
    float nx = -dy / len * thickness * 0.5f;
    float ny = dx / len * thickness * 0.5f;
    
    addVertex(start.x + nx, start.y + ny, 0, 0, color);
    addVertex(start.x - nx, start.y - ny, 0, 1, color);
    addVertex(end.x - nx, end.y - ny, 1, 1, color);
    addVertex(end.x + nx, end.y + ny, 1, 0, color);
    
    uint32_t base = static_cast<uint32_t>(vertices_.size()) - 4;
    indices_.push_back(base + 0);
    indices_.push_back(base + 1);
    indices_.push_back(base + 2);
    indices_.push_back(base + 0);
    indices_.push_back(base + 2);
    indices_.push_back(base + 3);
}

void NUIRendererGL::drawPolyline(const NUIPoint* points, int count, float thickness, const NUIColor& color) {
    for (int i = 0; i < count - 1; ++i) {
        drawLine(points[i], points[i + 1], thickness, color);
    }
}

// ============================================================================
// Gradient Drawing
// ============================================================================

void NUIRendererGL::fillRectGradient(const NUIRect& rect, const NUIColor& colorStart, const NUIColor& colorEnd, bool vertical) {
    if (vertical) {
        addVertex(rect.x, rect.y, 0, 0, colorStart);
        addVertex(rect.right(), rect.y, 1, 0, colorStart);
        addVertex(rect.right(), rect.bottom(), 1, 1, colorEnd);
        addVertex(rect.x, rect.bottom(), 0, 1, colorEnd);
    } else {
        addVertex(rect.x, rect.y, 0, 0, colorStart);
        addVertex(rect.right(), rect.y, 1, 0, colorEnd);
        addVertex(rect.right(), rect.bottom(), 1, 1, colorEnd);
        addVertex(rect.x, rect.bottom(), 0, 1, colorStart);
    }
    
    uint32_t base = static_cast<uint32_t>(vertices_.size()) - 4;
    indices_.push_back(base + 0);
    indices_.push_back(base + 1);
    indices_.push_back(base + 2);
    indices_.push_back(base + 0);
    indices_.push_back(base + 2);
    indices_.push_back(base + 3);
}

void NUIRendererGL::fillCircleGradient(const NUIPoint& center, float radius, const NUIColor& colorInner, const NUIColor& colorOuter) {
    // Simple radial gradient
    const int segments = 32;
    const float angleStep = 2.0f * 3.14159f / segments;
    
    for (int i = 0; i < segments; ++i) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;
        
        addVertex(center.x, center.y, 0.5f, 0.5f, colorInner);
        addVertex(center.x + std::cos(angle1) * radius, center.y + std::sin(angle1) * radius, 0, 0, colorOuter);
        addVertex(center.x + std::cos(angle2) * radius, center.y + std::sin(angle2) * radius, 1, 1, colorOuter);
    }
}

// ============================================================================
// Effects (Simplified for now)
// ============================================================================

void NUIRendererGL::drawGlow(const NUIRect& rect, float radius, float intensity, const NUIColor& color) {
    // Simple glow as expanded semi-transparent rect
    NUIRect glowRect = rect;
    glowRect.x -= radius;
    glowRect.y -= radius;
    glowRect.width += radius * 2;
    glowRect.height += radius * 2;
    
    NUIColor glowColor = color;
    glowColor.a *= intensity * 0.3f;
    
    fillRect(glowRect, glowColor);
}

void NUIRendererGL::drawShadow(const NUIRect& rect, float offsetX, float offsetY, float blur, const NUIColor& color) {
    NUIRect shadowRect = rect;
    shadowRect.x += offsetX;
    shadowRect.y += offsetY;
    
    NUIColor shadowColor = color;
    shadowColor.a *= 0.5f;
    
    fillRect(shadowRect, shadowColor);
}

// ============================================================================
// Text Rendering (Placeholder)
// ============================================================================

void NUIRendererGL::drawText(const std::string& text, const NUIPoint& position, float fontSize, const NUIColor& color) {
    // Try modern OpenGL text rendering first (only if SDL2 is available)
    #ifdef NOMADUI_SDL2_AVAILABLE
    if (modernTextRenderer_) {
        // For now, use fixed font size - in production, create separate atlases per size
        modernTextRenderer_->setViewport(width_, height_);
        modernTextRenderer_->drawText(text, position, color);
        return;
    }
    #endif
    
    // Fallback to GDI text rendering (Windows)
    #ifdef _WIN32
    if (gdiTextRenderer_) {
        // Get device context from current OpenGL context
        HDC hdc = wglGetCurrentDC();
        if (hdc) {
            gdiTextRenderer_->drawText(text, position, fontSize, color, hdc);
        }
    }
    #else
    // Fallback to legacy OpenGL text rendering on other platforms
    if (textRenderer_ && defaultFont_) {
        // Get font at requested size
        auto font = NUIFontManager::getInstance().getFont(defaultFont_->getFilepath(), static_cast<int>(fontSize));
        if (!font) font = defaultFont_;
        
        // Begin text batch
        textRenderer_->setOpacity(globalOpacity_);
        textRenderer_->beginBatch(projectionMatrix_);
        
        // Draw text
        textRenderer_->drawText(text, font, position, color);
        
        // End batch and flush
        textRenderer_->endBatch();
    }
    #endif
}

void NUIRendererGL::drawTextCentered(const std::string& text, const NUIRect& rect, float fontSize, const NUIColor& color) {
    if (!textRenderer_ || !defaultFont_) {
        // Fallback to placeholder
        float textWidth = text.length() * fontSize * 0.6f;
        float x = rect.x + (rect.width - textWidth) * 0.5f;
        float y = rect.y + (rect.height - fontSize) * 0.5f;
        NUIRect textRect(x, y, textWidth, fontSize);
        fillRect(textRect, color.withAlpha(0.3f));
        return;
    }
    
    // Get font at requested size
    auto font = NUIFontManager::getInstance().getFont(defaultFont_->getFilepath(), static_cast<int>(fontSize));
    if (!font) font = defaultFont_;
    
    // Begin text batch
    textRenderer_->setOpacity(globalOpacity_);
    textRenderer_->beginBatch(projectionMatrix_);
    
    // Draw centered text
    textRenderer_->drawTextAligned(
        text, 
        font, 
        rect, 
        color,
        NUITextRenderer::Alignment::Center,
        NUITextRenderer::VerticalAlignment::Middle
    );
    
    // End batch and flush
    textRenderer_->endBatch();
}

NUISize NUIRendererGL::measureText(const std::string& text, float fontSize) {
    if (!textRenderer_ || !defaultFont_) {
        // Fallback estimation
        return {text.length() * fontSize * 0.6f, fontSize};
    }
    
    // Get font at requested size
    auto font = NUIFontManager::getInstance().getFont(defaultFont_->getFilepath(), static_cast<int>(fontSize));
    if (!font) font = defaultFont_;
    
    return textRenderer_->measureText(text, font);
}

// ============================================================================
// Texture/Image Drawing (Placeholder)
// ============================================================================

void NUIRendererGL::drawTexture(uint32_t textureId, const NUIRect& destRect, const NUIRect& sourceRect) {
    // TODO: Implement
}

uint32_t NUIRendererGL::loadTexture(const std::string& filepath) {
    // TODO: Implement
    return 0;
}

uint32_t NUIRendererGL::createTexture(const uint8_t* data, int width, int height) {
    // TODO: Implement
    return 0;
}

void NUIRendererGL::deleteTexture(uint32_t textureId) {
    // TODO: Implement
}

// ============================================================================
// Batching
// ============================================================================

void NUIRendererGL::beginBatch() {
    batching_ = true;
}

void NUIRendererGL::endBatch() {
    batching_ = false;
    flush();
}

void NUIRendererGL::flush() {
    if (vertices_.empty()) {
        return;
    }
    
    // Upload vertex data
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), vertices_.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(uint32_t), indices_.data(), GL_DYNAMIC_DRAW);
    
    // Use shader
    glUseProgram(primitiveShader_.id);
    glUniformMatrix4fv(primitiveShader_.projectionLoc, 1, GL_FALSE, projectionMatrix_);
    // Note: opacity is already in vertex colors
    glUniform1i(primitiveShader_.primitiveTypeLoc, 0); // Simple rect
    
    // Draw
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, 0);
    
    // Clear for next batch
    vertices_.clear();
    indices_.clear();
}

// ============================================================================
// Private Helpers
// ============================================================================

bool NUIRendererGL::initializeGL() {
    // Load OpenGL functions with GLAD
    if (!gladLoadGL()) {
        // Failed to load OpenGL functions
        return false;
    }
    
    // OpenGL context should already be created by platform layer
    return true;
}

bool NUIRendererGL::loadShaders() {
    uint32_t vertShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    uint32_t fragShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    if (!vertShader || !fragShader) {
        return false;
    }
    
    primitiveShader_.id = linkProgram(vertShader, fragShader);
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    
    if (!primitiveShader_.id) {
        return false;
    }
    
    // Get uniform locations
    primitiveShader_.projectionLoc = glGetUniformLocation(primitiveShader_.id, "uProjection");
    // Note: opacity is baked into vertex colors, no uniform needed
    primitiveShader_.primitiveTypeLoc = glGetUniformLocation(primitiveShader_.id, "uPrimitiveType");
    primitiveShader_.radiusLoc = glGetUniformLocation(primitiveShader_.id, "uRadius");
    primitiveShader_.sizeLoc = glGetUniformLocation(primitiveShader_.id, "uSize");
    
    return true;
}

void NUIRendererGL::createBuffers() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    
    glBindVertexArray(vao_);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    
    // TexCoord
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));
    
    // Color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    
    glBindVertexArray(0);
}

uint32_t NUIRendererGL::compileShader(const char* source, uint32_t type) {
    uint32_t shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        // TODO: Log error
        return 0;
    }
    
    return shader;
}

uint32_t NUIRendererGL::linkProgram(uint32_t vertexShader, uint32_t fragmentShader) {
    uint32_t program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        // TODO: Log error
        return 0;
    }
    
    return program;
}

void NUIRendererGL::addVertex(float x, float y, float u, float v, const NUIColor& color) {
    applyTransform(x, y);
    
    Vertex vertex;
    vertex.x = x;
    vertex.y = y;
    vertex.u = u;
    vertex.v = v;
    vertex.r = color.r;
    vertex.g = color.g;
    vertex.b = color.b;
    vertex.a = color.a * globalOpacity_;
    
    vertices_.push_back(vertex);
}

void NUIRendererGL::addQuad(const NUIRect& rect, const NUIColor& color) {
    addVertex(rect.x, rect.y, 0, 0, color);
    addVertex(rect.right(), rect.y, 1, 0, color);
    addVertex(rect.right(), rect.bottom(), 1, 1, color);
    addVertex(rect.x, rect.bottom(), 0, 1, color);
    
    uint32_t base = static_cast<uint32_t>(vertices_.size()) - 4;
    indices_.push_back(base + 0);
    indices_.push_back(base + 1);
    indices_.push_back(base + 2);
    indices_.push_back(base + 0);
    indices_.push_back(base + 2);
    indices_.push_back(base + 3);
}

void NUIRendererGL::applyTransform(float& x, float& y) {
    if (transformStack_.empty()) {
        return;
    }
    
    for (const auto& t : transformStack_) {
        x += t.tx;
        y += t.ty;
        x *= t.scale;
        y *= t.scale;
        // TODO: Apply rotation
    }
}

void NUIRendererGL::updateProjectionMatrix() {
    // Orthographic projection matrix
    float left = 0.0f;
    float right = static_cast<float>(width_);
    float bottom = static_cast<float>(height_);
    float top = 0.0f;
    float nearPlane = -1.0f;
    float farPlane = 1.0f;
    
    std::memset(projectionMatrix_, 0, sizeof(projectionMatrix_));
    
    projectionMatrix_[0] = 2.0f / (right - left);
    projectionMatrix_[5] = 2.0f / (top - bottom);
    projectionMatrix_[10] = -2.0f / (farPlane - nearPlane);
    projectionMatrix_[12] = -(right + left) / (right - left);
    projectionMatrix_[13] = -(top + bottom) / (top - bottom);
    projectionMatrix_[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    projectionMatrix_[15] = 1.0f;
}

} // namespace NomadUI
