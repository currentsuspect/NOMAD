// © 2025 Nomad Studios — All Rights Reserved. Licensed for personal & educational use only.
#pragma once

#include "../Core/NUITypes.h"
#include <string>
#include <unordered_map>

namespace NomadUI {

/**
 * GPU-accelerated MSDF/SDF renderer built with stb_truetype-generated distance fields.
 * Currently generates an SDF atlas on startup for ASCII range.
 */
class NUITextRendererSDF {
public:
    struct GlyphData {
        float u0, v0, u1, v1;
        float width, height;
        float bearingX, bearingY;
        float advance;
    };

    NUITextRendererSDF();
    ~NUITextRendererSDF();

    bool initialize(const std::string& fontPath, float fontSize);
    void shutdown();

    void drawText(const std::string& text,
                  const NUIPoint& position,
                  float fontSize,
                  const NUIColor& color,
                  const float* projection);

    NUISize measureText(const std::string& text, float fontSize) const;
    float getAscent(float fontSize) const;

    bool isInitialized() const { return initialized_; }

private:
    bool createShader();
    bool loadFontAtlas(const std::string& fontPath, float fontSize);

    bool initialized_{false};
    float baseFontSize_{16.0f};
    float atlasFontSize_{16.0f};
    float ascent_{0.0f};
    float descent_{0.0f};

    /**
 * OpenGL texture handle for the font SDF atlas.
 * @details Holds the GPU texture ID for the generated atlas; value is `0` when no atlas has been created.
 */
uint32_t atlasTexture_{0};
    /**
 * OpenGL shader program handle used for SDF text rendering.
 *
 * Holds the GPU program object identifier for the renderer's shaders.
 * A value of `0` indicates the program has not been created or has been released.
 */
uint32_t shaderProgram_{0};
    /**
 * OpenGL Vertex Array Object (VAO) handle used to store vertex attribute state for the text mesh.
 * A value of 0 indicates the VAO has not been created.
 */
uint32_t vao_{0};
    /**
 * OpenGL vertex buffer object handle used to store glyph vertex data.
 *
 * A value of 0 indicates the buffer has not been created. */
uint32_t vbo_{0};
    /**
 * OpenGL element buffer object (index buffer) handle for glyph quad indices.
 *
 * Holds the GL name for the EBO used when rendering glyph geometry; value is 0 when no buffer has been created.
 */
uint32_t ebo_{0};

    float atlasWidth_{2048.0f};   /**
 * Height of the font atlas texture in pixels used for packing glyph bitmaps.
 *
 * Defaults to 2048.0 and may be increased to accommodate more or larger glyphs
 * during atlas generation.
 */

/**
 * Mapping from ASCII character code to per-glyph metrics and texture coordinates.
 *
 * Each entry stores the GlyphData for a character used when building and sampling
 * the SDF/MSDF atlas.
 */
    float atlasHeight_{2048.0f};  // Increased for better character packing

    std::unordered_map<char, GlyphData> glyphs_;
};

} // namespace NomadUI