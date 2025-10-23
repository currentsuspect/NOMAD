#include "FileBrowser.h"
#include "../NomadUI/Core/NUIThemeSystem.h"
#include "../NomadUI/Graphics/NUIRenderer.h"
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace NomadUI {

FileBrowser::FileBrowser()
    : NUIComponent()
    , selectedFile_(nullptr)
    , selectedIndex_(-1)
    , scrollOffset_(0.0f)
    , itemHeight_(32.0f)
    , visibleItems_(0)
    , showHiddenFiles_(false)
    , scrollbarVisible_(false)
    , scrollbarOpacity_(0.0f)
    , scrollbarWidth_(8.0f)
    , scrollbarTrackHeight_(0.0f)
    , scrollbarThumbHeight_(0.0f)
    , scrollbarThumbY_(0.0f)
    , isDraggingScrollbar_(false)
    , dragStartY_(0.0f)
    , dragStartScrollOffset_(0.0f)
    , scrollbarFadeTimer_(0.0f)
    , hoveredIndex_(-1)
    , sortMode_(SortMode::Name)
    , sortAscending_(true)
{
    setSize(400, 300); // Default size
    
    // Initialize icons with improved visibility for Liminal Dark v2.0
    // Use inline SVG content for reliable icon loading
    folderIcon_ = std::make_shared<NUIIcon>();
    const char* folderSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <path d="M10 4H4c-1.11 0-2 .89-2 2v12c0 1.11.89 2 2 2h16c1.11 0 2-.89 2-2V8c0-1.11-.89-2-2-2h-8l-2-2z"/>
        </svg>
    )";
    folderIcon_->loadSVG(folderSvg);
    folderIcon_->setIconSize(24, 24);
    folderIcon_->setColor(NUIColor(0.0f, 0.737f, 0.831f, 1.0f));  // #00bcd4 - Brighter cyan for folders
    
    folderOpenIcon_ = std::make_shared<NUIIcon>();
    const char* folderOpenSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <path d="M20 6h-8l-2-2H4c-1.11 0-1.99.89-1.99 2L2 18c0 1.11.89 2 2 2h16c1.11 0 2-.89 2-2V8c0-1.11-.89-2-2-2zm0 12H4V8h16v10z"/>
        </svg>
    )";
    folderOpenIcon_->loadSVG(folderOpenSvg);
    folderOpenIcon_->setIconSize(24, 24);
    folderOpenIcon_->setColor(NUIColor(0.0f, 0.737f, 0.831f, 1.0f));  // #00bcd4 - Brighter cyan for folders
    
    audioFileIcon_ = std::make_shared<NUIIcon>();
    const char* audioFileSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <path d="M14,2H6A2,2 0 0,0 4,4V20A2,2 0 0,0 6,22H18A2,2 0 0,0 20,20V8L14,2M18,20H6V4H13V9H18V20Z"/>
        </svg>
    )";
    audioFileIcon_->loadSVG(audioFileSvg);
    audioFileIcon_->setIconSize(24, 24);
    audioFileIcon_->setColor(NUIColor(0.902f, 0.902f, 0.922f, 1.0f));  // #e6e6eb - Bright white for file icons
    
    musicFileIcon_ = std::make_shared<NUIIcon>();
    const char* musicFileSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <path d="M14,2H6A2,2 0 0,0 4,4V20A2,2 0 0,0 6,22H18A2,2 0 0,0 20,20V8L14,2M18,20H6V4H13V9H18V20Z"/>
        </svg>
    )";
    musicFileIcon_->loadSVG(musicFileSvg);
    musicFileIcon_->setIconSize(24, 24);
    musicFileIcon_->setColor(NUIColor(0.902f, 0.902f, 0.922f, 1.0f));  // #e6e6eb - Bright white for file icons
    
    projectFileIcon_ = std::make_shared<NUIIcon>();
    const char* projectFileSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <path d="M14,2H6A2,2 0 0,0 4,4V20A2,2 0 0,0 6,22H18A2,2 0 0,0 20,20V8L14,2M18,20H6V4H13V9H18V20Z"/>
        </svg>
    )";
    projectFileIcon_->loadSVG(projectFileSvg);
    projectFileIcon_->setIconSize(24, 24);
    projectFileIcon_->setColor(NUIColor(0.902f, 0.902f, 0.922f, 1.0f));  // #e6e6eb - Bright white for file icons
    
    wavFileIcon_ = std::make_shared<NUIIcon>();
    const char* wavFileSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <path d="M14,2H6A2,2 0 0,0 4,4V20A2,2 0 0,0 6,22H18A2,2 0 0,0 20,20V8L14,2M18,20H6V4H13V9H18V20Z"/>
        </svg>
    )";
    wavFileIcon_->loadSVG(wavFileSvg);
    wavFileIcon_->setIconSize(24, 24);
    wavFileIcon_->setColor(NUIColor(0.902f, 0.902f, 0.922f, 1.0f));  // #e6e6eb - Bright white for file icons
    
    mp3FileIcon_ = std::make_shared<NUIIcon>();
    const char* mp3FileSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <path d="M14,2H6A2,2 0 0,0 4,4V20A2,2 0 0,0 6,22H18A2,2 0 0,0 20,20V8L14,2M18,20H6V4H13V9H18V20Z"/>
        </svg>
    )";
    mp3FileIcon_->loadSVG(mp3FileSvg);
    mp3FileIcon_->setIconSize(24, 24);
    mp3FileIcon_->setColor(NUIColor(0.902f, 0.902f, 0.922f, 1.0f));  // #e6e6eb - Bright white for file icons
    
    flacFileIcon_ = std::make_shared<NUIIcon>();
    const char* flacFileSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <path d="M14,2H6A2,2 0 0,0 4,4V20A2,2 0 0,0 6,22H18A2,2 0 0,0 20,20V8L14,2M18,20H6V4H13V9H18V20Z"/>
        </svg>
    )";
    flacFileIcon_->loadSVG(flacFileSvg);
    flacFileIcon_->setIconSize(24, 24);
    flacFileIcon_->setColor(NUIColor(0.902f, 0.902f, 0.922f, 1.0f));  // #e6e6eb - Bright white for file icons
    
    // Unknown file icon (use a generic document icon)
    unknownFileIcon_ = std::make_shared<NUIIcon>();
    const char* unknownFileSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <path d="M14,2H6A2,2 0 0,0 4,4V20A2,2 0 0,0 6,22H18A2,2 0 0,0 20,20V8L14,2M18,20H6V4H13V9H18V20Z"/>
        </svg>
    )";
    unknownFileIcon_->loadSVG(unknownFileSvg);
    unknownFileIcon_->setIconSize(24, 24);
    unknownFileIcon_->setColor(NUIColor(0.604f, 0.604f, 0.639f, 1.0f));  // #9a9aa3 - Secondary text for unknown files
    
    // Set initial path to current directory
    currentPath_ = std::filesystem::current_path().string();
    loadDirectoryContents();
    
    // Load Liminal Dark v2.0 theme colors
    auto& themeManager = NUIThemeManager::getInstance();
    backgroundColor_ = themeManager.getColor("backgroundSecondary");  // #1b1b1f - Panels, sidebars, file browser
    textColor_ = themeManager.getColor("textPrimary");                // #e6e6eb - Soft white
    selectedColor_ = themeManager.getColor("accentCyan");             // #00bcd4 - Accent cyan for selection
    hoverColor_ = themeManager.getColor("surfaceRaised");             // #32323a - Hovered buttons, focused areas
    borderColor_ = themeManager.getColor("border");                   // #2e2e35 - Subtle separation lines
}

void FileBrowser::onRender(NUIRenderer& renderer) {
    NUIRect bounds = getBounds();
    if (bounds.isEmpty()) return;
    
    
    // Render background with enhanced borders
    renderer.fillRoundedRect(bounds, 8, backgroundColor_);
    
    // Main border
    renderer.strokeRoundedRect(bounds, 8, 1, borderColor_);
    
    // Inner black border for cleaner look
    NUIRect innerBounds(bounds.x + 1, bounds.y + 1, bounds.width - 2, bounds.height - 2);
    renderer.strokeRoundedRect(innerBounds, 7, 1, NUIColor(0.0f, 0.0f, 0.0f, 0.4f));
    
    // Render toolbar
    renderToolbar(renderer);
    
    // Render path bar
    renderPathBar(renderer);
    
    // Render file list
    renderFileList(renderer);
    
    // Render scrollbar
    renderScrollbar(renderer);
}

void FileBrowser::onResize(int width, int height) {
    NUIComponent::onResize(width, height);
    
    // Update visible items count
    visibleItems_ = static_cast<int>((height - 80) / itemHeight_); // Account for toolbar and path bar
    visibleItems_ = std::max(1, visibleItems_);
    
    // Update scrollbar dimensions
    scrollbarTrackHeight_ = height - 80; // Same as file list area
    updateScrollbarVisibility();
}

bool FileBrowser::onMouseEvent(const NUIMouseEvent& event) {
    NUIRect bounds = getBounds();
    
    // If we're dragging the scrollbar, handle mouse events even outside bounds
    if (isDraggingScrollbar_) {
        // Always check scrollbar events if we're dragging
        if (handleScrollbarMouseEvent(event)) {
            return true;
        }
    }
    
    // Check if mouse is within bounds
    bool mouseInside = bounds.contains(event.position.x, event.position.y);
    
    // Clear hover if mouse leaves the file browser entirely
    if (!mouseInside) {
        if (hoveredIndex_ != -1) {
            hoveredIndex_ = -1;
            setDirty(true); // Trigger redraw when clearing hover
        }
        return false;
    }
    
    
    // Handle scrollbar mouse events first - check if scrollbar should be visible
    float contentHeight = files_.size() * itemHeight_;
    float maxScroll = std::max(0.0f, contentHeight - scrollbarTrackHeight_);
    bool needsScrollbar = maxScroll > 0.0f;
    
    // Handle mouse wheel scrolling first (works anywhere in the file browser)
    if (event.wheelDelta != 0) {
        float listY = bounds.y + 60; // Below toolbar and path bar
        float listHeight = bounds.height - 60;
        
        // Match Windows scroll speed - scroll 1 item per wheel step (no acceleration)
        float scrollSpeed = 1.0f; // Scroll 1 item per wheel step to match Windows
        scrollOffset_ -= event.wheelDelta * scrollSpeed * itemHeight_;
        
        // Clamp scroll offset
        float maxScroll = std::max(0.0f, (files_.size() * itemHeight_) - listHeight);
        scrollOffset_ = std::max(0.0f, std::min(scrollOffset_, maxScroll));
        
        updateScrollbarVisibility();
        return true;
    }
    
    // Check scrollbar events if scrollbar is needed (but not dragging - handled above)
    if (needsScrollbar && files_.size() > 0 && !isDraggingScrollbar_) {
        if (handleScrollbarMouseEvent(event)) {
            return true;
        }
    }
    
    // Check if click is in file list area
    float listY = bounds.y + 60; // Below toolbar and path bar
    float listHeight = bounds.height - 60;
    
    if (event.position.x >= bounds.x && event.position.x <= bounds.x + bounds.width &&
        event.position.y >= listY && event.position.y <= listY + listHeight) {
        
        // Calculate which item is being hovered
        float relativeY = event.position.y - listY;
        int itemIndex = static_cast<int>((relativeY + scrollOffset_) / itemHeight_);
        
        // Update hover state
        int newHoveredIndex = (itemIndex >= 0 && itemIndex < static_cast<int>(files_.size())) ? itemIndex : -1;
        if (newHoveredIndex != hoveredIndex_) {
            hoveredIndex_ = newHoveredIndex;
            setDirty(true); // Trigger redraw when hover state changes
        }
        
        if (event.pressed && event.button == NUIMouseButton::Left) {
            // Calculate which file was clicked
            float relativeY = event.position.y - listY;
            int itemIndex = static_cast<int>((relativeY + scrollOffset_) / itemHeight_);
            
            if (itemIndex >= 0 && itemIndex < static_cast<int>(files_.size())) {
                selectedIndex_ = itemIndex;
                selectedFile_ = &files_[itemIndex];
                
                if (onFileSelected_) {
                    onFileSelected_(*selectedFile_);
                }
                
                // Double click to open
                static auto lastClickTime = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastClickTime);
                
                if (duration.count() < 500) { // 500ms double click threshold
                    if (selectedFile_->isDirectory) {
                        navigateTo(selectedFile_->path);
                    } else {
                        if (onFileOpened_) {
                            onFileOpened_(*selectedFile_);
                        }
                    }
                }
                
                lastClickTime = now;
                setDirty(true);
                return true;
            }
        }
    }
    
    return false;
}

bool FileBrowser::onKeyEvent(const NUIKeyEvent& event) {
    if (!event.pressed) return false;
    
    switch (event.keyCode) {
        case NUIKeyCode::Up:
            if (selectedIndex_ > 0) {
                selectedIndex_--;
                selectedFile_ = &files_[selectedIndex_];
                updateScrollPosition();
                if (onFileSelected_) {
                    onFileSelected_(*selectedFile_);
                }
                setDirty(true);
                return true;
            }
            break;
            
        case NUIKeyCode::Down:
            if (selectedIndex_ < static_cast<int>(files_.size()) - 1) {
                selectedIndex_++;
                selectedFile_ = &files_[selectedIndex_];
                updateScrollPosition();
                if (onFileSelected_) {
                    onFileSelected_(*selectedFile_);
                }
                setDirty(true);
                return true;
            }
            break;
            
        case NUIKeyCode::Enter:
            if (selectedFile_) {
                if (selectedFile_->isDirectory) {
                    navigateTo(selectedFile_->path);
                } else {
                    if (onFileOpened_) {
                        onFileOpened_(*selectedFile_);
                    }
                }
                return true;
            }
            break;
            
        case NUIKeyCode::Backspace:
            navigateUp();
            return true;
    }
    
    return false;
}

void FileBrowser::setCurrentPath(const std::string& path) {
    currentPath_ = path;
    loadDirectoryContents();
    if (onPathChanged_) {
        onPathChanged_(currentPath_);
    }
    setDirty(true);
}

void FileBrowser::refresh() {
    loadDirectoryContents();
    setDirty(true);
}

void FileBrowser::navigateUp() {
    std::filesystem::path parent = std::filesystem::path(currentPath_).parent_path();
    if (parent != currentPath_) {
        setCurrentPath(parent.string());
    }
}

void FileBrowser::navigateTo(const std::string& path) {
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
        setCurrentPath(path);
    }
}

void FileBrowser::selectFile(const std::string& path) {
    for (size_t i = 0; i < files_.size(); ++i) {
        if (files_[i].path == path) {
            selectedIndex_ = static_cast<int>(i);
            selectedFile_ = &files_[i];
            updateScrollPosition();
            if (onFileSelected_) {
                onFileSelected_(*selectedFile_);
            }
            setDirty(true);
            break;
        }
    }
}

void FileBrowser::openFile(const std::string& path) {
    selectFile(path);
    if (selectedFile_ && onFileOpened_) {
        onFileOpened_(*selectedFile_);
    }
}

void FileBrowser::openFolder(const std::string& path) {
    navigateTo(path);
}

void FileBrowser::setSortMode(SortMode mode) {
    sortMode_ = mode;
    sortFiles();
    setDirty(true);
}

void FileBrowser::setSortAscending(bool ascending) {
    sortAscending_ = ascending;
    sortFiles();
    setDirty(true);
}

void FileBrowser::loadDirectoryContents() {
    files_.clear();
    selectedFile_ = nullptr;
    selectedIndex_ = -1;
    
    try {
        std::filesystem::path currentDir(currentPath_);
        
        // Add parent directory entry if not at root
        if (currentDir.has_parent_path() && currentDir.parent_path() != currentDir) {
            files_.emplace_back("..", currentDir.parent_path().string(), FileType::Folder, true);
        }
        
        // Iterate through directory contents
        for (const auto& entry : std::filesystem::directory_iterator(currentDir)) {
            if (!showHiddenFiles_ && entry.path().filename().string()[0] == '.') {
                continue;
            }
            
            std::string name = entry.path().filename().string();
            std::string path = entry.path().string();
            bool isDir = entry.is_directory();
            
            FileType type = FileType::Unknown;
            size_t size = 0;
            std::string lastModified;
            
            if (isDir) {
                type = FileType::Folder;
            } else {
                std::string extension = entry.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                type = getFileTypeFromExtension(extension);
                size = entry.file_size();
                
                // Get last modified time
                auto time = std::filesystem::last_write_time(entry);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
                auto time_t = std::chrono::system_clock::to_time_t(sctp);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M");
                lastModified = ss.str();
            }
            
            files_.emplace_back(name, path, type, isDir, size, lastModified);
        }
        
        sortFiles();
    }
    catch (const std::exception& e) {
        // Handle directory access errors
        std::cerr << "Error loading directory: " << e.what() << std::endl;
    }
    
    // Update scrollbar visibility after loading files
    updateScrollbarVisibility();
}

void FileBrowser::sortFiles() {
    std::sort(files_.begin(), files_.end(), [this](const FileItem& a, const FileItem& b) {
        // Always put directories first
        if (a.isDirectory != b.isDirectory) {
            return a.isDirectory > b.isDirectory;
        }
        
        // Handle ".." entry
        if (a.name == "..") return true;
        if (b.name == "..") return false;
        
        bool result = false;
        
        switch (sortMode_) {
            case SortMode::Name:
                result = a.name < b.name;
                break;
            case SortMode::Type:
                result = a.type < b.type;
                break;
            case SortMode::Size:
                result = a.size < b.size;
                break;
            case SortMode::Modified:
                result = a.lastModified < b.lastModified;
                break;
        }
        
        return sortAscending_ ? result : !result;
    });
}

FileType FileBrowser::getFileTypeFromExtension(const std::string& extension) {
    if (extension == ".wav") return FileType::WavFile;
    if (extension == ".mp3") return FileType::Mp3File;
    if (extension == ".flac") return FileType::FlacFile;
    if (extension == ".aiff" || extension == ".aif") return FileType::AudioFile;
    if (extension == ".nomad" || extension == ".nmd") return FileType::ProjectFile;
    if (extension == ".mid" || extension == ".midi") return FileType::MusicFile;
    
    return FileType::Unknown;
}

std::shared_ptr<NUIIcon> FileBrowser::getIconForFileType(FileType type) {
    switch (type) {
        case FileType::Folder:
            return folderIcon_;
        case FileType::AudioFile:
            return audioFileIcon_;
        case FileType::MusicFile:
            return musicFileIcon_;
        case FileType::ProjectFile:
            return projectFileIcon_;
        case FileType::WavFile:
            return wavFileIcon_;
        case FileType::Mp3File:
            return mp3FileIcon_;
        case FileType::FlacFile:
            return flacFileIcon_;
        default:
            return unknownFileIcon_;
    }
}

void FileBrowser::renderFileList(NUIRenderer& renderer) {
    NUIRect bounds = getBounds();
    float listY = bounds.y + 60; // Below toolbar and path bar
    float listHeight = bounds.height - 60;
    
    // Render file items
    for (int i = 0; i < static_cast<int>(files_.size()); ++i) {
        float itemY = listY + (i * itemHeight_) - scrollOffset_;
        
        // Skip items that are not visible
        if (itemY + itemHeight_ < listY || itemY > listY + listHeight) {
            continue;
        }
        
        NUIRect itemRect(bounds.x + 8, itemY, bounds.width - 16, itemHeight_); // Add padding to prevent bleeding
        
        // Enhanced selection highlighting with Liminal Dark v2.0 effects
        if (i == selectedIndex_) {
            // Heavy selection highlight with cyan accent
            renderer.fillRoundedRect(itemRect, 4, selectedColor_.withAlpha(0.3f));
            renderer.strokeRoundedRect(itemRect, 4, 2, selectedColor_.withAlpha(0.8f));
            
            // Left accent bar in cyan for selected items
            NUIRect accentBar(itemRect.x, itemRect.y, 3, itemRect.height);
            renderer.fillRoundedRect(accentBar, 1, selectedColor_);
        } else if (i == hoveredIndex_) {
            // Hover highlight with subtle grey background
            renderer.fillRoundedRect(itemRect, 4, hoverColor_.withAlpha(0.4f));
            renderer.strokeRoundedRect(itemRect, 4, 1, hoverColor_.withAlpha(0.6f));
        } else {
            // Subtle background for all items
            renderer.fillRoundedRect(itemRect, 4, backgroundColor_.withAlpha(0.02f));
        }
        
        // Add subtle divider line between items (except for the last item)
        if (i < files_.size() - 1) {
            NUIRect dividerRect(itemRect.x + 8, itemRect.y + itemRect.height - 1, itemRect.width - 16, 1);
            renderer.fillRect(dividerRect, NUIColor(0.0f, 0.0f, 0.0f, 0.3f)); // Black divider
        }
        
        // Render icon
        auto icon = getIconForFileType(files_[i].type);
        if (icon) {
            NUIRect iconRect(itemRect.x + 8, itemY + 4, 24, 24);
            icon->setBounds(iconRect);
            icon->onRender(renderer);
        } else {
            // Debug: Draw a simple rectangle if no icon
            renderer.fillRect(NUIRect(itemRect.x + 8, itemY + 4, 24, 24), NUIColor::white().withAlpha(0.3f));
        }
        
        // Render file name with proper vertical alignment
        float textX = itemRect.x + 40;
        float textY = itemY + itemHeight_ / 2 + 7; // Perfect vertical centering for 14px font (baseline)
        
        // Add hover effect
        NUIColor nameColor = textColor_;
        if (i == selectedIndex_) {
            nameColor = NUIColor::white(); // Bright white for selected
        }
        
        renderer.drawText(files_[i].name, NUIPoint(textX, textY), 14, nameColor);
        
        // Render file size (for files only)
        if (!files_[i].isDirectory && files_[i].size > 0) {
            std::string sizeStr;
            if (files_[i].size < 1024) {
                sizeStr = std::to_string(files_[i].size) + " B";
            } else if (files_[i].size < 1024 * 1024) {
                sizeStr = std::to_string(files_[i].size / 1024) + " KB";
            } else {
                sizeStr = std::to_string(files_[i].size / (1024 * 1024)) + " MB";
            }
            
            auto sizeText = renderer.measureText(sizeStr, 12);
            renderer.drawText(sizeStr, NUIPoint(itemRect.x + itemRect.width - sizeText.width - 8, textY), 12, textColor_.withAlpha(0.7f));
        }
    }
}

void FileBrowser::renderPathBar(NUIRenderer& renderer) {
    NUIRect bounds = getBounds();
    NUIRect pathRect(bounds.x + 8, bounds.y + 40, bounds.width - 24, 20); // More space for text
    
    // Render path bar background with enhanced styling
    renderer.fillRoundedRect(pathRect, 4, backgroundColor_.darkened(0.15f));
    renderer.strokeRoundedRect(pathRect, 4, 1, borderColor_.withAlpha(0.6f));
    
    // Render current path with proper vertical alignment and padding
    float textY = pathRect.y + pathRect.height / 2 + 4; // Better centering
    float textX = pathRect.x + 12; // More padding from left edge
    float maxTextWidth = pathRect.width - 24; // Account for padding on both sides
    
    // Truncate text if it's too long to prevent bleeding
    std::string displayPath = currentPath_;
    if (displayPath.length() > 40) { // More aggressive truncation
        displayPath = "..." + displayPath.substr(displayPath.length() - 37);
    }
    
    // Ensure text fits within bounds
    renderer.drawText(displayPath, NUIPoint(textX, textY), 12, textColor_);
}

void FileBrowser::renderToolbar(NUIRenderer& renderer) {
    NUIRect bounds = getBounds();
    NUIRect toolbarRect(bounds.x + 8, bounds.y + 8, bounds.width - 16, 24);
    
    // Render toolbar background with enhanced styling
    renderer.fillRoundedRect(toolbarRect, 4, backgroundColor_.darkened(0.08f));
    renderer.strokeRoundedRect(toolbarRect, 4, 1, borderColor_.withAlpha(0.3f));
    
    // Render refresh button with proper vertical alignment
    float textY = toolbarRect.y + toolbarRect.height / 2 + 6; // Perfect centering for 12px font (baseline)
    renderer.drawText("Refresh", NUIPoint(toolbarRect.x + 8, textY), 12, textColor_);
    
    // Render sort mode indicator with proper alignment
    std::string sortText = "Sort: ";
    switch (sortMode_) {
        case SortMode::Name: sortText += "Name"; break;
        case SortMode::Type: sortText += "Type"; break;
        case SortMode::Size: sortText += "Size"; break;
        case SortMode::Modified: sortText += "Modified"; break;
    }
    sortText += sortAscending_ ? " ↑" : " ↓";
    
    auto sortTextSize = renderer.measureText(sortText, 12);
    renderer.drawText(sortText, NUIPoint(toolbarRect.x + toolbarRect.width - sortTextSize.width - 8, textY), 12, textColor_.withAlpha(0.7f));
}

void FileBrowser::updateScrollPosition() {
    if (selectedIndex_ < 0) return;
    
    NUIRect bounds = getBounds();
    float listY = bounds.y + 60;
    float listHeight = bounds.height - 60;
    
    float itemY = listY + (selectedIndex_ * itemHeight_) - scrollOffset_;
    
    // Scroll up if item is above visible area
    if (itemY < listY) {
        scrollOffset_ = selectedIndex_ * itemHeight_;
    }
    // Scroll down if item is below visible area
    else if (itemY + itemHeight_ > listY + listHeight) {
        scrollOffset_ = (selectedIndex_ + 1) * itemHeight_ - listHeight;
    }
    
    // Clamp scroll offset
    float maxScroll = std::max(0.0f, (files_.size() * itemHeight_) - listHeight);
    scrollOffset_ = std::max(0.0f, std::min(scrollOffset_, maxScroll));
    
    // Update scrollbar visibility and position
    updateScrollbarVisibility();
}

void FileBrowser::renderScrollbar(NUIRenderer& renderer) {
    // Check if we need a scrollbar
    float contentHeight = files_.size() * itemHeight_;
    float maxScroll = std::max(0.0f, contentHeight - scrollbarTrackHeight_);
    bool needsScrollbar = maxScroll > 0.0f;
    
    if (!needsScrollbar || files_.size() == 0) return;
    
    NUIRect bounds = getBounds();
    float scrollbarX = bounds.x + bounds.width - scrollbarWidth_ - 1; // Perfectly flush with right edge
    float scrollbarY = bounds.y + 60; // Below toolbar and path bar
    float scrollbarHeight = bounds.height - 60; // Full height to bottom
    
    // Render scrollbar track
    NUIColor trackColor = NUIColor(0.3f, 0.3f, 0.3f, 0.8f);
    renderer.fillRoundedRect(NUIRect(scrollbarX, scrollbarY, scrollbarWidth_, scrollbarHeight), 
                            4, trackColor);
    
    // Render scrollbar thumb
    NUIColor thumbColor = NUIColor(0.8f, 0.8f, 0.8f, 1.0f);
    float thumbHeight = std::max(20.0f, scrollbarHeight * 0.2f); // 20% of track height
    float thumbY = scrollbarY + (scrollOffset_ / std::max(1.0f, (files_.size() * itemHeight_) - scrollbarHeight)) * (scrollbarHeight - thumbHeight);
    renderer.fillRoundedRect(NUIRect(scrollbarX, thumbY, scrollbarWidth_, thumbHeight), 
                            4, thumbColor);
}

bool FileBrowser::handleScrollbarMouseEvent(const NUIMouseEvent& event) {
    NUIRect bounds = getBounds();
    float scrollbarX = bounds.x + bounds.width - scrollbarWidth_ - 1; // Perfectly flush with right edge
    float scrollbarY = bounds.y + 60;
    
    // If we're dragging, continue dragging regardless of mouse position
    if (isDraggingScrollbar_) {
        // Stop dragging on mouse release (anywhere, not just in scrollbar area)
        if (!event.pressed && event.button == NUIMouseButton::Left) {
            isDraggingScrollbar_ = false;
            return true;
        }
        
        // Continue dragging even if mouse is outside scrollbar area
        float deltaY = event.position.y - dragStartY_;
        float scrollRatio = deltaY / scrollbarTrackHeight_;
        float maxScroll = std::max(0.0f, (files_.size() * itemHeight_) - scrollbarTrackHeight_);
        
        // Direct scrolling for responsive dragging
        scrollOffset_ = dragStartScrollOffset_ + (scrollRatio * maxScroll);
        
        // Clamp scroll offset
        scrollOffset_ = std::max(0.0f, std::min(scrollOffset_, maxScroll));
        updateScrollbarVisibility();
        return true;
    }
    
    // Check if mouse is over scrollbar area (only for starting drag/click)
    bool inScrollbarArea = (event.position.x >= scrollbarX && event.position.x <= scrollbarX + scrollbarWidth_ &&
                           event.position.y >= scrollbarY && event.position.y <= scrollbarY + scrollbarTrackHeight_);
    
    if (!inScrollbarArea) {
        return false;
    }
    
    
    if (event.pressed && event.button == NUIMouseButton::Left) {
        // Check if clicking on thumb or track
        if (event.position.y >= scrollbarY + scrollbarThumbY_ && 
            event.position.y <= scrollbarY + scrollbarThumbY_ + scrollbarThumbHeight_) {
            // Start dragging thumb
            isDraggingScrollbar_ = true;
            dragStartY_ = event.position.y;
            dragStartScrollOffset_ = scrollOffset_;
        } else {
            // Click on track - jump to position
            float relativeY = event.position.y - scrollbarY;
            float scrollRatio = relativeY / scrollbarTrackHeight_;
            float maxScroll = std::max(0.0f, (files_.size() * itemHeight_) - scrollbarTrackHeight_);
            scrollOffset_ = scrollRatio * maxScroll;
            
            // Clamp scroll offset
            scrollOffset_ = std::max(0.0f, std::min(scrollOffset_, maxScroll));
            updateScrollbarVisibility();
        }
        return true;
    } else if (!event.pressed && event.button == NUIMouseButton::Left) {
        // Stop dragging
        isDraggingScrollbar_ = false;
        return true;
    }
    
    return false;
}

void FileBrowser::updateScrollbarVisibility() {
    // Calculate if we need a scrollbar
    float contentHeight = files_.size() * itemHeight_;
    float maxScroll = std::max(0.0f, contentHeight - scrollbarTrackHeight_);
    bool needsScrollbar = maxScroll > 0.0f;
    
    if (needsScrollbar) {
        scrollbarVisible_ = true;
        scrollbarOpacity_ = 1.0f;
        
        // Calculate thumb height (proportional to visible area)
        scrollbarThumbHeight_ = std::max(20.0f, (scrollbarTrackHeight_ / contentHeight) * scrollbarTrackHeight_);
        
        // Calculate thumb position based on scroll offset
        if (maxScroll > 0.0f) {
            scrollbarThumbY_ = (scrollOffset_ / maxScroll) * (scrollbarTrackHeight_ - scrollbarThumbHeight_);
        } else {
            scrollbarThumbY_ = 0.0f;
        }
        
    } else {
        scrollbarVisible_ = false;
        scrollbarOpacity_ = 0.0f;
    }
}

} // namespace NomadUI
