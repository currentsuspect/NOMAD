#pragma once

#include <vector>
#include <cstdint>

/**
 * MSDF (Multi-channel Signed Distance Field) generator wrapper
 * 
 * This class provides a unified interface for MSDF generation, supporting both
 * the external msdfgen library and an integrated implementation.
 */
class MSDFGenerator {
public:
    /**
     * MSDF generation parameters
     */
    struct Params {
        int width = 64;         // Output bitmap width
        int height = 64;        // Output bitmap height
        double pxRange = 4.0;   // Distance field range in pixels
        double scale = 1.0;     // Scale factor for the shape
        bool overlapSupport = true;  // Enable overlap support for complex shapes
    };

    /**
     * Initialize the MSDF generator
     * @return true if initialization successful
     */
    bool init();

    /**
     * Generate MSDF from font outline
     * @param outlineData Font outline data (FreeType outline)
     * @param params Generation parameters
     * @param outData Output RGB data (3 bytes per pixel)
     * @return true if generation successful
     */
    bool generateMSDF(const void* outlineData, const Params& params, std::vector<uint8_t>& outData);

    /**
     * Generate MSDF from simple shape (for testing)
     * @param shapeType Simple shape type (0=circle, 1=square, 2=triangle)
     * @param params Generation parameters
     * @param outData Output RGB data (3 bytes per pixel)
     * @return true if generation successful
     */
    bool generateSimpleMSDF(int shapeType, const Params& params, std::vector<uint8_t>& outData);

    /**
     * Check if external msdfgen library is available
     */
    bool hasExternalLibrary() const { return hasExternalMSDFGen_; }

    /**
     * Cleanup resources
     */
    void cleanup();

private:
    // External msdfgen integration
    bool initExternalMSDFGen();
    bool generateWithExternalMSDFGen(const void* outlineData, const Params& params, std::vector<uint8_t>& outData);
    
    // Integrated MSDF implementation
    bool initIntegratedMSDF();
    bool generateWithIntegratedMSDF(const void* outlineData, const Params& params, std::vector<uint8_t>& outData);
    
    // Integrated MSDF algorithm implementation
    struct Point2D {
        double x, y;
        Point2D(double x = 0, double y = 0) : x(x), y(y) {}
    };
    
    struct Edge {
        Point2D p0, p1;
        bool isLinear;
        Edge(const Point2D& p0, const Point2D& p1, bool linear = true) 
            : p0(p0), p1(p1), isLinear(linear) {}
    };
    
    double signedDistance(const Point2D& point, const std::vector<Edge>& edges);
    double median(double a, double b, double c);
    void generateDistanceField(const std::vector<Edge>& edges, const Params& params, std::vector<uint8_t>& outData);
    
    // State
    bool initialized_;
    bool hasExternalMSDFGen_;
    void* msdfgenLibrary_;  // Handle to external library if available
};