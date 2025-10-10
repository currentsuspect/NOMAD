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
    initializeTextRendering();
    
    // Set initial state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    return true;
}

void NUIRendererGL::shutdown() {
    // MSDF text renderer cleanup handled externally
    
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
    
    // Clean up fonts
    fontCache_.clear();
    defaultFont_.reset();
}

void NUIRendererGL::initializeTextRendering() {
    // Try to load a default font
    // For now, we'll create a simple font system
    defaultFontPath_ = "C:/Windows/Fonts/arial.ttf"; // Windows default font
    
    // Create a basic font object
    defaultFont_ = std::make_shared<NUIFont>();
    if (defaultFont_) {
        // Initialize with basic properties
        defaultFont_->setSize(16.0f);
        fontCache_["default"] = defaultFont_;
    }
}

void NUIRendererGL::resize(int width, int height) {
    width_ = width;
    height_ = height;
    updateProjectionMatrix();
    
    // MSDF text renderer viewport will be updated externally
    
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
    drawLine(NUIPoint({rect.x, rect.y}), NUIPoint({rect.right(), rect.y}), color, thickness);
    drawLine(NUIPoint({rect.right(), rect.y}), NUIPoint({rect.right(), rect.bottom()}), color, thickness);
    drawLine(NUIPoint({rect.right(), rect.bottom()}), NUIPoint({rect.x, rect.bottom()}), color, thickness);
    drawLine(NUIPoint({rect.x, rect.bottom()}), NUIPoint({rect.x, rect.y}), color, thickness);
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
    // Simple text rendering using basic OpenGL text
    // Create a simple bitmap font representation
    float charWidth = fontSize * 0.6f;
    float charHeight = fontSize;
    
    // Set up text rendering state
    glUseProgram(primitiveShader_.id);
    glBindVertexArray(vao_);
    
    // Draw each character as a series of lines to form readable text
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (c >= 32 && c <= 126) { // Printable ASCII characters
            float x = position.x + i * charWidth;
            float y = position.y;
            
            // Draw character using simple line patterns
            drawCharacter(c, x, y, charWidth, charHeight, color);
        }
    }
}

void NUIRendererGL::drawCharacter(char c, float x, float y, float width, float height, const NUIColor& color) {
    // Simple character rendering using basic shapes
    // This creates a basic bitmap font representation
    
    float charWidth = width * 0.8f;
    float charHeight = height;
    float lineWidth = charWidth * 0.1f; // Thickness of lines
    
    // Adjust height based on character type
    if (c >= 'a' && c <= 'z') charHeight *= 0.8f; // lowercase
    else if (c >= 'A' && c <= 'Z') charHeight *= 0.9f; // uppercase
    else if (c >= '0' && c <= '9') charHeight *= 0.85f; // numbers
    else if (c == ' ') return; // space - don't draw anything
    else if (c == '.' || c == ',' || c == ';' || c == ':') charHeight *= 0.4f; // punctuation
    
    // Center the character vertically
    float charY = y + (height - charHeight) * 0.5f;
    
    // Draw character patterns based on ASCII value
    switch (c) {
        case 'A':
        case 'a':
            // A shape: /\ and -
            drawLine(NUIPoint(NUIPoint(x + charWidth*0.1f, charY + charHeight)), NUIPoint(NUIPoint(x + charWidth*0.5f, charY)), color, lineWidth);
            drawLine(NUIPoint(NUIPoint(x + charWidth*0.5f, charY)), NUIPoint(NUIPoint(x + charWidth*0.9f, charY + charHeight)), color, lineWidth);
            drawLine(NUIPoint(NUIPoint(x + charWidth*0.2f, charY + charHeight*0.5f)), NUIPoint(NUIPoint(x + charWidth*0.8f, charY + charHeight*0.5f)), color, lineWidth);
            break;
        case 'B':
        case 'b':
            // B shape: | and curves
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.7f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.7f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.7f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.7f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.25f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.7f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.75f), lineWidth, color);
            break;
        case 'C':
        case 'c':
            // C shape: curve
            drawLine(NUIPoint(x + charWidth*0.7f, charY), NUIPoint(x + charWidth*0.1f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.7f, charY + charHeight), lineWidth, color);
            break;
        case 'D':
        case 'd':
            // D shape: | and curve
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.6f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.6f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.6f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.6f, charY + charHeight), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.5f), lineWidth, color);
            break;
        case 'E':
        case 'e':
            // E shape: | and horizontal lines
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.8f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.6f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.8f, charY + charHeight), lineWidth, color);
            break;
        case 'F':
        case 'f':
            // F shape: | and horizontal lines
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.8f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.6f, charY + charHeight*0.5f), lineWidth, color);
            break;
        case 'G':
        case 'g':
            // G shape: C with line
            drawLine(NUIPoint(x + charWidth*0.7f, charY), NUIPoint(x + charWidth*0.1f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.7f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.7f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.5f), lineWidth, color);
            break;
        case 'H':
        case 'h':
            // H shape: | | and -
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.5f), lineWidth, color);
            break;
        case 'I':
        case 'i':
            // I shape: | with top and bottom
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.9f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.5f, charY), NUIPoint(x + charWidth*0.5f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
        case 'J':
        case 'j':
            // J shape: | with curve
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.9f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.5f, charY), NUIPoint(x + charWidth*0.5f, charY + charHeight*0.8f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.5f, charY + charHeight*0.8f), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            break;
        case 'K':
        case 'k':
            // K shape: | and diagonal
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.9f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
        case 'L':
        case 'l':
            // L shape: | and -
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.8f, charY + charHeight), lineWidth, color);
            break;
        case 'M':
        case 'm':
            // M shape: | \ / |
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.5f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.5f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.9f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
        case 'N':
        case 'n':
            // N shape: | \ |
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
        case 'O':
        case 'o':
            // O shape: circle/oval
            drawLine(NUIPoint(x + charWidth*0.3f, charY), NUIPoint(x + charWidth*0.7f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.2f), NUIPoint(x + charWidth*0.1f, charY + charHeight*0.8f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY + charHeight*0.2f), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.8f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.3f, charY + charHeight), NUIPoint(x + charWidth*0.7f, charY + charHeight), lineWidth, color);
            break;
        case 'P':
        case 'p':
            // P shape: | and P
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.7f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.7f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.7f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.25f), lineWidth, color);
            break;
        case 'Q':
        case 'q':
            // Q shape: O with tail
            drawLine(NUIPoint(x + charWidth*0.3f, charY), NUIPoint(x + charWidth*0.7f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.2f), NUIPoint(x + charWidth*0.1f, charY + charHeight*0.8f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY + charHeight*0.2f), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.8f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.3f, charY + charHeight), NUIPoint(x + charWidth*0.7f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.7f, charY + charHeight*0.8f), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
        case 'R':
        case 'r':
            // R shape: P with diagonal
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.7f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.7f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.7f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.25f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.7f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
        case 'S':
        case 's':
            // S shape: S curve
            drawLine(NUIPoint(x + charWidth*0.7f, charY), NUIPoint(x + charWidth*0.1f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.7f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.7f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.7f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.7f, charY + charHeight), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            break;
        case 'T':
        case 't':
            // T shape: - and |
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.9f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.5f, charY), NUIPoint(x + charWidth*0.5f, charY + charHeight), lineWidth, color);
            break;
        case 'U':
        case 'u':
            // U shape: | | and -
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight*0.8f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.8f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.8f), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.8f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
        case 'V':
        case 'v':
            // V shape: \ /
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.5f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.5f, charY + charHeight), NUIPoint(x + charWidth*0.9f, charY), lineWidth, color);
            break;
        case 'W':
        case 'w':
            // W shape: | \ / |
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.5f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.5f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
        case 'X':
        case 'x':
            // X shape: \ /
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            break;
        case 'Y':
        case 'y':
            // Y shape: \ / and |
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.5f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.5f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.9f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.5f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.5f, charY + charHeight), lineWidth, color);
            break;
        case 'Z':
        case 'z':
            // Z shape: - \ -
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.9f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
        case '0':
            // 0 shape: oval
            drawLine(NUIPoint(x + charWidth*0.3f, charY), NUIPoint(x + charWidth*0.7f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.2f), NUIPoint(x + charWidth*0.1f, charY + charHeight*0.8f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY + charHeight*0.2f), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.8f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.3f, charY + charHeight), NUIPoint(x + charWidth*0.7f, charY + charHeight), lineWidth, color);
            break;
        case '1':
            // 1 shape: |
            drawLine(NUIPoint(x + charWidth*0.5f, charY), NUIPoint(x + charWidth*0.5f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.3f, charY), NUIPoint(x + charWidth*0.5f, charY), lineWidth, color);
            break;
        case '2':
            // 2 shape: - \ -
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.8f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.8f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.8f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.8f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.8f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.8f, charY + charHeight), lineWidth, color);
            break;
        case '3':
            // 3 shape: - | -
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.8f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.8f, charY), NUIPoint(x + charWidth*0.8f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.8f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.8f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.8f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.8f, charY + charHeight), lineWidth, color);
            break;
        case '4':
            // 4 shape: | \ |
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.8f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.8f, charY), NUIPoint(x + charWidth*0.8f, charY + charHeight), lineWidth, color);
            break;
        case '5':
            // 5 shape: | - \ -
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.8f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.8f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.8f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.8f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.8f, charY + charHeight), lineWidth, color);
            break;
        case '6':
            // 6 shape: | - \ -
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.8f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.8f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.8f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.8f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.8f, charY + charHeight), lineWidth, color);
            break;
        case '7':
            // 7 shape: - |
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.9f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
        case '8':
            // 8 shape: | - | - |
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.9f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
        case '9':
            // 9 shape: | - | - |
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.9f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.9f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
        case '.':
            // Period: small dot
            fillRect(NUIRect(x + charWidth*0.4f, charY + charHeight*0.8f, charWidth*0.2f, charHeight*0.2f), color);
            break;
        case ',':
            // Comma: small dot with tail
            fillRect(NUIRect(x + charWidth*0.4f, charY + charHeight*0.8f, charWidth*0.2f, charHeight*0.2f), color);
            drawLine(NUIPoint(x + charWidth*0.4f, charY + charHeight*0.8f), NUIPoint(x + charWidth*0.3f, charY + charHeight), lineWidth, color);
            break;
        case ':':
            // Colon: two dots
            fillRect(NUIRect(x + charWidth*0.4f, charY + charHeight*0.3f, charWidth*0.2f, charHeight*0.2f), color);
            fillRect(NUIRect(x + charWidth*0.4f, charY + charHeight*0.7f, charWidth*0.2f, charHeight*0.2f), color);
            break;
        case ';':
            // Semicolon: dot with tail and comma
            fillRect(NUIRect(x + charWidth*0.4f, charY + charHeight*0.3f, charWidth*0.2f, charHeight*0.2f), color);
            fillRect(NUIRect(x + charWidth*0.4f, charY + charHeight*0.7f, charWidth*0.2f, charHeight*0.2f), color);
            drawLine(NUIPoint(x + charWidth*0.4f, charY + charHeight*0.7f), NUIPoint(x + charWidth*0.3f, charY + charHeight), lineWidth, color);
            break;
        case '!':
            // Exclamation: | and dot
            drawLine(NUIPoint(x + charWidth*0.5f, charY), NUIPoint(x + charWidth*0.5f, charY + charHeight*0.7f), lineWidth, color);
            fillRect(NUIRect(x + charWidth*0.4f, charY + charHeight*0.8f, charWidth*0.2f, charHeight*0.2f), color);
            break;
        case '?':
            // Question mark: ? shape
            drawLine(NUIPoint(x + charWidth*0.7f, charY), NUIPoint(x + charWidth*0.1f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight*0.3f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight*0.3f), NUIPoint(x + charWidth*0.5f, charY + charHeight*0.5f), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.5f, charY + charHeight*0.5f), NUIPoint(x + charWidth*0.5f, charY + charHeight*0.7f), lineWidth, color);
            fillRect(NUIRect(x + charWidth*0.4f, charY + charHeight*0.8f, charWidth*0.2f, charHeight*0.2f), color);
            break;
        case ' ':
            // Space: nothing
            break;
        default:
            // Unknown character: draw a box
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.9f, charY), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY), NUIPoint(x + charWidth*0.1f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.9f, charY), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            drawLine(NUIPoint(x + charWidth*0.1f, charY + charHeight), NUIPoint(x + charWidth*0.9f, charY + charHeight), lineWidth, color);
            break;
    }
}

void NUIRendererGL::drawTextCentered(const std::string& text, const NUIRect& rect, float fontSize, const NUIColor& color) {
    // Calculate text position for centering
    float textWidth = text.length() * fontSize * 0.6f;
    float x = rect.x + (rect.width - textWidth) * 0.5f;
    float y = rect.y + (rect.height - fontSize) * 0.5f;
    
    // Use the improved text rendering
    drawText(text, NUIPoint(x, y), fontSize, color);
}

NUISize NUIRendererGL::measureText(const std::string& text, float fontSize) {
    // Placeholder implementation - will be replaced with MSDF text renderer
    return {text.length() * fontSize * 0.6f, fontSize};
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
