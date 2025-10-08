#include "FileBrowserComponent.h"
#include "PlaylistComponent.h"

FileExplorerPanel::FileExplorerPanel()
    : fileFilter("*", "*", "All Files"),
      directoryThread("File Browser Thread")
{
    // Purple theme color
    juce::Colour purpleGlow(0xffa855f7);
    
    // Start the directory scanning thread
    directoryThread.startThread(juce::Thread::Priority::low);
    
    // Set initial root to user's documents folder
    currentRoot = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    
    // Create directory list
    directoryList = std::make_unique<juce::DirectoryContentsList>(&fileFilter, directoryThread);
    directoryList->setDirectory(currentRoot, true, true);
    
    // Create file tree
    fileTree = std::make_unique<juce::FileTreeComponent>(*directoryList);
    fileTree->addListener(this);
    fileTree->setDragAndDropDescription("AudioFiles"); // Enable drag and drop
    fileTree->setColour(juce::FileTreeComponent::backgroundColourId, juce::Colour(0xff0d0e0f));
    fileTree->setColour(juce::FileTreeComponent::linesColourId, purpleGlow.withAlpha(0.2f));
    fileTree->setColour(juce::FileTreeComponent::dragAndDropIndicatorColourId, purpleGlow);
    fileTree->setColour(juce::TreeView::selectedItemBackgroundColourId, purpleGlow.withAlpha(0.3f));
    fileTree->setColour(juce::TreeView::oddItemsColourId, juce::Colours::transparentBlack);
    fileTree->setColour(juce::TreeView::evenItemsColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(fileTree.get());
    
    // Setup compact home button
    homeButton.setButtonText("Home");
    homeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a));
    homeButton.setColour(juce::TextButton::buttonOnColourId, purpleGlow.withAlpha(0.3f));
    homeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
    homeButton.setColour(juce::TextButton::textColourOnId, purpleGlow);
    homeButton.onClick = [this]
    {
        setRoot(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory));
    };
    addAndMakeVisible(homeButton);
    
    // Setup compact refresh button
    refreshButton.setButtonText("Refresh");
    refreshButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a));
    refreshButton.setColour(juce::TextButton::buttonOnColourId, purpleGlow.withAlpha(0.3f));
    refreshButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
    refreshButton.setColour(juce::TextButton::textColourOnId, purpleGlow);
    refreshButton.onClick = [this]
    {
        directoryList->refresh();
    };
    addAndMakeVisible(refreshButton);
    
    // Setup compact path label
    pathLabel.setJustificationType(juce::Justification::centredLeft);
    pathLabel.setFont(juce::Font(9.0f));
    pathLabel.setColour(juce::Label::textColourId, juce::Colour(0xff555555));
    pathLabel.setText(currentRoot.getFullPathName(), juce::dontSendNotification);
    addAndMakeVisible(pathLabel);
}

FileExplorerPanel::~FileExplorerPanel()
{
    fileTree.reset();
    directoryList.reset();
    directoryThread.stopThread(1000);
}

void FileExplorerPanel::paint(juce::Graphics& g)
{
    juce::Colour purpleGlow(0xffa855f7);
    
    // Dark background
    g.fillAll(juce::Colour(0xff0d0e0f));
    
    // Compact header background
    g.setColour(juce::Colour(0xff151618));
    g.fillRect(0, 0, getWidth(), 60);
    
    // Title - modern, sleek font
    g.setFont(juce::Font("Arial", 11.0f, juce::Font::plain));
    g.setColour(purpleGlow);
    g.drawText("BROWSER", 10, 8, getWidth() - 20, 18, juce::Justification::centredLeft);
    
    // Separator line with purple glow
    g.setColour(purpleGlow.withAlpha(0.3f));
    g.drawLine(0, 60, (float)getWidth(), 60, 2.0f);
    g.setColour(purpleGlow.withAlpha(0.5f));
    g.drawLine(0, 60, (float)getWidth(), 60, 1.0f);
}

void FileExplorerPanel::resized()
{
    auto bounds = getLocalBounds();
    
    // Compact header area
    auto header = bounds.removeFromTop(60);
    header.removeFromTop(26); // Skip title area
    
    // Compact buttons in one row
    auto buttonArea = header.removeFromTop(24).reduced(6, 3);
    int buttonWidth = (buttonArea.getWidth() - 3) / 2;
    homeButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(3);
    refreshButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    
    // Path label (smaller)
    pathLabel.setBounds(header.reduced(6, 2));
    
    // File tree takes remaining space
    fileTree->setBounds(bounds);
}

void FileExplorerPanel::selectionChanged()
{
    // Handle selection change if needed
}

void FileExplorerPanel::fileClicked(const juce::File& file, const juce::MouseEvent& e)
{
    if (file.isDirectory())
    {
        // Update path label
        pathLabel.setText(file.getFullPathName(), juce::dontSendNotification);
    }
    else if (e.mods.isPopupMenu() && isAudioFile(file))
    {
        // Right-click on audio file - show context menu
        showContextMenu(file, e);
    }
}

void FileExplorerPanel::fileDoubleClicked(const juce::File& file)
{
    if (file.isDirectory())
    {
        setRoot(file);
    }
    else
    {
        // Handle file double-click (e.g., load audio file)
        juce::Logger::writeToLog("Double-clicked: " + file.getFullPathName());
    }
}

void FileExplorerPanel::browserRootChanged(const juce::File& newRoot)
{
    currentRoot = newRoot;
    pathLabel.setText(newRoot.getFullPathName(), juce::dontSendNotification);
}

void FileExplorerPanel::setRoot(const juce::File& newRoot)
{
    if (newRoot.exists() && newRoot.isDirectory())
    {
        currentRoot = newRoot;
        directoryList->setDirectory(newRoot, true, true);
        pathLabel.setText(newRoot.getFullPathName(), juce::dontSendNotification);
    }
}

bool FileExplorerPanel::isAudioFile(const juce::File& file) const
{
    return file.hasFileExtension("wav") || file.hasFileExtension("mp3") || 
           file.hasFileExtension("flac") || file.hasFileExtension("ogg") || 
           file.hasFileExtension("aiff") || file.hasFileExtension("aif");
}

void FileExplorerPanel::showContextMenu(const juce::File& file, const juce::MouseEvent& e)
{
    juce::PopupMenu menu;
    juce::Colour purpleGlow(0xffa855f7);
    
    // Theme the context menu with purple style
    contextMenuLookAndFeel.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1a1a1a));
    contextMenuLookAndFeel.setColour(juce::PopupMenu::textColourId, juce::Colour(0xffcccccc));
    contextMenuLookAndFeel.setColour(juce::PopupMenu::headerTextColourId, purpleGlow);
    contextMenuLookAndFeel.setColour(juce::PopupMenu::highlightedBackgroundColourId, purpleGlow.withAlpha(0.3f));
    contextMenuLookAndFeel.setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    
    menu.setLookAndFeel(&contextMenuLookAndFeel);
    
    menu.addItem(1, "Load to Playlist", playlistComponent != nullptr);
    menu.addItem(2, "Load to Sequencer", false); // Not implemented yet
    menu.addSeparator();
    menu.addItem(3, "Show in Explorer");
    
    // Get the exact mouse down position
    auto mousePos = e.getMouseDownScreenPosition();
    
    // Show menu at exact mouse position
    menu.showMenuAsync(juce::PopupMenu::Options()
                          .withTargetScreenArea(juce::Rectangle<int>(mousePos.x, mousePos.y, 1, 1))
                          .withMinimumWidth(180)
                          .withStandardItemHeight(24),
        [this, file](int result)
        {
            if (result == 1 && playlistComponent != nullptr)
            {
                // Load to playlist
                juce::StringArray files;
                files.add(file.getFullPathName());
                playlistComponent->filesDropped(files, 0, 0);
            }
            else if (result == 3)
            {
                // Show in Explorer
                file.revealToUser();
            }
        });
}
