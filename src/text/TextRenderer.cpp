#include "TextRenderer.h"
#include "MSDFGenerator.h"
#include "AtlasPacker.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

// FreeType headers
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

TextRenderer::TextRenderer()
    : initialized_(false)
    , fontPxHeight_(16)
    , atlasSize_(2048)
    , pxRange_(4.0f)
    , smoothing_(0.5f)
    , lineHeight_(0.0f)
    , ascender_(0.0f)
    , descender_(0.0f)
    , vao_(0)
    , vbo_(0)
    , ebo_(0)
    , shaderProgram_(0)
    , atlasTexture_(0)
    , projectionLoc_(-1)
    , atlasLoc_(-1)
    , colorLoc_(-1)
    , pxRangeLoc_(-1)
    , smoothingLoc_(-1)
    , scaleLoc_(-1)
    , batching_(false)
    , ftLibrary_(nullptr)
    , ftFace_(nullptr)
    , useIntegratedMSDF_(true)
{
    // Initialize glyphs array
    for (auto& glyph : glyphs_) {
        glyph.advance = 0.0f;
        glyph.bearing = glm::vec2(0.0f);
        glyph.size = glm::vec2(0.0f);
        glyph.uv = glm::vec4(0.0f);
        glyph.atlasPage = 0;
    }
}

TextRenderer::~TextRenderer() {
    cleanup();
}

bool TextRenderer::init(const std::string& font_path, int font_px_height, int atlas_size) {
    if (initialized_) {
        cleanup();
    }

    fontPxHeight_ = font_px_height;
    atlasSize_ = atlas_size;

    std::cout << "Initializing MSDF text renderer..." << std::endl;
    std::cout << "  Font: " << font_path << std::endl;
    std::cout << "  Size: " << font_px_height << "px" << std::endl;
    std::cout << "  Atlas: " << atlas_size << "x" << atlas_size << std::endl;

    // Initialize FreeType
    if (!loadFont(font_path, font_px_height)) {
        std::cerr << "Failed to load font" << std::endl;
        return false;
    }

    // Generate MSDF atlas
    if (!generateMSDFAtlas(font_path, font_px_height, atlas_size)) {
        std::cerr << "Failed to generate MSDF atlas" << std::endl;
        return false;
    }

    // Create OpenGL resources
    if (!createShaders()) {
        std::cerr << "Failed to create shaders" << std::endl;
        return false;
    }

    if (!createBuffers()) {
        std::cerr << "Failed to create buffers" << std::endl;
        return false;
    }

    // Set up projection matrix (orthographic)
    projectionMatrix_ = glm::ortho(0.0f, 1920.0f, 1080.0f, 0.0f, -1.0f, 1.0f);

    initialized_ = true;
    std::cout << "✓ MSDF text renderer initialized successfully" << std::endl;
    return true;
}

bool TextRenderer::loadFont(const std::string& font_path, int font_px_height) {
    // Initialize FreeType
    FT_Error error = FT_Init_FreeType((FT_Library*)&ftLibrary_);
    if (error) {
        std::cerr << "Failed to initialize FreeType library" << std::endl;
        return false;
    }

    // Load font face
    error = FT_New_Face((FT_Library)ftLibrary_, font_path.c_str(), 0, (FT_Face*)&ftFace_);
    if (error) {
        std::cerr << "Failed to load font: " << font_path << std::endl;
        return false;
    }

    // Set pixel size
    error = FT_Set_Pixel_Sizes((FT_Face)ftFace_, 0, font_px_height);
    if (error) {
        std::cerr << "Failed to set font size" << std::endl;
        return false;
    }

    // Get font metrics
    FT_Face face = (FT_Face)ftFace_;
    ascender_ = static_cast<float>(face->size->metrics.ascender >> 6);
    descender_ = static_cast<float>(face->size->metrics.descender >> 6);
    lineHeight_ = static_cast<float>(face->size->metrics.height >> 6);

    std::cout << "✓ Font loaded successfully" << std::endl;
    return true;
}

bool TextRenderer::generateMSDFAtlas(const std::string& font_path, int font_px_height, int atlas_size) {
    std::cout << "Generating MSDF atlas..." << std::endl;

    // Initialize MSDF generator
    MSDFGenerator msdfGen;
    if (!msdfGen.init()) {
        std::cerr << "Failed to initialize MSDF generator" << std::endl;
        return false;
    }

    // Initialize atlas packer
    AtlasPacker packer(atlas_size, atlas_size);

    // Generate MSDF for each ASCII character (32-126)
    std::vector<std::vector<uint8_t>> glyphData;
    glyphData.reserve(95);

    for (int i = 0; i < 95; ++i) {
        uint32_t character = static_cast<uint32_t>(i + 32);
        
        // Load glyph
        FT_Error error = FT_Load_Char((FT_Face)ftFace_, character, FT_LOAD_NO_BITMAP);
        if (error) {
            std::cerr << "Failed to load character: " << static_cast<char>(character) << std::endl;
            continue;
        }

        FT_Face face = (FT_Face)ftFace_;
        FT_GlyphSlot slot = face->glyph;

        // Get glyph metrics
        glyphs_[i].advance = static_cast<float>(slot->advance.x >> 6);
        glyphs_[i].bearing.x = static_cast<float>(slot->bitmap_left);
        glyphs_[i].bearing.y = static_cast<float>(slot->bitmap_top);

        // Generate MSDF
        int width = 64; // Fixed size for now
        int height = 64;
        std::vector<uint8_t> data;

        MSDFGenerator::Params params;
        params.width = width;
        params.height = height;
        params.pxRange = pxRange_;

        if (msdfGen.generateMSDF(&slot->outline, params, data)) {
            glyphs_[i].size.x = static_cast<float>(width);
            glyphs_[i].size.y = static_cast<float>(height);
            glyphData.push_back(std::move(data));
            
            // Add to atlas packer
            packer.addRect(width, height, i);
        } else {
            // Use empty glyph
            glyphs_[i].size = glm::vec2(0.0f);
            glyphData.push_back(std::vector<uint8_t>());
        }
    }

    // Pack atlas
    if (!packer.pack()) {
        std::cerr << "Failed to pack atlas" << std::endl;
        return false;
    }

    std::cout << "✓ Atlas packed with " << packer.getEfficiency() * 100.0f << "% efficiency" << std::endl;

    // Create atlas texture
    std::vector<uint8_t> atlasData(atlas_size * atlas_size * 3, 0);

    for (int i = 0; i < 95; ++i) {
        if (glyphData[i].empty()) continue;

        AtlasPacker::Rect rect = packer.getRect(i);
        if (!rect.isValid()) continue;

        // Copy glyph data to atlas
        for (int y = 0; y < rect.height; ++y) {
            for (int x = 0; x < rect.width; ++x) {
                int srcIndex = (y * rect.width + x) * 3;
                int dstIndex = ((rect.y + y) * atlas_size + (rect.x + x)) * 3;
                
                if (srcIndex < glyphData[i].size() && dstIndex < atlasData.size()) {
                    atlasData[dstIndex + 0] = glyphData[i][srcIndex + 0]; // R
                    atlasData[dstIndex + 1] = glyphData[i][srcIndex + 1]; // G
                    atlasData[dstIndex + 2] = glyphData[i][srcIndex + 2]; // B
                }
            }
        }

        // Update UV coordinates
        glyphs_[i].uv.x = static_cast<float>(rect.x) / static_cast<float>(atlas_size);
        glyphs_[i].uv.y = static_cast<float>(rect.y) / static_cast<float>(atlas_size);
        glyphs_[i].uv.z = static_cast<float>(rect.x + rect.width) / static_cast<float>(atlas_size);
        glyphs_[i].uv.w = static_cast<float>(rect.y + rect.height) / static_cast<float>(atlas_size);
    }

    // Upload atlas to GPU
    glGenTextures(1, &atlasTexture_);
    glBindTexture(GL_TEXTURE_2D, atlasTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, atlas_size, atlas_size, 0, GL_RGB, GL_UNSIGNED_BYTE, atlasData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "✓ MSDF atlas generated and uploaded to GPU" << std::endl;
    return true;
}

bool TextRenderer::createShaders() {
    // Load vertex shader
    std::string vertexSource = R"(
#version 330 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

out vec2 vUV;
out vec4 vColor;

uniform mat4 uProjection;
uniform float uScale;

void main() {
    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
    vUV = aUV;
    vColor = aColor;
}
)";

    // Load fragment shader
    std::string fragmentSource = R"(
#version 330 core
in vec2 vUV;
in vec4 vColor;

out vec4 FragColor;

uniform sampler2D uAtlas;
uniform float uPxRange;
uniform float uSmoothing;
uniform float uThickness;
uniform float uScale;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msdf = texture(uAtlas, vUV).rgb;
    float sd = median(msdf.r, msdf.g, msdf.b);
    float screenPxDistance = sd * uPxRange;
    
    float smoothing = uSmoothing * (1.0 / uScale);
    smoothing = clamp(smoothing, 0.5, 2.0);
    
    float alpha = smoothstep(uThickness - smoothing, uThickness + smoothing, screenPxDistance);
    FragColor = vec4(vColor.rgb, vColor.a * alpha);
    
    if (FragColor.a < 0.01) {
        discard;
    }
}
)";

    // Compile shaders
    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

    if (!vertexShader || !fragmentShader) {
        return false;
    }

    // Link program
    shaderProgram_ = linkProgram(vertexShader, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (!shaderProgram_) {
        return false;
    }

    // Get uniform locations
    projectionLoc_ = glGetUniformLocation(shaderProgram_, "uProjection");
    atlasLoc_ = glGetUniformLocation(shaderProgram_, "uAtlas");
    colorLoc_ = glGetUniformLocation(shaderProgram_, "uColor");
    pxRangeLoc_ = glGetUniformLocation(shaderProgram_, "uPxRange");
    smoothingLoc_ = glGetUniformLocation(shaderProgram_, "uSmoothing");
    scaleLoc_ = glGetUniformLocation(shaderProgram_, "uScale");

    std::cout << "✓ Text shaders compiled successfully" << std::endl;
    return true;
}

bool TextRenderer::createBuffers() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

    setupVertexAttributes();

    glBindVertexArray(0);
    return true;
}

void TextRenderer::setupVertexAttributes() {
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)offsetof(TextVertex, position));

    // UV
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)offsetof(TextVertex, uv));

    // Color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)offsetof(TextVertex, color));
}

void TextRenderer::setSDFParams(float pxRange, float smoothing) {
    pxRange_ = pxRange;
    smoothing_ = smoothing;
}

void TextRenderer::drawText(float x, float y, const std::string& text, const glm::vec4& color, float scale) {
    if (!initialized_ || text.empty()) {
        return;
    }

    beginBatch();

    float currentX = x;
    float currentY = y;

    for (char c : text) {
        if (c == '\n') {
            currentY += lineHeight_ * scale;
            currentX = x;
            continue;
        }

        uint32_t character = static_cast<uint32_t>(c);
        if (character < 32 || character > 126) {
            continue; // Skip non-printable characters
        }

        int glyphIndex = character - 32;
        const Glyph& glyph = glyphs_[glyphIndex];

        if (glyph.size.x > 0 && glyph.size.y > 0) {
            addGlyphQuad(currentX, currentY, glyph, color, scale);
        }

        currentX += glyph.advance * scale;
    }

    endBatch();
}

void TextRenderer::measureText(const std::string& text, float& outWidth, float& outHeight, float scale) {
    outWidth = 0.0f;
    outHeight = lineHeight_ * scale;

    float currentWidth = 0.0f;
    float maxWidth = 0.0f;

    for (char c : text) {
        if (c == '\n') {
            maxWidth = std::max(maxWidth, currentWidth);
            currentWidth = 0.0f;
            continue;
        }

        uint32_t character = static_cast<uint32_t>(c);
        if (character >= 32 && character <= 126) {
            int glyphIndex = character - 32;
            currentWidth += glyphs_[glyphIndex].advance * scale;
        }
    }

    outWidth = std::max(maxWidth, currentWidth);
}

void TextRenderer::cleanup() {
    if (atlasTexture_) {
        glDeleteTextures(1, &atlasTexture_);
        atlasTexture_ = 0;
    }

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

    if (shaderProgram_) {
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
    }

    if (ftFace_) {
        FT_Done_Face((FT_Face)ftFace_);
        ftFace_ = nullptr;
    }

    if (ftLibrary_) {
        FT_Done_FreeType((FT_Library)ftLibrary_);
        ftLibrary_ = nullptr;
    }

    initialized_ = false;
}

void TextRenderer::beginBatch() {
    batching_ = true;
    vertices_.clear();
    indices_.clear();

    glUseProgram(shaderProgram_);
    glUniformMatrix4fv(projectionLoc_, 1, GL_FALSE, &projectionMatrix_[0][0]);
    glUniform1i(atlasLoc_, 0);
    glUniform1f(pxRangeLoc_, pxRange_);
    glUniform1f(smoothingLoc_, smoothing_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasTexture_);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void TextRenderer::endBatch() {
    flush();
    batching_ = false;
}

void TextRenderer::flush() {
    if (vertices_.empty()) {
        return;
    }

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(TextVertex), vertices_.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(uint32_t), indices_.data(), GL_DYNAMIC_DRAW);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void TextRenderer::addGlyphQuad(float x, float y, const Glyph& glyph, const glm::vec4& color, float scale) {
    float scaledWidth = glyph.size.x * scale;
    float scaledHeight = glyph.size.y * scale;

    float xpos = x + glyph.bearing.x * scale;
    float ypos = y + (ascender_ - glyph.bearing.y) * scale;

    // Add vertices
    uint32_t base = static_cast<uint32_t>(vertices_.size());

    vertices_.push_back({{xpos, ypos}, {glyph.uv.x, glyph.uv.y}, color});
    vertices_.push_back({{xpos + scaledWidth, ypos}, {glyph.uv.z, glyph.uv.y}, color});
    vertices_.push_back({{xpos + scaledWidth, ypos + scaledHeight}, {glyph.uv.z, glyph.uv.w}, color});
    vertices_.push_back({{xpos, ypos + scaledHeight}, {glyph.uv.x, glyph.uv.w}, color});

    // Add indices
    indices_.push_back(base + 0);
    indices_.push_back(base + 1);
    indices_.push_back(base + 2);
    indices_.push_back(base + 0);
    indices_.push_back(base + 2);
    indices_.push_back(base + 3);
}

GLuint TextRenderer::compileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint TextRenderer::linkProgram(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader linking failed: " << infoLog << std::endl;
        glDeleteProgram(program);
        return 0;
    }

    return program;
}