#version 330 core

// Input vertex attributes
layout(location = 0) in vec2 aPosition;    // Screen position
layout(location = 1) in vec2 aUV;          // Texture coordinates
layout(location = 2) in vec4 aColor;       // RGBA color

// Output to fragment shader
out vec2 vUV;
out vec4 vColor;

// Uniforms
uniform mat4 uProjection;  // Orthographic projection matrix
uniform float uScale;      // Scale factor for text

void main() {
    // Transform position by projection matrix
    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
    
    // Pass through texture coordinates and color
    vUV = aUV;
    vColor = aColor;
}