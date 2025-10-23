#pragma once

#include "../NomadUI/Core/NUIComponent.h"
#include "../NomadUI/Core/NUIIcon.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace NomadUI {

/**
 * File type enumeration for proper icon display
 */
enum class FileType {
    Folder,
    AudioFile,
    MusicFile,
    ProjectFile,
    WavFile,
    Mp3File,
    FlacFile,
    Unknown
};

/**
 * File item structure
 */
struct FileItem {
    std::string name;
    std::string path;
    FileType type;
    bool isDirectory;
    size_t size;
    std::string lastModified;
    
    FileItem(const std::string& n, const std::string& p, FileType t, bool isDir, size_t s = 0, const std::string& modified = "")
        : name(n), path(p), type(t), isDirectory(isDir), size(s), lastModified(modified) {}
};

/**
 * File Browser Component
 * 
 * A modern file browser with icons, sorting, and navigation.
 * Integrates with NomadUI theme system for consistent styling.
 */
class FileBrowser : public NUIComponent {
public:
    FileBrowser();
    ~FileBrowser() override = default;
    
    // Component interface
    void onRender(NUIRenderer& renderer) override;
    void onResize(int width, int height) override;
    bool onMouseEvent(const NUIMouseEvent& event) override;
    bool onKeyEvent(const NUIKeyEvent& event) override;
    
    // File browser functionality
    void setCurrentPath(const std::string& path);
    void refresh();
    void navigateUp();
    void navigateTo(const std::string& path);
    
    // File operations
    void selectFile(const std::string& path);
    void openFile(const std::string& path);
    void openFolder(const std::string& path);
    
    // Callbacks
    void setOnFileSelected(std::function<void(const FileItem&)> callback) { onFileSelected_ = callback; }
    void setOnFileOpened(std::function<void(const FileItem&)> callback) { onFileOpened_ = callback; }
    void setOnPathChanged(std::function<void(const std::string&)> callback) { onPathChanged_ = callback; }
    
    // Properties
    const std::string& getCurrentPath() const { return currentPath_; }
    const FileItem* getSelectedFile() const { return selectedFile_; }
    const std::vector<FileItem>& getFiles() const { return files_; }
    
    // Sorting
    enum class SortMode {
        Name,
        Type,
        Size,
        Modified
    };
    
    void setSortMode(SortMode mode);
    void setSortAscending(bool ascending);
    
private:
    void loadDirectoryContents();
    void sortFiles();
    FileType getFileTypeFromExtension(const std::string& extension);
    std::shared_ptr<NUIIcon> getIconForFileType(FileType type);
    void renderFileList(NUIRenderer& renderer);
    void renderPathBar(NUIRenderer& renderer);
    void renderToolbar(NUIRenderer& renderer);
    void renderScrollbar(NUIRenderer& renderer);
    void updateScrollPosition();
    bool handleScrollbarMouseEvent(const NUIMouseEvent& event);
    void updateScrollbarVisibility();
    
    // File management
    std::string currentPath_;
    std::vector<FileItem> files_;
    const FileItem* selectedFile_;
    int selectedIndex_;
    
    // UI state
    float scrollOffset_;
    float itemHeight_;
    int visibleItems_;
    bool showHiddenFiles_;
    
    // Scrollbar state
    bool scrollbarVisible_;
    float scrollbarOpacity_;
    float scrollbarWidth_;
    float scrollbarTrackHeight_;
    float scrollbarThumbHeight_;
    float scrollbarThumbY_;
    bool isDraggingScrollbar_;
    float dragStartY_;
    float dragStartScrollOffset_;
    float scrollbarFadeTimer_;
    static constexpr float SCROLLBAR_FADE_DELAY = 1.0f; // seconds
    static constexpr float SCROLLBAR_FADE_DURATION = 0.3f; // seconds
    
    // Hover state
    int hoveredIndex_;
    
    // Sorting
    SortMode sortMode_;
    bool sortAscending_;
    
    // Icons
    std::shared_ptr<NUIIcon> folderIcon_;
    std::shared_ptr<NUIIcon> folderOpenIcon_;
    std::shared_ptr<NUIIcon> audioFileIcon_;
    std::shared_ptr<NUIIcon> musicFileIcon_;
    std::shared_ptr<NUIIcon> projectFileIcon_;
    std::shared_ptr<NUIIcon> wavFileIcon_;
    std::shared_ptr<NUIIcon> mp3FileIcon_;
    std::shared_ptr<NUIIcon> flacFileIcon_;
    std::shared_ptr<NUIIcon> unknownFileIcon_;
    
    // Callbacks
    std::function<void(const FileItem&)> onFileSelected_;
    std::function<void(const FileItem&)> onFileOpened_;
    std::function<void(const std::string&)> onPathChanged_;
    
    // Theme colors
    NUIColor backgroundColor_;
    NUIColor textColor_;
    NUIColor selectedColor_;
    NUIColor hoverColor_;
    NUIColor borderColor_;
};

} // namespace NomadUI
