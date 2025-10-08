#pragma once

#include <JuceHeader.h>

/**
 * File explorer panel for the left sidebar
 * Shows folders and files with purple theme styling
 */
// Forward declaration
class PlaylistComponent;

class FileExplorerPanel : public juce::Component,
                          private juce::FileBrowserListener
{
public:
    FileExplorerPanel();
    ~FileExplorerPanel() override;
    
    void setPlaylistComponent(PlaylistComponent* playlist) { playlistComponent = playlist; }
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // FileBrowserListener
    void selectionChanged() override;
    void fileClicked(const juce::File& file, const juce::MouseEvent& e) override;
    void fileDoubleClicked(const juce::File& file) override;
    void browserRootChanged(const juce::File& newRoot) override;
    
private:
    juce::File currentRoot;
    juce::WildcardFileFilter fileFilter;
    juce::TimeSliceThread directoryThread;
    std::unique_ptr<juce::DirectoryContentsList> directoryList;
    std::unique_ptr<juce::FileTreeComponent> fileTree;
    
    juce::TextButton homeButton;
    juce::TextButton refreshButton;
    juce::Label pathLabel;
    
    PlaylistComponent* playlistComponent = nullptr;
    juce::LookAndFeel_V4 contextMenuLookAndFeel;
    
    void setRoot(const juce::File& newRoot);
    void showContextMenu(const juce::File& file, const juce::MouseEvent& e);
    bool isAudioFile(const juce::File& file) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileExplorerPanel)
};
