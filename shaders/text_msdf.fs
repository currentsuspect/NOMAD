#version 330 core

// Input from vertex shader
in vec2 vUV;
in vec4 vColor;

// Output color
out vec4 FragColor;

// Uniforms
uniform sampler2D uAtlas;      // MSDF atlas texture
uniform float uPxRange;        // Distance field range in pixels
uniform float uSmoothing;      // Additional smoothing factor
uniform float uThickness;      // Outline thickness (0.5 = normal, <0.5 = outline, >0.5 = glow)
uniform float uScale;          // Scale factor for text

// Median function for MSDF reconstruction
float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    // Sample the MSDF texture (3-channel RGB)
    vec3 msdf = texture(uAtlas, vUV).rgb;
    
    // Reconstruct signed distance using median trick
    float sd = median(msdf.r, msdf.g, msdf.b);
    
    // Convert to screen pixel distance
    float screenPxDistance = sd * uPxRange;
    
    // Calculate smoothing based on scale
    float smoothing = uSmoothing * (1.0 / uScale);
    smoothing = clamp(smoothing, 0.5, 2.0);
    
    // Compute alpha using smoothstep for anti-aliasing
    float alpha = smoothstep(uThickness - smoothing, uThickness + smoothing, screenPxDistance);
    
    // Apply color
    FragColor = vec4(vColor.rgb, vColor.a * alpha);
    
    // Discard fragments with very low alpha to avoid overdraw
    if (FragColor.a < 0.01) {
        discard;
    }
}