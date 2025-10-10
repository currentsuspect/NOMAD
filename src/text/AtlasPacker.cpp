#include "AtlasPacker.h"
#include <algorithm>
#include <iostream>

AtlasPacker::AtlasPacker(int atlasWidth, int atlasHeight)
    : atlasWidth_(atlasWidth)
    , atlasHeight_(atlasHeight)
    , usedArea_(0)
{
    // Initialize with a single skyline node covering the entire width
    skyline_.clear();
    skyline_.emplace_back(0, 0, atlasWidth_);
}

bool AtlasPacker::addRect(int width, int height, int id) {
    if (width <= 0 || height <= 0 || width > atlasWidth_ || height > atlasHeight_) {
        return false;
    }
    
    rects_.emplace_back(0, 0, width, height, id);
    return true;
}

bool AtlasPacker::pack() {
    // Sort rectangles by height (tallest first) for better packing
    std::sort(rects_.begin(), rects_.end(), [](const Rect& a, const Rect& b) {
        return a.height > b.height;
    });
    
    usedArea_ = 0;
    
    for (auto& rect : rects_) {
        int x, y;
        if (!findBestPosition(rect.width, rect.height, x, y)) {
            std::cerr << "Failed to pack rectangle " << rect.id << " (" 
                      << rect.width << "x" << rect.height << ")" << std::endl;
            return false;
        }
        
        rect.x = x;
        rect.y = y;
        usedArea_ += rect.width * rect.height;
        
        // Update skyline
        addSkylineNode(x, y + rect.height, rect.width);
        mergeSkylineNodes();
    }
    
    return true;
}

AtlasPacker::Rect AtlasPacker::getRect(int id) const {
    for (const auto& rect : rects_) {
        if (rect.id == id) {
            return rect;
        }
    }
    return Rect(); // Invalid rect
}

float AtlasPacker::getEfficiency() const {
    if (atlasWidth_ <= 0 || atlasHeight_ <= 0) {
        return 0.0f;
    }
    return static_cast<float>(usedArea_) / static_cast<float>(atlasWidth_ * atlasHeight_);
}

void AtlasPacker::clear() {
    rects_.clear();
    skyline_.clear();
    skyline_.emplace_back(0, 0, atlasWidth_);
    usedArea_ = 0;
}

bool AtlasPacker::findBestPosition(int width, int height, int& outX, int& outY) {
    int bestX = -1;
    int bestY = -1;
    int bestWaste = atlasWidth_ * atlasHeight_; // Start with worst case
    
    for (size_t i = 0; i < skyline_.size(); ++i) {
        const auto& node = skyline_[i];
        
        // Check if rectangle fits at this position
        if (node.x + width <= atlasWidth_ && node.y + height <= atlasHeight_) {
            // Calculate waste (unused area)
            int waste = 0;
            int usedWidth = 0;
            
            // Check all skyline nodes that would be affected
            for (size_t j = i; j < skyline_.size() && skyline_[j].y <= node.y; ++j) {
                if (skyline_[j].x < node.x + width) {
                    usedWidth = std::max(usedWidth, skyline_[j].x + skyline_[j].width);
                }
            }
            
            if (usedWidth < node.x + width) {
                waste = (node.x + width - usedWidth) * height;
            }
            
            // Choose position with least waste
            if (waste < bestWaste) {
                bestX = node.x;
                bestY = node.y;
                bestWaste = waste;
            }
        }
    }
    
    if (bestX >= 0) {
        outX = bestX;
        outY = bestY;
        return true;
    }
    
    return false;
}

void AtlasPacker::addSkylineNode(int x, int y, int width) {
    // Find insertion point
    size_t insertPos = 0;
    while (insertPos < skyline_.size() && skyline_[insertPos].x < x) {
        ++insertPos;
    }
    
    // Insert new node
    skyline_.insert(skyline_.begin() + insertPos, SkylineNode(x, y, width));
}

void AtlasPacker::mergeSkylineNodes() {
    if (skyline_.size() <= 1) {
        return;
    }
    
    std::vector<SkylineNode> merged;
    merged.reserve(skyline_.size());
    
    for (const auto& node : skyline_) {
        if (merged.empty()) {
            merged.push_back(node);
        } else {
            auto& last = merged.back();
            
            // If this node is at the same Y level and adjacent to the last node, merge them
            if (last.y == node.y && last.x + last.width == node.x) {
                last.width += node.width;
            } else {
                merged.push_back(node);
            }
        }
    }
    
    skyline_ = std::move(merged);
}