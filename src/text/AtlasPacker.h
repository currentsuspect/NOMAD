#pragma once

#include <vector>
#include <glm/glm.hpp>

/**
 * Simple skyline-based texture atlas packer
 * 
 * Packs rectangular textures into a larger atlas using a skyline algorithm.
 * This is efficient for text glyphs which are typically small and varied in size.
 */
class AtlasPacker {
public:
    /**
     * Rectangle structure for packing
     */
    struct Rect {
        int x, y;           // Position in atlas
        int width, height;  // Dimensions
        int id;             // Unique identifier
        
        Rect(int x = 0, int y = 0, int w = 0, int h = 0, int id = -1)
            : x(x), y(y), width(w), height(h), id(id) {}
        
        bool isValid() const { return width > 0 && height > 0; }
        int right() const { return x + width; }
        int bottom() const { return y + height; }
    };

    /**
     * Initialize the atlas packer
     * @param atlasWidth Width of the atlas texture
     * @param atlasHeight Height of the atlas texture
     */
    AtlasPacker(int atlasWidth, int atlasHeight);

    /**
     * Add a rectangle to be packed
     * @param width Rectangle width
     * @param height Rectangle height
     * @param id Unique identifier
     * @return true if rectangle was added successfully
     */
    bool addRect(int width, int height, int id);

    /**
     * Pack all rectangles into the atlas
     * @return true if all rectangles fit successfully
     */
    bool pack();

    /**
     * Get the packed rectangle for a given ID
     * @param id Rectangle identifier
     * @return Packed rectangle, or invalid rect if not found
     */
    Rect getRect(int id) const;

    /**
     * Get all packed rectangles
     */
    const std::vector<Rect>& getRects() const { return rects_; }

    /**
     * Get atlas dimensions
     */
    int getAtlasWidth() const { return atlasWidth_; }
    int getAtlasHeight() const { return atlasHeight_; }

    /**
     * Get packing efficiency (used area / total area)
     */
    float getEfficiency() const;

    /**
     * Clear all rectangles
     */
    void clear();

private:
    /**
     * Skyline node for the packing algorithm
     */
    struct SkylineNode {
        int x, y, width;
        
        SkylineNode(int x = 0, int y = 0, int w = 0) : x(x), y(y), width(w) {}
    };

    /**
     * Find the best position for a rectangle using skyline algorithm
     * @param width Rectangle width
     * @param height Rectangle height
     * @param outX Output X position
     * @param outY Output Y position
     * @return true if position found
     */
    bool findBestPosition(int width, int height, int& outX, int& outY);

    /**
     * Add a skyline node
     * @param x X position
     * @param y Y position
     * @param width Width
     */
    void addSkylineNode(int x, int y, int width);

    /**
     * Merge skyline nodes to remove redundancy
     */
    void mergeSkylineNodes();

    int atlasWidth_;
    int atlasHeight_;
    std::vector<Rect> rects_;
    std::vector<SkylineNode> skyline_;
    int usedArea_;
};