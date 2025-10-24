/**
 * @file AudioSettingsDialog.cpp
 * @brief Audio settings dialog for NOMAD DAW
 */

#include "AudioSettingsDialog.h"
#include "../NomadUI/Core/NUIThemeSystem.h"
#include "../NomadUI/Graphics/NUIRenderer.h"
#include "../NomadCore/include/NomadLog.h"
#include <iostream>

namespace Nomad {

AudioSettingsDialog::AudioSettingsDialog(Audio::AudioDeviceManager* audioManager)
    : m_audioManager(audioManager)
    , m_visible(false)
    , m_dialogBounds(0, 0, 500, 420)
    , m_selectedDeviceId(0)
    , m_selectedSampleRate(48000)
    , m_selectedBufferSize(512)
    , m_originalDeviceId(0)
    , m_originalSampleRate(48000)
    , m_originalBufferSize(512)
{
    createUI();
    loadCurrentSettings();
}

void AudioSettingsDialog::createUI() {
    // Create labels
    m_deviceLabel = std::make_shared<NomadUI::NUILabel>();
    m_deviceLabel->setText("Audio Device:");
    addChild(m_deviceLabel);

    m_sampleRateLabel = std::make_shared<NomadUI::NUILabel>();
    m_sampleRateLabel->setText("Sample Rate:");
    addChild(m_sampleRateLabel);

    m_bufferSizeLabel = std::make_shared<NomadUI::NUILabel>();
    m_bufferSizeLabel->setText("Buffer Size:");
    addChild(m_bufferSizeLabel);

    // Create dropdowns
    m_deviceDropdown = std::make_shared<NomadUI::NUIDropdown>();
    m_deviceDropdown->setPlaceholderText("Select Audio Device");
    m_deviceDropdown->setOnSelectionChanged([this](int index, int value, const std::string& text) {
        m_selectedDeviceId = static_cast<uint32_t>(value);
    });
    addChild(m_deviceDropdown);

    m_sampleRateDropdown = std::make_shared<NomadUI::NUIDropdown>();
    m_sampleRateDropdown->setPlaceholderText("Select Sample Rate");
    m_sampleRateDropdown->setOnSelectionChanged([this](int index, int value, const std::string& text) {
        m_selectedSampleRate = static_cast<uint32_t>(value);
    });
    addChild(m_sampleRateDropdown);

    m_bufferSizeDropdown = std::make_shared<NomadUI::NUIDropdown>();
    m_bufferSizeDropdown->setPlaceholderText("Select Buffer Size");
    m_bufferSizeDropdown->setOnSelectionChanged([this](int index, int value, const std::string& text) {
        m_selectedBufferSize = static_cast<uint32_t>(value);
    });
    addChild(m_bufferSizeDropdown);

    // Create buttons
    m_applyButton = std::make_shared<NomadUI::NUIButton>();
    m_applyButton->setText("Apply");
    m_applyButton->setOnClick([this]() {
        applySettings();
    });
    addChild(m_applyButton);
    
    m_cancelButton = std::make_shared<NomadUI::NUIButton>();
    m_cancelButton->setText("Cancel");
    m_cancelButton->setOnClick([this]() {
        cancelSettings();
    });
    addChild(m_cancelButton);
    
    // Load lists
    updateDeviceList();
    updateSampleRateList();
    updateBufferSizeList();
}

void AudioSettingsDialog::show() {
    m_visible = true;
    setVisible(true);
    loadCurrentSettings();
    layoutComponents();
}

void AudioSettingsDialog::hide() {
    m_visible = false;
    setVisible(false);
}

void AudioSettingsDialog::setVisible(bool visible) {
    m_visible = visible;
    NomadUI::NUIComponent::setVisible(visible);
}

void AudioSettingsDialog::onRender(NomadUI::NUIRenderer& renderer) {
    if (!m_visible) return;
    
    // Render background overlay
    renderBackground(renderer);
    
    // Render dialog
    renderDialog(renderer);
    
    // Render all children (buttons)
    NomadUI::NUIComponent::onRender(renderer);
    
    // Render dropdown lists after all other components for proper z-order
    if (m_deviceDropdown && m_deviceDropdown->isOpen()) {
        m_deviceDropdown->renderDropdownList(renderer);
    }
    if (m_sampleRateDropdown && m_sampleRateDropdown->isOpen()) {
        m_sampleRateDropdown->renderDropdownList(renderer);
    }
    if (m_bufferSizeDropdown && m_bufferSizeDropdown->isOpen()) {
        m_bufferSizeDropdown->renderDropdownList(renderer);
    }
}

void AudioSettingsDialog::onResize(int width, int height) {
    // Center dialog
    m_dialogBounds.x = (width - m_dialogBounds.width) / 2;
    m_dialogBounds.y = (height - m_dialogBounds.height) / 2;
    
        layoutComponents();
}

bool AudioSettingsDialog::onMouseEvent(const NomadUI::NUIMouseEvent& event) {
    if (!m_visible) return false;
    
    // Check if click is outside dialog
    if (event.pressed && event.button == NomadUI::NUIMouseButton::Left) {
        if (!m_dialogBounds.contains(event.position.x, event.position.y)) {
            hide();
            return true;
        }
    }
    
    // Let children handle events (buttons will handle their own clicks)
    return NomadUI::NUIComponent::onMouseEvent(event);
}

bool AudioSettingsDialog::onKeyEvent(const NomadUI::NUIKeyEvent& event) {
    if (!m_visible) return false;
    
    if (event.pressed) {
        if (event.keyCode == NomadUI::NUIKeyCode::Escape) {
            hide();
            return true;
        }
    }
    
    return NomadUI::NUIComponent::onKeyEvent(event);
}

void AudioSettingsDialog::updateDeviceList() {
    if (!m_audioManager) return;
    
    m_devices = m_audioManager->getDevices();
    m_deviceDropdown->clearItems();
    
    for (const auto& device : m_devices) {
        m_deviceDropdown->addItem(device.name, device.id);
        if (device.id == m_selectedDeviceId) {
            m_deviceDropdown->setSelectedIndex(m_deviceDropdown->getItemCount() - 1);
        }
    }
}

void AudioSettingsDialog::updateSampleRateList() {
    m_sampleRates = {44100, 48000, 88200, 96000, 176400, 192000};
    m_sampleRateDropdown->clearItems();
    
    for (const auto& rate : m_sampleRates) {
        std::string text = std::to_string(rate) + " Hz";
        m_sampleRateDropdown->addItem(text, rate);
        if (rate == m_selectedSampleRate) {
            m_sampleRateDropdown->setSelectedIndex(m_sampleRateDropdown->getItemCount() - 1);
        }
    }
}

void AudioSettingsDialog::updateBufferSizeList() {
    m_bufferSizes = {64, 128, 256, 512, 1024, 2048};
    m_bufferSizeDropdown->clearItems();
    
    for (const auto& size : m_bufferSizes) {
        std::string text = std::to_string(size) + " samples";
        m_bufferSizeDropdown->addItem(text, size);
        if (size == m_selectedBufferSize) {
            m_bufferSizeDropdown->setSelectedIndex(m_bufferSizeDropdown->getItemCount() - 1);
        }
    }
}

void AudioSettingsDialog::loadCurrentSettings() {
    if (!m_audioManager) return;
    
    // Load current audio settings from the current config
    const auto& config = m_audioManager->getCurrentConfig();
    m_originalDeviceId = config.deviceId;
    m_originalSampleRate = config.sampleRate;
    m_originalBufferSize = config.bufferSize;
    
    // Set selected values
    m_selectedDeviceId = m_originalDeviceId;
    m_selectedSampleRate = m_originalSampleRate;
    m_selectedBufferSize = m_originalBufferSize;
}

void AudioSettingsDialog::applySettings() {
    if (!m_audioManager) return;
    
    // Apply new settings
    m_audioManager->switchDevice(m_selectedDeviceId);
    m_audioManager->setSampleRate(m_selectedSampleRate);
    m_audioManager->setBufferSize(m_selectedBufferSize);
    
    // Close dialog
    hide();
    
    // Notify callback
        if (m_onApply) {
            m_onApply();
    }
}

void AudioSettingsDialog::cancelSettings() {
    // Restore original settings
    m_selectedDeviceId = m_originalDeviceId;
    m_selectedSampleRate = m_originalSampleRate;
    m_selectedBufferSize = m_originalBufferSize;
    
    // Close dialog
    hide();
    
    // Notify callback
    if (m_onCancel) {
        m_onCancel();
    }
}

void AudioSettingsDialog::layoutComponents() {
    if (!m_visible) return;
    
    float padding = 20.0f;
    float labelWidth = 120.0f;
    float dropdownWidth = 250.0f;
    float dropdownHeight = 32.0f;
    float buttonWidth = 100.0f;
    float buttonHeight = 32.0f;
    float buttonSpacing = 10.0f;
    float verticalSpacing = 20.0f;
    
    // Start position for components
    float startY = m_dialogBounds.y + 60.0f;
    float labelX = m_dialogBounds.x + padding;
    float dropdownX = labelX + labelWidth + padding;
    
    // Device selector
    m_deviceLabel->setBounds(NomadUI::NUIRect(labelX, startY, labelWidth, dropdownHeight));
    m_deviceDropdown->setBounds(NomadUI::NUIRect(dropdownX, startY, dropdownWidth, dropdownHeight));
    
    // Sample rate selector
    startY += dropdownHeight + verticalSpacing;
    m_sampleRateLabel->setBounds(NomadUI::NUIRect(labelX, startY, labelWidth, dropdownHeight));
    m_sampleRateDropdown->setBounds(NomadUI::NUIRect(dropdownX, startY, dropdownWidth, dropdownHeight));
    
    // Buffer size selector
    startY += dropdownHeight + verticalSpacing;
    m_bufferSizeLabel->setBounds(NomadUI::NUIRect(labelX, startY, labelWidth, dropdownHeight));
    m_bufferSizeDropdown->setBounds(NomadUI::NUIRect(dropdownX, startY, dropdownWidth, dropdownHeight));
    
    // Position buttons at bottom
    float buttonY = m_dialogBounds.y + m_dialogBounds.height - buttonHeight - padding;
    float buttonX = m_dialogBounds.x + m_dialogBounds.width - (buttonWidth * 2 + buttonSpacing) - padding;
    
    // Apply button
    m_applyButton->setBounds(NomadUI::NUIRect(buttonX, buttonY, buttonWidth, buttonHeight));
    
    // Cancel button
    m_cancelButton->setBounds(NomadUI::NUIRect(buttonX + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight));
}

void AudioSettingsDialog::renderBackground(NomadUI::NUIRenderer& renderer) {
    // Render semi-transparent overlay
    auto& themeManager = NomadUI::NUIThemeManager::getInstance();
    NomadUI::NUIColor overlayColor = themeManager.getColor("backgroundPrimary");
    overlayColor = overlayColor.withAlpha(0.8f);
    
    NomadUI::NUIRect overlay(0, 0, 2000, 2000); // Large overlay
    renderer.fillRect(overlay, overlayColor);
}

void AudioSettingsDialog::renderDialog(NomadUI::NUIRenderer& renderer) {
    auto& themeManager = NomadUI::NUIThemeManager::getInstance();
    
    // Dialog background
    NomadUI::NUIColor bgColor = themeManager.getColor("backgroundSecondary");
    renderer.fillRoundedRect(m_dialogBounds, 8, bgColor);
    
    // Dialog border
    NomadUI::NUIColor borderColor = themeManager.getColor("border");
    renderer.strokeRoundedRect(m_dialogBounds, 8, 1, borderColor);
    
    // Title
    NomadUI::NUIColor textColor = themeManager.getColor("textPrimary");
    float titleY = m_dialogBounds.y + 20;
    float titleX = m_dialogBounds.x + 20;
    renderer.drawText("Audio Settings", NomadUI::NUIPoint(titleX, titleY), 18, textColor);
}

} // namespace Nomad