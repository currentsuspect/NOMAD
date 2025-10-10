#pragma once

#include <string>
#include <vector>
#include <array>
#include "glm.hpp"
#include <glad/glad.h>

/**
 * Modern GPU-driven MSDF text renderer using OpenGL 3.3+
 * 
 * Features:
 * - Multi-channel signed distance field (MSDF) text rendering
 * - Crisp text at any scale with proper anti-aliasing
 * - Support for outlines and glow effects
 * - Single packed atlas texture for all ASCII characters (32-126)
 * - Efficient GPU batching with minimal state changes
 * - FreeType integration for font loading and outline extraction
 */
class TextRenderer {
public:
    /**
     * Glyph metrics structure for atlas-packed characters
     */
    struct Glyph {
        float advance;           // Horizontal advance to next glyph
        glm::vec2 bearing;      // Offset from baseline (bearingX, bearingY)
        glm::vec2 size;         // Glyph dimensions (width, height)
        glm::vec4 uv;           // UV coordinates in atlas (u0, v0, u1, v1)
        int atlasPage;          // Atlas page (always 0 for single page)
    };

    /**
     * Text vertex structure for batching
     */
    struct TextVertex {
        glm::vec2 position;     // Screen position
        glm::vec2 uv;           // Texture coordinates
        glm::vec4 color;        // RGBA color
    };

    TextRenderer();
    ~TextRenderer();

    // Prevent copying
    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    /**
     * Initialize the text renderer with a font
     * @param font_path Path to TTF/OTF font file
     * @param font_px_height Font size in pixels
     * @param atlas_size Size of the packed atlas texture (default 2048)
     * @return true if initialization successful
     */
    bool init(const std::string& font_path, int font_px_height, int atlas_size = 2048);

    /**
     * Set MSDF parameters for rendering quality
     * @param pxRange Distance field range in pixels (affects sharpness)
     * @param smoothing Additional smoothing factor (0.0 = sharp, 1.0 = smooth)
     */
    void setSDFParams(float pxRange, float smoothing);

    /**
     * Draw text at specified position
     * @param x X position in screen coordinates
     * @param y Y position in screen coordinates
     * @param text Text string to render
     * @param color RGBA color
     * @param scale Scale factor (1.0 = normal size)
     */
    void drawText(float x, float y, const std::string& text, const glm::vec4& color, float scale = 1.0f);

    /**
     * Measure text dimensions
     * @param text Text to measure
     * @param outWidth Output width
     * @param outHeight Output height
     * @param scale Scale factor
     */
    void measureText(const std::string& text, float& outWidth, float& outHeight, float scale = 1.0f);

    /**
     * Cleanup resources
     */
    void cleanup();

    /**
     * Check if renderer is initialized
     */
    bool isInitialized() const { return initialized_; }

    /**
     * Get font metrics
     */
    float getLineHeight() const { return lineHeight_; }
    float getAscender() const { return ascender_; }
    float getDescender() const { return descender_; }

private:
    // MSDF generation and atlas packing
    bool generateMSDFAtlas(const std::string& font_path, int font_px_height, int atlas_size);
    bool loadFont(const std::string& font_path, int font_px_height);
    bool generateGlyphMSDF(uint32_t character, int& width, int& height, std::vector<uint8_t>& data);
    bool packAtlas(int atlas_size);
    
    // OpenGL setup
    bool createShaders();
    bool createBuffers();
    void setupVertexAttributes();
    
    // Rendering
    void beginBatch();
    void endBatch();
    void flush();
    void addGlyphQuad(float x, float y, const Glyph& glyph, const glm::vec4& color, float scale);
    
    // Shader compilation
    GLuint compileShader(const std::string& source, GLenum type);
    GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader);
    
    // State
    bool initialized_;
    int fontPxHeight_;
    int atlasSize_;
    float pxRange_;
    float smoothing_;
    
    // Font metrics
    float lineHeight_;
    float ascender_;
    float descender_;
    
    // Glyph data (ASCII 32-126 = 95 characters)
    std::array<Glyph, 95> glyphs_;
    
    // OpenGL resources
    GLuint vao_;
    GLuint vbo_;
    GLuint ebo_;
    GLuint shaderProgram_;
    GLuint atlasTexture_;
    
    // Shader uniforms
    GLint projectionLoc_;
    GLint atlasLoc_;
    GLint colorLoc_;
    GLint pxRangeLoc_;
    GLint smoothingLoc_;
    GLint scaleLoc_;
    
    // Batching
    std::vector<TextVertex> vertices_;
    std::vector<uint32_t> indices_;
    bool batching_;
    
    // Projection matrix
    glm::mat4 projectionMatrix_;
    
    // FreeType (if available)
    void* ftLibrary_;
    void* ftFace_;
    
    // MSDF generation (if msdfgen not available, use integrated implementation)
    bool useIntegratedMSDF_;
};