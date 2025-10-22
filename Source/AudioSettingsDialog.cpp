/**
 * @file AudioSettingsDialog.cpp
 * @brief Audio settings dialog implementation
 */

#include "AudioSettingsDialog.h"
#include "../NomadAudio/include/AudioDeviceManager.h"
#include "../NomadUI/Core/NUIThemeSystem.h"
#include "../NomadUI/Graphics/NUIRenderer.h"
#include "../NomadCore/include/NomadLog.h"
#include <sstream>

namespace Nomad {

AudioSettingsDialog::AudioSettingsDialog(Nomad::Audio::AudioDeviceManager* audioManager)
    : NomadUI::NUIComponent()
    , m_audioManager(audioManager)
    , m_visible(false)
    , m_selectedDeviceIndex(0)
    , m_selectedDeviceId(0)
    , m_selectedSampleRateIndex(0)
    , m_selectedSampleRate(48000)
    , m_selectedBufferSizeIndex(0)
    , m_selectedBufferSize(512)
{
    // Initialize dropdown system
    
    // Common buffer sizes
    m_bufferSizes = {64, 128, 256, 512, 1024, 2048};
    
    // Initialize dialog bounds - will be set properly when shown
    m_dialogBounds = NomadUI::NUIRect(0, 0, 500, 420);
    
    // Initialize as hidden but enabled
    setVisible(false);
    setEnabled(true);
    
    createUI();
    updateDeviceList();
    loadCurrentSettings();
    
    // Layout components immediately to set proper dropdown bounds
    // Set initial layout
    layoutComponents();
    
    // Constructor completed
}

void AudioSettingsDialog::createUI() {
    // Create dropdowns
    m_deviceDropdown = std::make_shared<NomadUI::NUIDropdown>();
    m_deviceDropdown->setId("device_dropdown");
    m_deviceDropdown->setPlaceholderText("Select Audio Device");
    m_deviceDropdown->setOnSelectionChanged([this](int index, int value, const std::string& text) {
        m_selectedDeviceIndex = index;
        m_selectedDeviceId = static_cast<uint32_t>(value);
        std::cout << "Selected device: " << text << " (ID: " << value << ")" << std::endl;
    });
    addChild(m_deviceDropdown);
    m_deviceDropdown->registerWithManager();
    
    m_sampleRateDropdown = std::make_shared<NomadUI::NUIDropdown>();
    m_sampleRateDropdown->setId("sample_rate_dropdown");
    m_sampleRateDropdown->setPlaceholderText("Select Sample Rate");
    m_sampleRateDropdown->setOnSelectionChanged([this](int index, int value, const std::string& text) {
        m_selectedSampleRateIndex = index;
        m_selectedSampleRate = static_cast<uint32_t>(value);
        std::cout << "Selected sample rate: " << text << " Hz" << std::endl;
    });
    addChild(m_sampleRateDropdown);
    m_sampleRateDropdown->registerWithManager();
    
    m_bufferSizeDropdown = std::make_shared<NomadUI::NUIDropdown>();
    m_bufferSizeDropdown->setId("buffer_size_dropdown");
    m_bufferSizeDropdown->setPlaceholderText("Select Buffer Size");
    m_bufferSizeDropdown->setOnSelectionChanged([this](int index, int value, const std::string& text) {
        m_selectedBufferSizeIndex = index;
        m_selectedBufferSize = static_cast<uint32_t>(value);
        std::cout << "Selected buffer size: " << text << " samples" << std::endl;
    });
    addChild(m_bufferSizeDropdown);
    m_bufferSizeDropdown->registerWithManager();
    
    // Create buttons
    m_applyButton = std::make_shared<NomadUI::NUIButton>();
    m_applyButton->setText("Apply");
    m_applyButton->setOnClick([this]() { applySettings(); });
    addChild(m_applyButton);
    
    m_cancelButton = std::make_shared<NomadUI::NUIButton>();
    m_cancelButton->setText("Cancel");
    m_cancelButton->setOnClick([this]() { cancelSettings(); });
    addChild(m_cancelButton);
}

void AudioSettingsDialog::show() {
    std::cout << "===== AudioSettingsDialog::show() called =====" << std::endl;
    std::cout << "Current m_visible: " << m_visible << std::endl;
    m_visible = true;
    std::cout << "Set m_visible to true" << std::endl;
    
    // Position the dialog in the center of the window
    float dialogWidth = 500.0f;
    float dialogHeight = 420.0f;
    m_dialogBounds = NomadUI::NUICentered(NomadUI::NUIRect(0, 0, 1280, 720), dialogWidth, dialogHeight);
    setBounds(m_dialogBounds);
    
    std::cout << "[show] Dialog positioned at: (" << m_dialogBounds.x << "," << m_dialogBounds.y << "," << m_dialogBounds.width << "," << m_dialogBounds.height << ")" << std::endl;
    
    loadCurrentSettings();
    layoutComponents(); // Add this line to set dropdown bounds
    setDirty(true);
    
    // Ensure we're visible and can receive events
    setVisible(true);
    setEnabled(true);
    setFocused(true);
    std::cout << "===== AudioSettingsDialog::show() completed =====" << std::endl;
}

void AudioSettingsDialog::hide() {
    m_visible = false;
    setDirty(true);
    
    // Hide the component and remove focus
    setVisible(false);
    setEnabled(false);
    setFocused(false);
}

void AudioSettingsDialog::setVisible(bool visible) {
    // Set visibility and update layout
    
    // Call base class implementation
    NUIComponent::setVisible(visible);
    
    // If becoming visible, ensure layout is updated
    if (visible) {
        // Update layout when becoming visible
        layoutComponents();
    }
    
    // Visibility set
}

void AudioSettingsDialog::updateDeviceList() {
    if (!m_audioManager) return;
    
    m_devices = m_audioManager->getDevices();
    
    // Clear and populate dropdown
    m_deviceDropdown->clear();
    for (const auto& device : m_devices) {
        m_deviceDropdown->addItem(device.name, static_cast<int>(device.id));
    }
    
    // Find current device
    auto currentConfig = m_audioManager->getCurrentConfig();
    for (size_t i = 0; i < m_devices.size(); ++i) {
        if (m_devices[i].id == currentConfig.deviceId) {
            m_selectedDeviceIndex = static_cast<int>(i);
            m_selectedDeviceId = m_devices[i].id;
            m_deviceDropdown->setSelectedIndex(static_cast<int>(i));
            break;
        }
    }
}

void AudioSettingsDialog::updateSampleRateList() {
    m_sampleRates.clear();
    
    if (m_selectedDeviceIndex >= 0 && m_selectedDeviceIndex < static_cast<int>(m_devices.size())) {
        m_sampleRates = m_devices[m_selectedDeviceIndex].supportedSampleRates;
    }
    
    // If empty, use common sample rates
    if (m_sampleRates.empty()) {
        m_sampleRates = {44100, 48000, 88200, 96000};
    }
    
    // Clear and populate dropdown
    m_sampleRateDropdown->clear();
    for (uint32_t rate : m_sampleRates) {
        m_sampleRateDropdown->addItem(std::to_string(rate) + " Hz", static_cast<int>(rate));
    }
    
    // Find current sample rate
    for (size_t i = 0; i < m_sampleRates.size(); ++i) {
        if (m_sampleRates[i] == m_selectedSampleRate) {
            m_selectedSampleRateIndex = static_cast<int>(i);
            m_sampleRateDropdown->setSelectedIndex(static_cast<int>(i));
            return;
        }
    }
    
    // Default to first if not found
    if (!m_sampleRates.empty()) {
        m_selectedSampleRateIndex = 0;
        m_selectedSampleRate = m_sampleRates[0];
        m_sampleRateDropdown->setSelectedIndex(0);
    }
}

void AudioSettingsDialog::updateBufferSizeList() {
    // Clear and populate dropdown
    m_bufferSizeDropdown->clear();
    for (uint32_t size : m_bufferSizes) {
        m_bufferSizeDropdown->addItem(std::to_string(size) + " samples", static_cast<int>(size));
    }
    
    // Find current buffer size
    for (size_t i = 0; i < m_bufferSizes.size(); ++i) {
        if (m_bufferSizes[i] == m_selectedBufferSize) {
            m_selectedBufferSizeIndex = static_cast<int>(i);
            m_bufferSizeDropdown->setSelectedIndex(static_cast<int>(i));
            return;
        }
    }
    
    // Default to first if not found
    if (!m_bufferSizes.empty()) {
        m_selectedBufferSizeIndex = 0;
        m_selectedBufferSize = m_bufferSizes[0];
        m_bufferSizeDropdown->setSelectedIndex(0);
    }
}

void AudioSettingsDialog::loadCurrentSettings() {
    if (!m_audioManager) return;
    
    auto config = m_audioManager->getCurrentConfig();
    
    m_originalDeviceId = config.deviceId;
    m_originalSampleRate = config.sampleRate;
    m_originalBufferSize = config.bufferSize;
    
    m_selectedDeviceId = config.deviceId;
    m_selectedSampleRate = config.sampleRate;
    m_selectedBufferSize = config.bufferSize;
    
    updateDeviceList();
    updateSampleRateList();
    updateBufferSizeList();
}

void AudioSettingsDialog::applySettings() {
    if (!m_audioManager) return;
    
    std::stringstream ss;
    ss << "Applying audio settings: Device=" << m_selectedDeviceId 
       << ", SampleRate=" << m_selectedSampleRate 
       << ", BufferSize=" << m_selectedBufferSize;
    Log::info(ss.str());
    
    // Apply settings through audio manager
    bool success = true;
    
    if (m_selectedDeviceId != m_originalDeviceId) {
        success = m_audioManager->switchDevice(m_selectedDeviceId);
    }
    
    if (success && m_selectedSampleRate != m_originalSampleRate) {
        success = m_audioManager->setSampleRate(m_selectedSampleRate);
    }
    
    if (success && m_selectedBufferSize != m_originalBufferSize) {
        success = m_audioManager->setBufferSize(m_selectedBufferSize);
    }
    
    if (success) {
        Log::info("Audio settings applied successfully");
        if (m_onApply) {
            m_onApply();
        }
        hide();
    } else {
        Log::error("Failed to apply audio settings");
    }
}

void AudioSettingsDialog::cancelSettings() {
    Log::info("Audio settings cancelled");
    
    // Restore original settings
    m_selectedDeviceId = m_originalDeviceId;
    m_selectedSampleRate = m_originalSampleRate;
    m_selectedBufferSize = m_originalBufferSize;
    
    if (m_onCancel) {
        m_onCancel();
    }
    hide();
}

void AudioSettingsDialog::onRender(NomadUI::NUIRenderer& renderer) {
    if (!m_visible) return;
    
    // Ensure layout is updated on first render
    static bool layoutInitialized = false;
    if (!layoutInitialized) {
        std::cout << "First render, calling layoutComponents()" << std::endl;
        layoutComponents();
        layoutInitialized = true;
    }
    
    renderBackground(renderer);
    renderDialog(renderer);
    
    // Render all children (dropdowns and original buttons)
    renderChildren(renderer);
}

void AudioSettingsDialog::renderBackground(NomadUI::NUIRenderer& renderer) {
    NomadUI::NUIRect bounds = getBounds();
    
    // Semi-transparent overlay
    renderer.fillRect(bounds, NomadUI::NUIColor(0.0f, 0.0f, 0.0f, 0.5f));
}

void AudioSettingsDialog::renderDialog(NomadUI::NUIRenderer& renderer) {
    // Always call layoutComponents() to ensure bounds are set
    layoutComponents();
    
    auto& themeManager = NomadUI::NUIThemeManager::getInstance();
    
    // Get theme colors
    NomadUI::NUIColor bgColor = themeManager.getColor("surfaceRaised");
    NomadUI::NUIColor borderColor = themeManager.getColor("borderSubtle");
    NomadUI::NUIColor textColor = themeManager.getColor("textPrimary");
    
    // FL Studio-inspired dialog styling
    // Add subtle drop shadow for depth
    NomadUI::NUIRect shadowBounds = m_dialogBounds;
    shadowBounds.x += 2.0f;
    shadowBounds.y += 2.0f;
    renderer.fillRoundedRect(shadowBounds, 8, NomadUI::NUIColor(0, 0, 0, 0.2f));
    
    // Draw dialog background with modern styling
    renderer.fillRoundedRect(m_dialogBounds, 8, bgColor);
    renderer.strokeRoundedRect(m_dialogBounds, 8, 1, borderColor);
    
    // FL Studio-style title
    std::string title = "Audio Settings";
    float titleSize = 18.0f;  // Slightly smaller for cleaner look
    auto titleBounds = renderer.measureText(title, titleSize);
    float titleX = m_dialogBounds.x + (m_dialogBounds.width - titleBounds.width) / 2.0f;
    float titleY = m_dialogBounds.y + 25.0f;  // More padding from top
    renderer.drawText(title, NomadUI::NUIPoint(titleX, titleY), titleSize, textColor);
    
    // FL Studio-style labels for dropdowns
    float labelSize = 13.0f;  // Slightly smaller for cleaner look
    float padding = 20.0f;
    float dropdownHeight = 32.0f;
    float dropdownSpacing = 50.0f;  // Match layoutComponents() - balanced spacing
    float labelHeight = 20.0f;  // Match layoutComponents()
    
    float currentY = m_dialogBounds.y + padding + 80.0f;  // Match layoutComponents() - moved down
    
    // FL Studio-style label colors
    NomadUI::NUIColor labelColor = themeManager.getColor("textSecondary");
    
    // Device label - positioned above the dropdown
    renderer.drawText("Audio Device:", 
                     NomadUI::NUIPoint(m_dialogBounds.x + padding, currentY - 25.0f), 
                     labelSize, labelColor);
    
    // Sample rate label - positioned above the dropdown
    renderer.drawText("Sample Rate:", 
                     NomadUI::NUIPoint(m_dialogBounds.x + padding, currentY + dropdownHeight + dropdownSpacing - 25.0f), 
                     labelSize, labelColor);
    
    // Buffer size label - positioned above the dropdown
    renderer.drawText("Buffer Size:", 
                     NomadUI::NUIPoint(m_dialogBounds.x + padding, currentY + (dropdownHeight + dropdownSpacing) * 2 - 25.0f), 
                     labelSize, labelColor);
}


void AudioSettingsDialog::onResize(int width, int height) {
    // Handle resize - just update the dialog size, positioning is handled in show()
    float dialogWidth = 500.0f;
    float dialogHeight = 420.0f;
    m_dialogBounds.width = dialogWidth;
    m_dialogBounds.height = dialogHeight;
    
    std::cout << "[onResize] Dialog resized to: " << width << "x" << height << std::endl;
    
    layoutComponents();
    std::cout << "===== AudioSettingsDialog::onResize() completed =====" << std::endl;
}

void AudioSettingsDialog::layoutComponents() {
    std::cout << "===== layoutComponents() called =====" << std::endl;
    
    float padding = 20.0f;
    float dropdownHeight = 32.0f;
    float dropdownSpacing = 50.0f;  // Balanced spacing for better alignment
    float labelSize = 14.0f;
    float labelHeight = 20.0f;  // Height for label text
    
    // Position dropdowns - back to working position
    float currentY = m_dialogBounds.y + padding + 80.0f; // Back to working position
    std::cout << "Dialog Y: " << m_dialogBounds.y << ", currentY: " << currentY << std::endl;
    // Dropdown positioning debug
    std::cout << "=== DROPDOWN POSITIONING ===" << std::endl;
    std::cout << "Dialog bounds: (" << m_dialogBounds.x << ", " << m_dialogBounds.y 
              << ", " << m_dialogBounds.width << ", " << m_dialogBounds.height << ")" << std::endl;
    std::cout << "currentY start: " << currentY << std::endl;
    
    // Device dropdown - use absolute window coordinates
    m_deviceDropdown->setBounds(NomadUI::NUIRect(m_dialogBounds.x + padding, 
                                                 currentY, 
                                                 m_dialogBounds.width - padding * 2, 
                                                 dropdownHeight));
    std::cout << "Device dropdown: (" << (m_dialogBounds.x + padding) << ", " << currentY 
              << ", " << (m_dialogBounds.width - padding * 2) << ", " << dropdownHeight << ")" << std::endl;
    currentY += dropdownHeight + dropdownSpacing;
    
    // Sample rate dropdown - use absolute coordinates directly
    m_sampleRateDropdown->setBounds(NomadUI::NUIRect(m_dialogBounds.x + padding, 
                                                     currentY, 
                                                     m_dialogBounds.width - padding * 2, 
                                                     dropdownHeight));
    std::cout << "Sample rate dropdown: (" << (m_dialogBounds.x + padding) << ", " << currentY 
              << ", " << (m_dialogBounds.width - padding * 2) << ", " << dropdownHeight << ")" << std::endl;
    currentY += dropdownHeight + dropdownSpacing;
    
    // Buffer size dropdown - use absolute coordinates directly
    m_bufferSizeDropdown->setBounds(NomadUI::NUIRect(m_dialogBounds.x + padding, 
                                                     currentY, 
                                                     m_dialogBounds.width - padding * 2, 
                                                     dropdownHeight));
    std::cout << "Buffer size dropdown: (" << (m_dialogBounds.x + padding) << ", " << currentY 
              << ", " << (m_dialogBounds.width - padding * 2) << ", " << dropdownHeight << ")" << std::endl;
    
    // Debug: Print final dropdown positions
    std::cout << "=== FINAL DROPDOWN POSITIONS ===" << std::endl;
    std::cout << "Device dropdown: (" << m_deviceDropdown->getBounds().x << ", " << m_deviceDropdown->getBounds().y 
              << ", " << m_deviceDropdown->getBounds().width << ", " << m_deviceDropdown->getBounds().height << ")" << std::endl;
    std::cout << "Sample rate dropdown: (" << m_sampleRateDropdown->getBounds().x << ", " << m_sampleRateDropdown->getBounds().y 
              << ", " << m_sampleRateDropdown->getBounds().width << ", " << m_sampleRateDropdown->getBounds().height << ")" << std::endl;
    std::cout << "Buffer size dropdown: (" << m_bufferSizeDropdown->getBounds().x << ", " << m_bufferSizeDropdown->getBounds().y 
              << ", " << m_bufferSizeDropdown->getBounds().width << ", " << m_bufferSizeDropdown->getBounds().height << ")" << std::endl;
    
    // Position buttons at bottom of dialog - compact and sleek
    float buttonWidth = 70.0f;   // Reduced from 100 to 70
    float buttonHeight = 28.0f;  // Reduced from 36 to 28
    float buttonSpacing = 8.0f;  // Reduced from 10 to 8
    
    float buttonY = m_dialogBounds.y + m_dialogBounds.height - buttonHeight - padding;
    float cancelX = m_dialogBounds.x + m_dialogBounds.width - buttonWidth - padding;
    float applyX = cancelX - buttonWidth - buttonSpacing;
    
    m_applyButton->setBounds(NomadUI::NUIRect(applyX, buttonY, buttonWidth, buttonHeight));
    
    m_cancelButton->setBounds(NomadUI::NUIRect(cancelX, buttonY, buttonWidth, buttonHeight));
}

bool AudioSettingsDialog::onMouseEvent(const NomadUI::NUIMouseEvent& event) {
    if (!m_visible) return false;
    
    // Dropdown debugging only
    std::cout << "===== AudioSettingsDialog::onMouseEvent =====" << std::endl;
    std::cout << "Mouse position: (" << event.position.x << ", " << event.position.y << ")" << std::endl;
    
    // Check if click is outside dialog (close on background click)
    if (event.pressed && event.button == NomadUI::NUIMouseButton::Left) {
        if (!m_dialogBounds.contains(event.position)) {
            std::cout << "Clicked outside dialog, closing" << std::endl;
            cancelSettings();
            return true;
        }
    }
    
    // Let children handle events (dropdowns will handle their own clicks)
    
    std::cout << "Calling base class onMouseEvent for event propagation..." << std::endl;
    bool handled = NomadUI::NUIComponent::onMouseEvent(event);
    std::cout << "Base class onMouseEvent returned: " << handled << std::endl;
    return handled;
}

bool AudioSettingsDialog::onKeyEvent(const NomadUI::NUIKeyEvent& event) {
    if (!m_visible) return false;
    
    // ESC to cancel
    if (event.pressed && event.keyCode == NomadUI::NUIKeyCode::Escape) {
        cancelSettings();
        return true;
    }
    
    // Enter to apply
    if (event.pressed && event.keyCode == NomadUI::NUIKeyCode::Enter) {
        applySettings();
        return true;
    }
    
    return false;
}

} // namespace Nomad
