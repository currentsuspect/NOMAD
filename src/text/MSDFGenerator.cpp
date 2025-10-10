#include "MSDFGenerator.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <dlfcn.h>
#endif

// Forward declarations for external msdfgen functions (if available)
extern "C" {
    typedef int (*msdfgen_generateMSDF_t)(unsigned char*, int, int, const void*, double, double, double, int);
    typedef int (*msdfgen_init_t)();
}

MSDFGenerator::MSDFGenerator() 
    : initialized_(false)
    , hasExternalMSDFGen_(false)
    , msdfgenLibrary_(nullptr)
{
}

MSDFGenerator::~MSDFGenerator() {
    cleanup();
}

bool MSDFGenerator::init() {
    if (initialized_) {
        return true;
    }

    // Try to load external msdfgen library first
    if (initExternalMSDFGen()) {
        hasExternalMSDFGen_ = true;
        std::cout << "✓ Using external msdfgen library" << std::endl;
    } else {
        // Fall back to integrated implementation
        if (initIntegratedMSDF()) {
            hasExternalMSDFGen_ = false;
            std::cout << "✓ Using integrated MSDF implementation" << std::endl;
        } else {
            std::cerr << "✗ Failed to initialize MSDF generator" << std::endl;
            return false;
        }
    }

    initialized_ = true;
    return true;
}

bool MSDFGenerator::initExternalMSDFGen() {
#ifdef _WIN32
    // Try to load msdfgen.dll
    msdfgenLibrary_ = LoadLibraryA("msdfgen.dll");
    if (!msdfgenLibrary_) {
        return false;
    }
    
    // Get function pointers
    auto initFunc = (msdfgen_init_t)GetProcAddress((HMODULE)msdfgenLibrary_, "msdfgen_init");
    auto generateFunc = (msdfgen_generateMSDF_t)GetProcAddress((HMODULE)msdfgenLibrary_, "msdfgen_generateMSDF");
    
    if (!initFunc || !generateFunc) {
        FreeLibrary((HMODULE)msdfgenLibrary_);
        msdfgenLibrary_ = nullptr;
        return false;
    }
    
    // Initialize the library
    if (initFunc() != 0) {
        FreeLibrary((HMODULE)msdfgenLibrary_);
        msdfgenLibrary_ = nullptr;
        return false;
    }
#else
    // Try to load libmsdfgen.so
    msdfgenLibrary_ = dlopen("libmsdfgen.so", RTLD_LAZY);
    if (!msdfgenLibrary_) {
        return false;
    }
    
    // Get function pointers
    auto initFunc = (msdfgen_init_t)dlsym(msdfgenLibrary_, "msdfgen_init");
    auto generateFunc = (msdfgen_generateMSDF_t)dlsym(msdfgenLibrary_, "msdfgen_generateMSDF");
    
    if (!initFunc || !generateFunc) {
        dlclose(msdfgenLibrary_);
        msdfgenLibrary_ = nullptr;
        return false;
    }
    
    // Initialize the library
    if (initFunc() != 0) {
        dlclose(msdfgenLibrary_);
        msdfgenLibrary_ = nullptr;
        return false;
    }
#endif

    return true;
}

bool MSDFGenerator::initIntegratedMSDF() {
    // Integrated MSDF implementation is always available
    return true;
}

bool MSDFGenerator::generateMSDF(const void* outlineData, const Params& params, std::vector<uint8_t>& outData) {
    if (!initialized_) {
        return false;
    }

    if (hasExternalMSDFGen_) {
        return generateWithExternalMSDFGen(outlineData, params, outData);
    } else {
        return generateWithIntegratedMSDF(outlineData, params, outData);
    }
}

bool MSDFGenerator::generateWithExternalMSDFGen(const void* outlineData, const Params& params, std::vector<uint8_t>& outData) {
    // This would use the external msdfgen library
    // For now, fall back to integrated implementation
    return generateWithIntegratedMSDF(outlineData, params, outData);
}

bool MSDFGenerator::generateWithIntegratedMSDF(const void* outlineData, const Params& params, std::vector<uint8_t>& outData) {
    // For now, generate a simple test pattern
    // In a real implementation, this would process the FreeType outline
    outData.resize(params.width * params.height * 3);
    
    // Generate a simple circle MSDF for testing
    double centerX = params.width * 0.5;
    double centerY = params.height * 0.5;
    double radius = std::min(params.width, params.height) * 0.4;
    
    for (int y = 0; y < params.height; ++y) {
        for (int x = 0; x < params.width; ++x) {
            double dx = x - centerX;
            double dy = y - centerY;
            double dist = std::sqrt(dx * dx + dy * dy) - radius;
            
            // Convert distance to MSDF value (-1 to 1 range)
            double msdfValue = std::max(-1.0, std::min(1.0, dist / params.pxRange));
            
            // Convert to 0-255 range
            uint8_t value = static_cast<uint8_t>((msdfValue + 1.0) * 127.5);
            
            int index = (y * params.width + x) * 3;
            outData[index + 0] = value;  // R channel
            outData[index + 1] = value;  // G channel  
            outData[index + 2] = value;  // B channel
        }
    }
    
    return true;
}

bool MSDFGenerator::generateSimpleMSDF(int shapeType, const Params& params, std::vector<uint8_t>& outData) {
    outData.resize(params.width * params.height * 3);
    
    double centerX = params.width * 0.5;
    double centerY = params.height * 0.5;
    double size = std::min(params.width, params.height) * 0.4;
    
    for (int y = 0; y < params.height; ++y) {
        for (int x = 0; x < params.width; ++x) {
            double dx = (x - centerX) / size;
            double dy = (y - centerY) / size;
            double dist = 0.0;
            
            switch (shapeType) {
                case 0: // Circle
                    dist = std::sqrt(dx * dx + dy * dy) - 1.0;
                    break;
                case 1: // Square
                    dist = std::max(std::abs(dx), std::abs(dy)) - 1.0;
                    break;
                case 2: // Triangle
                    {
                        double ax = dx + 0.5;
                        double ay = dy + 0.866;
                        double bx = dx - 0.5;
                        double by = dy + 0.866;
                        double cx = dx;
                        double cy = dy - 0.866;
                        
                        double da = std::sqrt(ax * ax + ay * ay) - 1.0;
                        double db = std::sqrt(bx * bx + by * by) - 1.0;
                        double dc = std::sqrt(cx * cx + cy * cy) - 1.0;
                        
                        dist = std::min({da, db, dc});
                    }
                    break;
            }
            
            // Convert distance to MSDF value
            double msdfValue = std::max(-1.0, std::min(1.0, dist * params.pxRange));
            uint8_t value = static_cast<uint8_t>((msdfValue + 1.0) * 127.5);
            
            int index = (y * params.width + x) * 3;
            outData[index + 0] = value;
            outData[index + 1] = value;
            outData[index + 2] = value;
        }
    }
    
    return true;
}

void MSDFGenerator::cleanup() {
    if (msdfgenLibrary_) {
#ifdef _WIN32
        FreeLibrary((HMODULE)msdfgenLibrary_);
#else
        dlclose(msdfgenLibrary_);
#endif
        msdfgenLibrary_ = nullptr;
    }
    
    initialized_ = false;
    hasExternalMSDFGen_ = false;
}

// Integrated MSDF algorithm implementation
double MSDFGenerator::signedDistance(const Point2D& point, const std::vector<Edge>& edges) {
    double minDist = std::numeric_limits<double>::infinity();
    
    for (const auto& edge : edges) {
        double dist = 0.0;
        
        if (edge.isLinear) {
            // Linear edge distance calculation
            double dx = edge.p1.x - edge.p0.x;
            double dy = edge.p1.y - edge.p0.y;
            double length = std::sqrt(dx * dx + dy * dy);
            
            if (length > 0) {
                double t = std::max(0.0, std::min(1.0, 
                    ((point.x - edge.p0.x) * dx + (point.y - edge.p0.y) * dy) / (length * length)));
                
                double projX = edge.p0.x + t * dx;
                double projY = edge.p0.y + t * dy;
                
                dist = std::sqrt((point.x - projX) * (point.x - projX) + (point.y - projY) * (point.y - projY));
            } else {
                dist = std::sqrt((point.x - edge.p0.x) * (point.x - edge.p0.x) + (point.y - edge.p0.y) * (point.y - edge.p0.y));
            }
        }
        // TODO: Add quadratic and cubic edge support
        
        minDist = std::min(minDist, dist);
    }
    
    return minDist;
}

double MSDFGenerator::median(double a, double b, double c) {
    return std::max(std::min(a, b), std::min(std::max(a, b), c));
}

void MSDFGenerator::generateDistanceField(const std::vector<Edge>& edges, const Params& params, std::vector<uint8_t>& outData) {
    outData.resize(params.width * params.height * 3);
    
    for (int y = 0; y < params.height; ++y) {
        for (int x = 0; x < params.width; ++x) {
            Point2D point(x + 0.5, y + 0.5);
            double dist = signedDistance(point, edges);
            
            // Convert to MSDF value
            double msdfValue = std::max(-1.0, std::min(1.0, dist / params.pxRange));
            uint8_t value = static_cast<uint8_t>((msdfValue + 1.0) * 127.5);
            
            int index = (y * params.width + x) * 3;
            outData[index + 0] = value;
            outData[index + 1] = value;
            outData[index + 2] = value;
        }
    }
}